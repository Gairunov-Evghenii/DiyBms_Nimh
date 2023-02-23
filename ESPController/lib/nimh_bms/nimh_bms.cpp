#include "nimh_bms.h"
#include <stdlib.h>

nimh_bms bms = {};
uint16_t (*get_module_count_Function)();
void (*relay_on_Function)(uint8_t pin);
void (*relay_off_Function)(uint8_t pin);

static void update_realy_state();
static inline uint8_t get_state(uint16_t states);
static inline uint8_t get_temp_state(uint16_t states);
static inline uint8_t get_volt_state(uint16_t states);

// takes the functions wich calculates the modules count
void nimh_bms_init(uint16_t (*get_module_count_Ptr)(),
void (*relay_on)(uint8_t pin),
void (*relay_off)(uint8_t pin))
{
    get_module_count_Function = get_module_count_Ptr;
    relay_on_Function = relay_on;
    relay_off_Function = relay_off;
    update_realy_state();
}

void nimh_bms_check_state(uint16_t states, uint8_t current_module)
{
    bms.module_count = get_module_count_Function();
    if(current_module > MAX_MODULE_COUNT) {
        return;
    }

    uint8_t temp_state = get_temp_state(states);
    uint8_t volt_state = get_volt_state(states);

    if(get_temp_state(bms.states[current_module]) != temp_state){
        if( temp_state == BMS_ERROR_STATE_TEMPERATURE_HIGH || 
            temp_state == BMS_ERROR_STATE_TEMPERATURE_CUT_OFF){
            bms.disable_charger += 1;
        }else {
            bms.disable_charger -= 1;
        }
    }

    if(get_volt_state(bms.states[current_module]) != volt_state){
        if(volt_state == BMS_ERROR_STATE_OVERVOLTAGE){
            bms.disable_charger += 1;
        }else if (get_volt_state(bms.states[current_module]) == BMS_ERROR_STATE_OVERVOLTAGE){
            bms.disable_charger -= 1;
        }
        if(volt_state == BMS_ERROR_STATE_UNDERVOLTAGE){
            bms.disable_load += 1;
        }else if (get_volt_state(bms.states[current_module]) == BMS_ERROR_STATE_UNDERVOLTAGE){
            bms.disable_load -= 1;
        }
    }
    update_realy_state();
    bms.states[current_module] = states;
}

nimh_bms nih_bms_get_struct(){
    return bms;
}

static void update_realy_state(){
    if(bms.disable_charger == 0){
        relay_on_Function(RELAY_CHARGER_PIN);
    }else if(bms.disable_charger > 0){
        relay_off_Function(RELAY_CHARGER_PIN);
    }

    if(bms.disable_load == 0){
        relay_on_Function(RELAY_LOAD_PIN);
    }else if(bms.disable_load > 0){
        relay_off_Function(RELAY_LOAD_PIN);
    }
}

static inline uint8_t get_state(uint16_t states){
    return (states & 0xff);
}

static inline uint8_t get_temp_state(uint16_t states){
    return (states >> 8) & 0xf;
}

static inline uint8_t get_volt_state(uint16_t states){
    return (states >> 12) & 0xf;
}