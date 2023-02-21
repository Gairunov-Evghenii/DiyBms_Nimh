#include "nimh_bms.h"
#include <stdlib.h>

nimh_bms bms = {};
uint16_t (*get_module_count_Function)();
void (*change_relay_state_Function)(uint8_t, uint8_t);

enum RelayState : uint8_t
{
  RELAY_ON = 0xFF,
  RELAY_OFF = 0x99,
  RELAY_X = 0x00
};

//takes the functions wich calculates the modules count
void nimh_bms_init(uint16_t (*get_module_count_Ptr)(),
void (*change_relay_state_Ptr)(uint8_t, uint8_t)
){
    get_module_count_Function = get_module_count_Ptr;
    change_relay_state_Function = change_relay_state_Ptr;
    bms.relay_charger_state = RELAY_ON;
    bms.relay_load_state = RELAY_ON;
    change_relay_state_Function(relay_charger_state_PIN, bms.relay_charger_state);
    change_relay_state_Function(relay_load_state_PIN, bms.relay_load_state);
}

void nimh_bms_check_state(uint16_t states, uint8_t current_module)
{
    bms.state = (states & 0xff);
    bms.error_state_temp = (states >> 8) & 0xf;
    bms.error_state_volt = (states >> 12) & 0xf;
    bms.module_count = get_module_count_Function();
}

nimh_bms nih_bms_get_struct(){
    return bms;
}