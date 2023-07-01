
/*
  ____  ____  _  _  ____  __  __  ___
 (  _ \(_  _)( \/ )(  _ \(  \/  )/ __)
  )(_) )_)(_  \  /  ) _ < )    ( \__ \
 (____/(____) (__) (____/(_/\/\_)(___/

DIYBMS V4.0
ESP8266 MODULE

(c)2019 Stuart Pittaway

COMPILE THIS CODE USING PLATFORM.IO

LICENSE
Attribution-NonCommercial-ShareAlike 2.0 UK: England & Wales (CC BY-NC-SA 2.0 UK)
https://creativecommons.org/licenses/by-nc-sa/2.0/uk/

* Non-Commercial — You may not use the material for commercial purposes.
* Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
  You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
* ShareAlike — If you remix, transform, or build upon the material, you must distribute your   contributions under the same license as the original.
* No additional restrictions — You may not apply legal terms or technological measures that legally restrict others from doing anything the license permits.
*/

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "FS.h"


#include "ESP8266TrueRandom.h"
#include <TimeLib.h>
#include <LittleFS.h>


#include "defines.h"
#include "DIYBMSServer.h"
#include "EmbeddedFiles_AutoGenerated.h"
#include "EmbeddedFiles_Integrity.h"

#include "settings.h"

#include "nimh_bms.h"

AsyncWebServer *DIYBMSServer::_myserver;
String DIYBMSServer::UUIDString;

sdcard_info (*DIYBMSServer::_sdcardcallback)() = 0;
void (*DIYBMSServer::_sdcardaction_callback)(uint8_t action) = 0;
PacketRequestGenerator *DIYBMSServer::_prg = 0;
PacketReceiveProcessor *DIYBMSServer::_receiveProc = 0;
diybms_eeprom_settings *DIYBMSServer::_mysettings = 0;
Rules *DIYBMSServer::_rules = 0;
ControllerState *DIYBMSServer::_controlState = 0;

#define REBOOT_COUNT_DOWN 2000

String DIYBMSServer::uuidToString(uint8_t *uuidLocation)
{
  String string = "";
  int i;
  for (i = 0; i < 16; i++)
  {
    if (i == 4)
      string += "-";
    if (i == 6)
      string += "-";
    if (i == 8)
      string += "-";
    if (i == 10)
      string += "-";
    int topDigit = uuidLocation[i] >> 4;
    int bottomDigit = uuidLocation[i] & 0x0f;
    // High hex digit
    string += "0123456789abcdef"[topDigit];
    // Low hex digit
    string += "0123456789abcdef"[bottomDigit];
  }

  return string;
}

void DIYBMSServer::generateUUID()
{
  //SERIAL_DEBUG.print("generateUUID=");
  byte uuidNumber[16]; // UUIDs in binary form are 16 bytes long

  ESP8266TrueRandom.uuid(uuidNumber);



  UUIDString = uuidToString(uuidNumber);
}

bool DIYBMSServer::validateXSS(AsyncWebServerRequest *request)
{
  if (request->hasHeader("Cookie"))
  {
    AsyncWebHeader *cookie = request->getHeader("Cookie");
    if (cookie->value().startsWith("DIYBMS_XSS="))
    {
      if (cookie->value().substring(11).equals(DIYBMSServer::UUIDString))
      {
        if (request->hasParam("xss", true))
        {
          AsyncWebParameter *p1 = request->getParam("xss", true);

          if (p1->value().equals(DIYBMSServer::UUIDString) == true)
          {
            return true;
          }
        }
      }
    }
  }
  request->send(500, "text/plain", "XSS invalid");
  return false;
}

void DIYBMSServer::SendSuccess(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  StaticJsonDocument<100> doc;
  doc["success"] = true;
  serializeJson(doc, *response);
  request->send(response);
}

void DIYBMSServer::SendFailure(AsyncWebServerRequest *request)
{
  request->send(500, "text/plain", "Failed");
}

void DIYBMSServer::sdMount(AsyncWebServerRequest *request)
{
  if (!validateXSS(request))
    return;

  (*DIYBMSServer::_sdcardaction_callback)(1);

  SendSuccess(request);
}
void DIYBMSServer::sdUnmount(AsyncWebServerRequest *request)
{
  if (!validateXSS(request))
    return;

  (*DIYBMSServer::_sdcardaction_callback)(0);

  SendSuccess(request);
}

void DIYBMSServer::resetCounters(AsyncWebServerRequest *request)
{
  if (!validateXSS(request))
    return;

  //Ask modules to reset bad packet counters
  _prg->sendBadPacketCounterReset();
  _prg->sendResetBalanceCurrentCounter();

  for (uint8_t i = 0; i < maximum_controller_cell_modules; i++)
  {
    cmi[i].badPacketCount = 0;
    cmi[i].PacketReceivedCount = 0;
  }

  //Reset internal counters on CONTROLLER
  _receiveProc->ResetCounters();

  _prg->packetsGenerated = 0;

  SendSuccess(request);
}

void DIYBMSServer::saveDisplaySetting(AsyncWebServerRequest *request)
{
  if (!validateXSS(request))
    return;

  if (request->hasParam("VoltageHigh", true))
  {
    AsyncWebParameter *p1 = request->getParam("VoltageHigh", true);
    _mysettings->graph_voltagehigh = p1->value().toFloat();
  }

  if (request->hasParam("VoltageLow", true))
  {
    AsyncWebParameter *p1 = request->getParam("VoltageLow", true);
    _mysettings->graph_voltagelow = p1->value().toFloat();
  }

  //Validate high is greater than low
  if (_mysettings->graph_voltagelow > _mysettings->graph_voltagehigh)
  {
    _mysettings->graph_voltagelow = 0;
  }

  saveConfiguration();

  SendSuccess(request);
}

void DIYBMSServer::saveInfluxDBSetting(AsyncWebServerRequest *request)
{
  if (!validateXSS(request))
    return;

  if (request->hasParam("influxEnabled", true))
  {
    AsyncWebParameter *p1 = request->getParam("influxEnabled", true);
    _mysettings->influxdb_enabled = p1->value().equals("on") ? true : false;
  }
  else
  {
    _mysettings->influxdb_enabled = false;
  }

  if (request->hasParam("influxPort", true))
  {
    AsyncWebParameter *p1 = request->getParam("influxPort", true);
    _mysettings->influxdb_httpPort = p1->value().toInt();
  }

  if (request->hasParam("influxServer", true))
  {
    AsyncWebParameter *p1 = request->getParam("influxServer", true);
    p1->value().toCharArray(_mysettings->influxdb_host, sizeof(_mysettings->influxdb_host));
  }

  if (request->hasParam("influxDatabase", true))
  {
    AsyncWebParameter *p1 = request->getParam("influxDatabase", true);
    p1->value().toCharArray(_mysettings->influxdb_database, sizeof(_mysettings->influxdb_database));
  }

  if (request->hasParam("influxUsername", true))
  {
    AsyncWebParameter *p1 = request->getParam("influxUsername", true);
    p1->value().toCharArray(_mysettings->influxdb_user, sizeof(_mysettings->influxdb_user));
  }

  if (request->hasParam("influxPassword", true))
  {
    AsyncWebParameter *p1 = request->getParam("influxPassword", true);
    p1->value().toCharArray(_mysettings->influxdb_password, sizeof(_mysettings->influxdb_password));
  }

  saveConfiguration();

  //ConfigHasChanged = REBOOT_COUNT_DOWN;
  SendSuccess(request);
}

void DIYBMSServer::saveRuleConfiguration(AsyncWebServerRequest *request)
{
  if (!validateXSS(request))
    return;

  //relaytype
  for (int i = 0; i < RELAY_TOTAL; i++)
  {
    String name = "relaytype";
    name = name + (i + 1);
    if (request->hasParam(name.c_str(), true, false))
    {
      AsyncWebParameter *p1 = request->getParam(name.c_str(), true, false);
      //Default
      _mysettings->relaytype[i] = RelayType::RELAY_STANDARD;
      if (p1->value().equals("Pulse"))
      {
        _mysettings->relaytype[i] = RelayType::RELAY_PULSE;
      }
    }
  }

  //Relay default
  for (int i = 0; i < RELAY_TOTAL; i++)
  {
    String name = "defaultrelay";
    name = name + (i + 1);
    if (request->hasParam(name.c_str(), true, false))
    {
      AsyncWebParameter *p1 = request->getParam(name.c_str(), true, false);
      //Default
      _mysettings->rulerelaydefault[i] = RelayState::RELAY_OFF;
      if (p1->value().equals("On"))
      {
        _mysettings->rulerelaydefault[i] = RelayState::RELAY_ON;
      }
    }
  }

  for (int rule = 0; rule < RELAY_RULES; rule++)
  {

    //TODO: This STRING doesnt work properly if its on a single line!
    String name = "rule";
    name = name + (rule + 1);
    name = name + "value";

    if (request->hasParam(name, true))
    {
      AsyncWebParameter *p1 = request->getParam(name, true);
      _mysettings->rulevalue[rule] = p1->value().toInt();
    }

    //TODO: This STRING doesnt work properly if its on a single line!
    String hname = "rule";
    hname = hname + (rule + 1);
    hname = hname + "hysteresis";
    if (request->hasParam(hname, true))
    {
      AsyncWebParameter *p1 = request->getParam(hname, true);
      _mysettings->rulehysteresis[rule] = p1->value().toInt();
    }

    //Rule/relay processing
    for (int i = 0; i < RELAY_TOTAL; i++)
    {
      //TODO: This STRING doesnt work properly if its on a single line!
      String name = "rule";
      name = name + (rule + 1);
      name = name + "relay";
      name = name + (i + 1);
      if (request->hasParam(name, true))
      {
        AsyncWebParameter *p1 = request->getParam(name, true);
        _mysettings->rulerelaystate[rule][i] = p1->value().equals("X") ? RELAY_X : p1->value().equals("On") ? RelayState::RELAY_ON : RelayState::RELAY_OFF;
      }
    }

    //Reset state of rules after updating the new values
    for (int8_t r = 0; r < RELAY_RULES; r++)
    {
      _rules->rule_outcome[r] = false;
    }
  }

  saveConfiguration();

  SendSuccess(request);
}

void DIYBMSServer::saveStorage(AsyncWebServerRequest *request)
{
  if (!validateXSS(request))
    return;

  if (request->hasParam("loggingEnabled", true))
  {
    AsyncWebParameter *p1 = request->getParam("loggingEnabled", true);
    _mysettings->loggingEnabled = p1->value().equals("on") ? true : false;
  }

  if (request->hasParam("loggingFreq", true))
  {
    AsyncWebParameter *p1 = request->getParam("loggingFreq", true);
    _mysettings->loggingFrequencySeconds = p1->value().toInt();
    //Validate
    if (_mysettings->loggingFrequencySeconds < 15 || _mysettings->loggingFrequencySeconds > 600)
    {
      _mysettings->loggingFrequencySeconds = 15;
    }
  }

  saveConfiguration();

  SendSuccess(request);
}

void DIYBMSServer::saveNTP(AsyncWebServerRequest *request)
{
  if (!validateXSS(request))
    return;

  if (request->hasParam("NTPServer", true))
  {
    AsyncWebParameter *p1 = request->getParam("NTPServer", true);
    p1->value().toCharArray(_mysettings->ntpServer, sizeof(_mysettings->ntpServer));
  }

  if (request->hasParam("NTPZoneHour", true))
  {
    AsyncWebParameter *p1 = request->getParam("NTPZoneHour", true);
    _mysettings->timeZone = p1->value().toInt();
  }

  if (request->hasParam("NTPZoneMin", true))
  {
    AsyncWebParameter *p1 = request->getParam("NTPZoneMin", true);
    _mysettings->minutesTimeZone = p1->value().toInt();
  }

  _mysettings->daylight = false;
  if (request->hasParam("NTPDST", true))
  {
    AsyncWebParameter *p1 = request->getParam("NTPDST", true);
    _mysettings->daylight = p1->value().equals("on") ? true : false;
  }

  saveConfiguration();

  //ConfigHasChanged = REBOOT_COUNT_DOWN;
  SendSuccess(request);
}

void DIYBMSServer::saveBankConfiguration(AsyncWebServerRequest *request)
{
  if (!validateXSS(request))
    return;

  uint8_t totalSeriesModules = 1;
  uint8_t totalBanks = 1;

  if (request->hasParam("totalSeriesModules", true))
  {
    AsyncWebParameter *p1 = request->getParam("totalSeriesModules", true);
    totalSeriesModules = p1->value().toInt();
  }

  if (request->hasParam("totalBanks", true))
  {
    AsyncWebParameter *p1 = request->getParam("totalBanks", true);
    totalBanks = p1->value().toInt();
  }

  if (totalSeriesModules * totalBanks <= maximum_controller_cell_modules)
  {
    _mysettings->totalNumberOfSeriesModules = totalSeriesModules;
    _mysettings->totalNumberOfBanks = totalBanks;
    saveConfiguration();

    SendSuccess(request);
  }
  else
  {
    SendFailure(request);
  }
}

void DIYBMSServer::saveMQTTSetting(AsyncWebServerRequest *request)
{
  if (!validateXSS(request))
    return;

  if (request->hasParam("mqttEnabled", true))
  {
    AsyncWebParameter *p1 = request->getParam("mqttEnabled", true);
    _mysettings->mqtt_enabled = p1->value().equals("on") ? true : false;
  }
  else
  {
    _mysettings->mqtt_enabled = false;
  }

  if (request->hasParam("mqttTopic", true))
  {
    AsyncWebParameter *p1 = request->getParam("mqttTopic", true);
    p1->value().toCharArray(_mysettings->mqtt_topic, sizeof(_mysettings->mqtt_topic));
  }
  else
  {
    sprintf(_mysettings->mqtt_topic, "diybms");
  }

  if (request->hasParam("mqttPort", true))
  {
    AsyncWebParameter *p1 = request->getParam("mqttPort", true);
    _mysettings->mqtt_port = p1->value().toInt();
  }

  if (request->hasParam("mqttServer", true))
  {
    AsyncWebParameter *p1 = request->getParam("mqttServer", true);
    p1->value().toCharArray(_mysettings->mqtt_server, sizeof(_mysettings->mqtt_server));
  }

  if (request->hasParam("mqttUsername", true))
  {
    AsyncWebParameter *p1 = request->getParam("mqttUsername", true);
    p1->value().toCharArray(_mysettings->mqtt_username, sizeof(_mysettings->mqtt_username));
  }

  if (request->hasParam("mqttPassword", true))
  {
    AsyncWebParameter *p1 = request->getParam("mqttPassword", true);
    p1->value().toCharArray(_mysettings->mqtt_password, sizeof(_mysettings->mqtt_password));
  }

  saveConfiguration();

  SendSuccess(request);
}

void DIYBMSServer::saveGlobalSetting(AsyncWebServerRequest *request)
{
  if (!validateXSS(request))
    return;

  if (request->hasParam("BypassOverTempShutdown", true) && request->hasParam("BypassThresholdmV", true) &&
      request->hasParam("min_temperature", true) && request->hasParam("max_temperature", true) &&
      request->hasParam("min_voltage", true) && request->hasParam("max_voltage", true)
  )
  {

    AsyncWebParameter *p1 = request->getParam("BypassOverTempShutdown", true);
    _mysettings->BypassOverTempShutdown = p1->value().toInt();

    AsyncWebParameter *p2 = request->getParam("BypassThresholdmV", true);
    _mysettings->BypassThresholdmV = p2->value().toInt();

    AsyncWebParameter *p3 = request->getParam("min_temperature", true);
    _mysettings->min_admisible_temperature = p3->value().toInt();

    AsyncWebParameter *p7 = request->getParam("min_temperature", true);
    _mysettings->min_admisible_temperature = p7->value().toInt();

    AsyncWebParameter *p4 = request->getParam("max_temperature", true);
    _mysettings->max_admisible_temperature = p4->value().toInt();

    AsyncWebParameter *p5 = request->getParam("min_voltage", true);
    _mysettings->min_admisble_voltage = p5->value().toInt();

    AsyncWebParameter *p6 = request->getParam("max_voltage", true);
    _mysettings->max_admisible_voltage = p6->value().toInt();

    saveConfiguration();
    nimh_bms_set_limits(_mysettings->max_admisible_voltage, 
                      _mysettings->min_admisble_voltage, 
                      _mysettings->max_admisible_temperature, 
                      _mysettings->min_admisible_temperature);

    _prg->sendSaveGlobalSetting(_mysettings->BypassThresholdmV, _mysettings->BypassOverTempShutdown);

    uint8_t totalModules = _mysettings->totalNumberOfBanks * _mysettings->totalNumberOfSeriesModules;

    for (uint8_t i = 0; i < totalModules; i++)
    {
      if (cmi[i].valid)
      {
        cmi[i].BypassThresholdmV = _mysettings->BypassThresholdmV;
        cmi[i].BypassOverTempShutdown = _mysettings->BypassOverTempShutdown;
      }
    }

    //Just returns NULL
    SendSuccess(request);
  }
  else
  {
    request->send(500, "text/plain", "Missing parameters");
  }
}

void DIYBMSServer::handleNotFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void DIYBMSServer::saveSetting(AsyncWebServerRequest *request)
{
  if (!validateXSS(request))
    return;

  if (request->hasParam("m", true))
  {
    AsyncWebParameter *module = request->getParam("m", true);
    //Will this overflow?
    uint8_t m = module->value().toInt();

    if (m > maximum_controller_cell_modules)
    {
      request->send(500, "text/plain", "Wrong parameters");
    }
    else
    {

      uint8_t BypassOverTempShutdown = 0xFF;
      uint16_t BypassThresholdmV = 0xFFFF;

      // Resistance of bypass load
      //float LoadResistance = 0xFFFF;
      //Voltage Calibration
      float Calibration = 0xFFFF;
      //Reference voltage (millivolt) normally 2.00mV
      //float mVPerADC = 0xFFFF;
      //Internal Thermistor settings
      //uint16_t Internal_BCoefficient = 0xFFFF;
      //External Thermistor settings
      //uint16_t External_BCoefficient = 0xFFFF;

      if (request->hasParam("BypassOverTempShutdown", true))
      {
        AsyncWebParameter *p1 = request->getParam("BypassOverTempShutdown", true);
        BypassOverTempShutdown = p1->value().toInt();
      }

      if (request->hasParam("BypassThresholdmV", true))
      {
        AsyncWebParameter *p1 = request->getParam("BypassThresholdmV", true);
        BypassThresholdmV = p1->value().toInt();
      }
      if (request->hasParam("Calib", true))
      {
        AsyncWebParameter *p1 = request->getParam("Calib", true);
        Calibration = p1->value().toFloat();
      }

      _prg->sendSaveSetting(m, BypassThresholdmV, BypassOverTempShutdown, Calibration);

      clearModuleValues(m);

      SendSuccess(request);
    }
  }
  else
  {
    request->send(500, "text/plain", "Missing parameters");
  }
}

void DIYBMSServer::clearModuleValues(uint8_t module)
{
  cmi[module].valid = false;
  cmi[module].voltagemV = 0;
  cmi[module].voltagemVMin = 6000;
  cmi[module].voltagemVMax = 0;
  cmi[module].badPacketCount = 0;
  cmi[module].inBypass = false;
  cmi[module].bypassOverTemp = false;
  cmi[module].internalTemp = -40;
  cmi[module].externalTemp = -40;
}

void DIYBMSServer::GetRules(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream("application/json");

  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>();

  root["timenow"] = (hour() * 60) + minute();

  root["OutputsEnabled"] = OutputsEnabled;
  root["InputsEnabled"] = InputsEnabled;
  root["ControlState"] = (*_controlState);

  JsonArray defaultArray = root.createNestedArray("relaydefault");
  for (uint8_t relay = 0; relay < RELAY_TOTAL; relay++)
  {
    switch (_mysettings->rulerelaydefault[relay])
    {
    case RELAY_OFF:
      defaultArray.add(false);
      break;
    case RELAY_ON:
      defaultArray.add(true);
      break;
    default:
      defaultArray.add((char *)0);
      break;
    }
  }

  JsonArray typeArray = root.createNestedArray("relaytype");
  for (uint8_t relay = 0; relay < RELAY_TOTAL; relay++)
  {
    switch (_mysettings->relaytype[relay])
    {
    case RELAY_STANDARD:
      typeArray.add("Std");
      break;
    case RELAY_PULSE:
      typeArray.add("Pulse");
      break;
    default:
      typeArray.add((char *)0);
      break;
    }
  }

  JsonArray bankArray = root.createNestedArray("rules");

  for (uint8_t r = 0; r < RELAY_RULES; r++)
  {
    JsonObject rule = bankArray.createNestedObject();
    rule["value"] = _mysettings->rulevalue[r];
    rule["hysteresis"] = _mysettings->rulehysteresis[r];
    rule["triggered"] = _rules->rule_outcome[r];
    JsonArray data = rule.createNestedArray("relays");

    for (uint8_t relay = 0; relay < RELAY_TOTAL; relay++)
    {
      switch (_mysettings->rulerelaystate[r][relay])
      {
      case RELAY_OFF:
        data.add(false);
        break;
      case RELAY_ON:
        data.add(true);
        break;
      default:
        data.add((char *)0);
        break;
      }
    }
  }

  serializeJson(doc, *response);
  request->send(response);
}

void DIYBMSServer::settings(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream("application/json");

  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>();

  JsonObject settings = root.createNestedObject("settings");

  //settings["Version"] = String(GIT_VERSION);
  //settings["CompileDate"] = String(COMPILE_DATE_TIME);

  settings["totalnumberofbanks"] = _mysettings->totalNumberOfBanks;
  settings["totalseriesmodules"] = _mysettings->totalNumberOfSeriesModules;

  settings["bypassthreshold"] = _mysettings->BypassThresholdmV;
  settings["bypassovertemp"] = _mysettings->BypassOverTempShutdown;

  settings["min_temperature"] = _mysettings->min_admisible_temperature;
  settings["max_temperature"] = _mysettings->max_admisible_temperature;
  settings["min_voltage"] = _mysettings->min_admisble_voltage;
  settings["max_voltage"] = _mysettings->max_admisible_voltage;

  settings["NTPServerName"] = _mysettings->ntpServer;
  settings["TimeZone"] = _mysettings->timeZone;
  settings["MinutesTimeZone"] = _mysettings->minutesTimeZone;
  settings["DST"] = _mysettings->daylight;

  settings["FreeHeap"] = ESP.getFreeHeap();
  settings["FreeBlockSize"] = ESP.getMaxFreeBlockSize();

  settings["now"] = now();

  response->addHeader("Cache-Control", "no-store");

  serializeJson(doc, *response);
  request->send(response);
}

void DIYBMSServer::storage(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream("application/json");

  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>();

  JsonObject settings = root.createNestedObject("storage");

  settings["enabled"] = _mysettings->loggingEnabled;
  settings["frequency"] = _mysettings->loggingFrequencySeconds;

  if (DIYBMSServer::_sdcardcallback != 0)
  {
    //Get data from main.cpp
    sdcard_info info = (*DIYBMSServer::_sdcardcallback)();

    settings["sdcard"] = info.available;
    settings["sdcard_total"] = info.totalkilobytes;
    settings["sdcard_used"] = info.usedkilobytes;

    settings["flash_total"] = info.flash_totalkilobytes;
    settings["flash_used"] = info.flash_usedkilobytes;
  }
  response->addHeader("Cache-Control", "no-store");

  serializeJson(doc, *response);
  request->send(response);
}

void DIYBMSServer::integration(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream("application/json");

  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>();

  JsonObject mqtt = root.createNestedObject("mqtt");
  mqtt["enabled"] = _mysettings->mqtt_enabled;
  mqtt["topic"] = _mysettings->mqtt_topic;
  mqtt["port"] = _mysettings->mqtt_port;
  mqtt["server"] = _mysettings->mqtt_server;
  mqtt["username"] = _mysettings->mqtt_username;
  //We don't output the password in the json file as this could breach security
  //mqtt["password"] =_mysettings->mqtt_password;

  JsonObject influxdb = root.createNestedObject("influxdb");
  influxdb["enabled"] = _mysettings->influxdb_enabled;
  influxdb["port"] = _mysettings->influxdb_httpPort;
  influxdb["server"] = _mysettings->influxdb_host;
  influxdb["database"] = _mysettings->influxdb_database;
  influxdb["username"] = _mysettings->influxdb_user;
  //We don't output the password in the json file as this could breach security
  //influxdb["password"] = _mysettings->influxdb_password;

  serializeJson(doc, *response);
  request->send(response);
}

void DIYBMSServer::identifyModule(AsyncWebServerRequest *request)
{
  if (request->hasParam("c", false))
  {
    AsyncWebParameter *cellid = request->getParam("c", false);
    uint8_t c = cellid->value().toInt();

    if (c > _mysettings->totalNumberOfBanks * _mysettings->totalNumberOfSeriesModules)
    {
      request->send(500, "text/plain", "Wrong parameter bank");
      return;
    }
    else
    {
      _prg->sendIdentifyModuleRequest(c);
      SendSuccess(request);
    }
  }
  else
  {
    request->send(500, "text/plain", "Missing parameters");
  }
}

void DIYBMSServer::modules(AsyncWebServerRequest *request)
{
  if (request->hasParam("c", false))
  {
    AsyncWebParameter *cellid = request->getParam("c", false);
    uint8_t c = cellid->value().toInt();

    if (c > _mysettings->totalNumberOfBanks * _mysettings->totalNumberOfSeriesModules)
    {
      request->send(500, "text/plain", "Wrong parameter bank");
      return;
    }

    if (cmi[c].settingsCached == false)
    {
      _prg->sendGetSettingsRequest(c);
    }

    AsyncResponseStream *response = request->beginResponseStream("application/json");

    DynamicJsonDocument doc(2048);
    JsonObject root = doc.to<JsonObject>();
    JsonObject settings = root.createNestedObject("settings");

    uint8_t b = c / _mysettings->totalNumberOfSeriesModules;
    uint8_t m = c - (b * _mysettings->totalNumberOfSeriesModules);
    settings["bank"] = b;
    settings["module"] = m;
    settings["id"] = c;
    settings["ver"] = cmi[c].BoardVersionNumber;
    settings["code"] = cmi[c].CodeVersionNumber;
    settings["Cached"] = cmi[c].settingsCached;

    if (cmi[c].settingsCached)
    {
      settings["BypassOverTempShutdown"] = cmi[c].BypassOverTempShutdown;
      settings["BypassThresholdmV"] = cmi[c].BypassThresholdmV;
      settings["LoadRes"] = cmi[c].LoadResistance;
      settings["Calib"] = cmi[c].Calibration;
      settings["mVPerADC"] = cmi[c].mVPerADC;
      settings["IntBCoef"] = cmi[c].Internal_BCoefficient;
      settings["ExtBCoef"] = cmi[c].External_BCoefficient;
    }

    serializeJson(doc, *response);
    request->send(response);
  }
}

/*
Restart controller from web interface
*/
void DIYBMSServer::handleRestartController(AsyncWebServerRequest *request)
{
  ESP.restart();
}

void DIYBMSServer::monitor3(AsyncWebServerRequest *request)
{
  //DynamicJsonDocument doc(maximum_controller_cell_modules * 50);
  AsyncResponseStream *response = request->beginResponseStream("application/json");

  uint8_t totalModules = _mysettings->totalNumberOfBanks * _mysettings->totalNumberOfSeriesModules;
  uint8_t comma = totalModules - 1;

  response->print("{\"badpacket\":[");

  for (uint8_t i = 0; i < totalModules; i++)
  {
    if (cmi[i].valid)
    {
      response->print(cmi[i].badPacketCount);
    }
    else
    {
      //Return NULL
      response->print("null");
    }
    if (i < comma)
    {
      response->print(',');
    }
  }

  response->print("],\"balcurrent\":[");

  for (uint8_t i = 0; i < totalModules; i++)
  {
    if (cmi[i].valid)
    {
      response->print(cmi[i].BalanceCurrentCount);
    }
    else
    {
      //Return NULL
      response->print("null");
    }
    if (i < comma)
    {
      response->print(',');
    }
  }

  response->print("],\"pktrecvd\":[");

  for (uint8_t i = 0; i < totalModules; i++)
  {
    if (cmi[i].valid)
    {
      response->print(cmi[i].PacketReceivedCount);
    }
    else
    {
      //Return NULL
      response->print("null");
    }
    if (i < comma)
    {
      response->print(',');
    }
  }
  response->print("]}");

  request->send(response);
}

void DIYBMSServer::PrintStreamComma(AsyncResponseStream *response, const __FlashStringHelper *ifsh, uint32_t value)
{
  response->print(ifsh);
  response->print(value);
  response->print(',');
}

void DIYBMSServer::monitor_nimh_bms(AsyncWebServerRequest *request){
  uint8_t totalModules = _mysettings->totalNumberOfBanks * _mysettings->totalNumberOfSeriesModules;
  const char comma = ',';
  const char *null = "null";
  static nimh_bms* bms = nih_bms_get_struct();

  AsyncResponseStream *response = request->beginResponseStream("application/json");

  PrintStreamComma(response, F("{\"disable_charger\":"), bms->disable_charger);
  PrintStreamComma(response, F("\"disable_load\":"), bms->disable_load);
  PrintStreamComma(response, F("\"module_count\":"), bms->module_count);
  PrintStreamComma(response, F("\"upper_voltage_limit\":"), bms->upper_voltage_limit);
  PrintStreamComma(response, F("\"lower_voltage_limit\":"), bms->lower_voltage_limit);
  PrintStreamComma(response, F("\"upper_temperature_limit\":"), bms->upper_temperature_limit);
  PrintStreamComma(response, F("\"lower_temperature_limit\":"), bms->lower_temperature_limit);

  response->print(F("\"module_states\":["));
  for (size_t i = 0; i < bms->module_count; i++)
  {
      if (i){
        response->print(comma);
      }
      response->print("[");
      response->print(nimh_decode_state(bms->cell[i].state));
      response->print(comma);
      response->print(nimh_decode_error_state(bms->cell[i].error_state_volt));
      response->print(comma);
      response->print(nimh_decode_error_state(bms->cell[i].error_state_temp));
      response->print(comma);
      response->print("{");
        PrintStreamComma(response, F("\"timer\":"), bms->cell[i].timer % SAMPLE_RANGE);
        PrintStreamComma(response, F("\"max_temp\":"), bms->cell[i].max_temp[CURREN]);
        PrintStreamComma(response, F("\"min_temp\":"), bms->cell[i].min_temp[CURREN]);
        PrintStreamComma(response, F("\"max_voltage\":"), bms->cell[i].max_voltage[CURREN]);
        response->printf("\"min_voltage\": %d", bms->cell[i].min_voltage[CURREN]);
      response->print("}");
      response->print("]");
  }

  response->print("]");
  response->print('}');
  request->send(response);


}

void DIYBMSServer::monitor2(AsyncWebServerRequest *request)
{
  uint8_t totalModules = _mysettings->totalNumberOfBanks * _mysettings->totalNumberOfSeriesModules;
  const char comma = ',';
  const char *null = "null";

  AsyncResponseStream *response = request->beginResponseStream("application/json");

  PrintStreamComma(response, F("{\"banks\":"), _mysettings->totalNumberOfBanks);
  PrintStreamComma(response, F("\"seriesmodules\":"), _mysettings->totalNumberOfSeriesModules);
  PrintStreamComma(response, F("\"sent\":"), _prg->packetsGenerated);
  PrintStreamComma(response, F("\"received\":"), _receiveProc->packetsReceived);
  PrintStreamComma(response, F("\"modulesfnd\":"), _receiveProc->totalModulesFound);
  PrintStreamComma(response, F("\"badcrc\":"), _receiveProc->totalCRCErrors);
  PrintStreamComma(response, F("\"ignored\":"), _receiveProc->totalNotProcessedErrors);
  PrintStreamComma(response, F("\"roundtrip\":"), _receiveProc->packetTimerMillisecond);
  PrintStreamComma(response, F("\"oos\":"), _receiveProc->totalOutofSequenceErrors);

  response->print(F("\"errors\":["));
  for (size_t i = 0; i < sizeof(_rules->ErrorCodes); i++)
  {
    if (_rules->ErrorCodes[i] != InternalErrorCode::NoError)
    {
      //Comma if not zero
      if (i)
        response->print(comma);

      response->print(_rules->ErrorCodes[i]);
    }
  }

  response->print("],");

  response->print(F("\"warnings\":["));
  for (size_t i = 0; i < sizeof(_rules->WarningCodes); i++)
  {
    if (_rules->WarningCodes[i] != InternalWarningCode::NoWarning)
    {
      //Comma if not zero
      if (i)
        response->print(comma);

      response->print(_rules->WarningCodes[i]);
    }
  }
  response->print("],");

  //voltages
  response->print(F("\"voltages\":["));

  for (uint8_t i = 0; i < totalModules; i++)
  {
    //Comma if not zero
    if (i)
      response->print(comma);

    if (cmi[i].valid)
    {
      response->print(cmi[i].voltagemV);
    }
    else
    {
      //Module is not yet valid so return null values...
      response->print(null);
    }
  }
  response->print("],");

  response->print(F("\"minvoltages\":["));

  for (uint8_t i = 0; i < totalModules; i++)
  {
    //Comma if not zero
    if (i)
      response->print(comma);

    if (cmi[i].valid)
    {
      response->print(cmi[i].voltagemVMin);
    }
    else
    {
      //Module is not yet valid so return null values...
      response->print(null);
    }
  }
  response->print("],");

  //maxvoltages

  response->print(F("\"maxvoltages\":["));

  for (uint8_t i = 0; i < totalModules; i++)
  {
    //Comma if not zero
    if (i)
      response->print(comma);

    if (cmi[i].valid)
    {
      response->print(cmi[i].voltagemVMax);
    }
    else
    {
      //Module is not yet valid so return null values...
      response->print(null);
    }
  }
  response->print("]");

  response->print(comma);

  //inttemp
  response->print(F("\"inttemp\":["));

  for (uint8_t i = 0; i < totalModules; i++)
  {
    //Comma if not zero
    if (i)
      response->print(comma);

    if (cmi[i].valid && cmi[i].internalTemp != -40)
    {
      response->print(((float)cmi[i].internalTemp)/10.0);
    }
    else
    {
      //Module is not yet valid so return null values...
      response->print(null);
    }
  }
  response->print("]");

  response->print(comma);

  //exttemp
  response->print(F("\"exttemp\":["));

  for (uint8_t i = 0; i < totalModules; i++)
  {
    //Comma if not zero
    if (i)
      response->print(comma);

    if (cmi[i].valid && cmi[i].externalTemp != -40)
    {
      response->print(((float)cmi[i].externalTemp)/10.0);
    }
    else
    {
      //Module is not yet valid so return null values...
      response->print(null);
    }
  }
  response->print(']');

  response->print(comma);

  //bypass
  response->print(F("\"bypass\":["));

  for (uint8_t i = 0; i < totalModules; i++)
  {
    //Comma if not zero
    if (i)
      response->print(comma);

    if (cmi[i].valid && cmi[i].inBypass)
    {
      response->print('1');
    }
    else
    {
      response->print('0');
    }
  }
  response->print("]");

  response->print(comma);

  //bypasshot
  response->print(F("\"bypasshot\":["));

  for (uint8_t i = 0; i < totalModules; i++)
  {
    //Comma if not zero
    if (i)
      response->print(comma);

    if (cmi[i].valid && cmi[i].bypassOverTemp)
    {
      response->print('1');
    }
    else
    {
      response->print('0');
    }
  }
  response->print(']');

  response->print(comma);

  //bypasspwm
  response->print(F("\"bypasspwm\":["));

  for (uint8_t i = 0; i < totalModules; i++)
  {
    //Comma if not zero
    if (i)
      response->print(comma);

    if (cmi[i].valid && cmi[i].inBypass)
    {
      response->print(cmi[i].PWMValue);
    }
    else
    {
      response->print('0');
    }
  }
  response->print(']');

  response->print(comma);

  //bypasspwm
  response->print(F("\"bankv\":["));

  for (uint8_t i = 0; i < _mysettings->totalNumberOfBanks; i++)
  {
    //Comma if not zero
    if (i)
      response->print(comma);

    response->print(_rules->packvoltage[i]);
  }
  response->print("]");

  response->print(comma);

  //bypasspwm
  response->print(F("\"voltrange\":["));

  for (uint8_t i = 0; i < _mysettings->totalNumberOfBanks; i++)
  {
    //Comma if not zero
    if (i)
      response->print(comma);

    response->print(_rules->VoltageRangeInBank(i));
  }
  response->print("]");

  response->print(comma);
  response->print(F("\"current\":["));
  response->print(null);
  response->print("]");

  //The END...
  response->print('}');
  request->send(response);
}

String DIYBMSServer::TemplateProcessor(const String &var)
{
  if (var == "XSS_KEY")
    return DIYBMSServer::UUIDString;

#if defined(ESP8266)
  if (var == "PLATFORM")
    return String("ESP8266");
#endif

#if defined(ESP32)
  if (var == "PLATFORM")
    return String("ESP32");
#endif

  if (var == "GIT_VERSION")
    return String(GIT_VERSION);

  if (var == "COMPILE_DATE_TIME")
    return String(COMPILE_DATE_TIME);

  //  const DEFAULT_GRAPH_MAX_VOLTAGE = %graph_voltagehigh%;
  //  const DEFAULT_GRAPH_MIN_VOLTAGE = %graph_voltagelow%;

  if (var == "graph_voltagehigh")
    return String(_mysettings->graph_voltagehigh);

  if (var == "graph_voltagelow")
    return String(_mysettings->graph_voltagelow);

  if (var == "integrity_file_jquery_js")
    return String(integrity_file_jquery_js);

  return String();
}

void DIYBMSServer::SetCacheAndETagGzip(AsyncWebServerResponse *response, String ETag)
{
  response->addHeader("Content-Encoding", "gzip");
  SetCacheAndETag(response, ETag);
}
void DIYBMSServer::SetCacheAndETag(AsyncWebServerResponse *response, String ETag)
{
  response->addHeader("ETag", ETag);
  response->addHeader("Cache-Control", "no-cache, max-age=86400");
}

// Start Web Server (crazy amount of pointer params!)
void DIYBMSServer::StartServer(AsyncWebServer *webserver,
                               diybms_eeprom_settings *mysettings,
                               sdcard_info (*sdcardcallback)(),
                               PacketRequestGenerator *prg,
                               PacketReceiveProcessor *pktreceiveproc,
                               ControllerState *controlState,
                               Rules *rules,
                               void (*sdcardaction_callback)(uint8_t action))
{
  _myserver = webserver;
  _prg = prg;
  _controlState = controlState;
  _rules = rules;
  _sdcardcallback = sdcardcallback;
  _mysettings = mysettings;
  _receiveProc = pktreceiveproc;
  _sdcardaction_callback = sdcardaction_callback;

  String cookieValue = "DIYBMS_XSS=";
  cookieValue += DIYBMSServer::UUIDString;
  cookieValue += String("; path=/; HttpOnly");
  DefaultHeaders::Instance().addHeader("Set-Cookie", cookieValue);

  _myserver->on("/", HTTP_GET, [](AsyncWebServerRequest *request) { request->redirect("/default.htm"); });

  //file_default_htm
  //_myserver->serveStatic("/default.htm", LittleFS, "/default.htm").setTemplateProcessor(DIYBMSServer::TemplateProcessor);
  //_myserver->serveStatic("/", LittleFS, "/").setCacheControl("max-age=600");

  _myserver->on("/default.htm", HTTP_GET,
                [](AsyncWebServerRequest *request) {
                  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", (char *)file_default_htm, DIYBMSServer::TemplateProcessor);
                  response->addHeader("Cache-Control", "no-store");
                  request->send(response);
                });


  //Read endpoints
  _myserver->on("/nimh_bms.json", HTTP_GET, DIYBMSServer::monitor_nimh_bms);
  _myserver->on("/monitor2.json", HTTP_GET, DIYBMSServer::monitor2);
  _myserver->on("/monitor3.json", HTTP_GET, DIYBMSServer::monitor3);
  _myserver->on("/integration.json", HTTP_GET, DIYBMSServer::integration);
  _myserver->on("/modules.json", HTTP_GET, DIYBMSServer::modules);
  _myserver->on("/identifyModule.json", HTTP_GET, DIYBMSServer::identifyModule);
  _myserver->on("/settings.json", HTTP_GET, DIYBMSServer::settings);
  _myserver->on("/rules.json", HTTP_GET, DIYBMSServer::GetRules);
  _myserver->on("/storage.json", HTTP_GET, DIYBMSServer::storage);


  //POST method endpoints
  _myserver->on("/savesetting.json", HTTP_POST, DIYBMSServer::saveSetting);
  _myserver->on("/saveglobalsetting.json", HTTP_POST, DIYBMSServer::saveGlobalSetting);
  _myserver->on("/savemqtt.json", HTTP_POST, DIYBMSServer::saveMQTTSetting);
  _myserver->on("/saveinfluxdb.json", HTTP_POST, DIYBMSServer::saveInfluxDBSetting);
  _myserver->on("/savebankconfig.json", HTTP_POST, DIYBMSServer::saveBankConfiguration);
  _myserver->on("/saverules.json", HTTP_POST, DIYBMSServer::saveRuleConfiguration);
  _myserver->on("/saventp.json", HTTP_POST, DIYBMSServer::saveNTP);
  _myserver->on("/savedisplaysetting.json", HTTP_POST, DIYBMSServer::saveDisplaySetting);
  _myserver->on("/savestorage.json", HTTP_POST, DIYBMSServer::saveStorage);

  _myserver->on("/resetcounters.json", HTTP_POST, DIYBMSServer::resetCounters);
  _myserver->on("/restartcontroller.json", HTTP_POST, DIYBMSServer::handleRestartController);

  _myserver->on("/sdmount.json", HTTP_POST, DIYBMSServer::sdMount);
  _myserver->on("/sdunmount.json", HTTP_POST, DIYBMSServer::sdUnmount);

  _myserver->onNotFound(DIYBMSServer::handleNotFound);
  _myserver->begin();
}
