#include "nimh_bms.h"
#include <stdlib.h>

char *bms_state_string[]={
  "BMS_STATE_CRITICAL",
  "BMS_STATE_INITIALIZED",
  "BMS_STATE_CHARGING",
  "BMS_STATE_CHARGED",
  "BMS_STATE_DISCHARGING"
};

char *bms_errr_state_string[]={
  "BMS_ERROR_STATE_NONE",
  "BMS_ERROR_STATE_OVERVOLTAGE",
  "BMS_ERROR_STATE_UNDERVOLTAGE",
  "BMS_ERROR_STATE_TEMPERATURE_HIGH",
  "BMS_ERROR_STATE_TEMPERATURE_LOW",
  "BMS_ERROR_STATE_TEMPERATURE_CUT_OFF"
};

nimh_bms bms = {};
uint16_t (*get_module_count_Function)();
void (*relay_on_Function)(uint8_t pin);
void (*relay_off_Function)(uint8_t pin);
HardwareSerial * serail_debug;

static void update_realy_state();

//takes the functions wich calculates the modules count
void nimh_bms_init(uint16_t (*get_module_count_Ptr)(),
void (*relay_on)(uint8_t pin),
void (*relay_off)(uint8_t pin),
HardwareSerial * serial)
{
    get_module_count_Function = get_module_count_Ptr;
    relay_on_Function = relay_on;
    relay_off_Function = relay_off;
    serail_debug = serial;
    bms.relay_charger_state = RELAY_STATE_ON;
    bms.relay_load_state = RELAY_STATE_ON;
    update_realy_state();
}

void nimh_bms_check_state(uint16_t states, uint8_t current_module)
{
    bms.state = (states & 0xff);
    bms.error_state_temp = (states >> 8) & 0xf;
    bms.error_state_volt = (states >> 12) & 0xf;
    bms.module_count = get_module_count_Function();

    if(bms.error_state_temp == BMS_ERROR_STATE_TEMPERATURE_HIGH || bms.error_state_temp == BMS_ERROR_STATE_TEMPERATURE_CUT_OFF || bms.error_state_volt == BMS_ERROR_STATE_OVERVOLTAGE){
        bms.relay_charger_state = RELAY_STATE_OFF;
    }else if(bms.error_state_volt == BMS_ERROR_STATE_UNDERVOLTAGE){
        bms.relay_load_state == RELAY_STATE_OFF;
    }else{
        bms.relay_charger_state = RELAY_STATE_ON;
        bms.relay_load_state == RELAY_STATE_ON;
    }
    update_realy_state();

    #if defined(MY_DEBUG_NIMH)
        serail_debug->printf("\n");
        serail_debug->printf("Actual bms state: %s\n", bms_state_string[bms.state]);
        serail_debug->printf("Actual bms temperature state: %s\n", bms_errr_state_string[bms.error_state_temp]);
        serail_debug->printf("Actual bms voltage state: %s\n", bms_errr_state_string[bms.error_state_volt]);
        serail_debug->printf("Power relay: %u\n", bms.relay_charger_state);
        serail_debug->printf("Load relay: %u\n", bms.relay_load_state);
    #endif
}

nimh_bms nih_bms_get_struct(){
    return bms;
}

static void update_realy_state(){
    if(bms.relay_charger_state == RELAY_STATE_ON){
        relay_on_Function(RELAY_CHARGER_PIN);
    }else if(bms.relay_charger_state == RELAY_STATE_OFF){
        relay_off_Function(RELAY_CHARGER_PIN);
    }

    if(bms.relay_load_state == RELAY_STATE_ON){
        relay_on_Function(RELAY_LOAD_PIN);
    }else if(bms.relay_load_state == RELAY_STATE_OFF){
        relay_off_Function(RELAY_LOAD_PIN);
    }
}

