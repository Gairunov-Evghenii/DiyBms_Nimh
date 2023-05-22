#include "nimh_bms.h"
#include <stdlib.h>

nimh_bms bms = {};
uint16_t (*get_module_count_Function)();
void (*relay_on_Function)(uint8_t pin);
void (*relay_off_Function)(uint8_t pin);

static void apply_relay_state();
static void nimh_bms_sample_range_tick(uint8_t module);
static inline void nimh_bms_shift_samples(uint8_t module);
static inline uint8_t nimh_bms_dT_dt(uint8_t module);
static inline uint8_t is_temperature_increasing(uint8_t module);
static inline uint8_t is_voltage_increasing(uint8_t module);
static uint8_t nimh_bms_get_temperature_state(uint8_t module);
static uint8_t nimh_bms_get_voltage_state(uint8_t module);

// takes the functions wich calculates the modules count
void nimh_bms_init(uint16_t (*get_module_count_Ptr)(),
                    void (*relay_on)(uint8_t pin),
                    void (*relay_off)(uint8_t pin))
{
    get_module_count_Function = get_module_count_Ptr;
    relay_on_Function = relay_on;
    relay_off_Function = relay_off;
    for(uint8_t i = 0; i < MAX_MODULE_COUNT; i++){
        bms.cell[i].state = BMS_STATE_INITIALIZED;
    }
    apply_relay_state();
}

void nimh_bms_set_limits(
    uint16_t upper_voltage_limit,
    uint16_t lower_voltage_limit,
    int16_t upper_temperature_limit,
    int16_t lower_temperature_limit)
{
    bms.upper_voltage_limit = upper_voltage_limit;
    bms.lower_voltage_limit = lower_voltage_limit;
    bms.upper_temperature_limit = upper_temperature_limit;
    bms.lower_temperature_limit = lower_temperature_limit;
}

void nimh_bms_read_temperature(int16_t temperature, uint8_t module)
{
    bms.module_count = get_module_count_Function();
    if(module > MAX_MODULE_COUNT) {
        return;
    }
    bms.cell[module].timer += 1;

    if (bms.cell[module].timer % SAMPLE_RANGE == 1)
    {
        bms.cell[module].max_temp[CURREN] = temperature;
        bms.cell[module].min_temp[CURREN] = temperature;
    }

    if (temperature > bms.cell[module].max_temp[CURREN])
    {
        bms.cell[module].max_temp[CURREN] = temperature;
    }

    if (temperature < bms.cell[module].min_temp[CURREN])
    {
        bms.cell[module].min_temp[CURREN] = temperature;
    }
    
    nimh_bms_sample_range_tick(module);
}

void nimh_bms_read_voltage(uint16_t voltage, uint8_t module)
{
    bms.module_count = get_module_count_Function();
    if(module > MAX_MODULE_COUNT) {
        return;
    }
    bms.cell[module].timer += 1;

    if (bms.cell[module].timer % SAMPLE_RANGE == 1)
    {
        bms.cell[module].max_voltage[CURREN] = INT16_MIN;
        bms.cell[module].min_voltage[CURREN] = INT16_MAX;
    }

    if(voltage > bms.cell[module].max_voltage[CURREN]){
        bms.cell[module].max_voltage[CURREN] = voltage;
    }
    if(voltage < bms.cell[module].min_voltage[CURREN]){
        bms.cell[module].min_voltage[CURREN] = voltage;
    }
    nimh_bms_sample_range_tick(module);
}

static inline void nimh_bms_shift_samples(uint8_t module)
{
    bms.cell[module].max_temp[PREVIOUS] = bms.cell[module].max_temp[CURREN];
    bms.cell[module].min_temp[PREVIOUS] = bms.cell[module].min_temp[CURREN];
    bms.cell[module].max_voltage[PREVIOUS] = bms.cell[module].max_voltage[CURREN];
    bms.cell[module].min_voltage[PREVIOUS] = bms.cell[module].min_voltage[CURREN];
}

static void nimh_bms_sample_range_tick(uint8_t module)
{
    if (bms.cell[module].timer % SAMPLE_RANGE)
    {   // will enter the function every SAmple_range seconds
        return;
    }
    
    if(bms.cell[module].state != BMS_STATE_INITIALIZED){
    uint8_t last_state = bms.cell[module].error_state_temp;
    bms.cell[module].error_state_temp = nimh_bms_get_temperature_state(module);
    if(last_state != bms.cell[module].error_state_temp && (bms.cell[module].state != BMS_STATE_INITIALIZED)){
        if( bms.cell[module].error_state_temp == BMS_ERROR_STATE_TEMPERATURE_HIGH){
            bms.disable_charger += 1;
        }else {
            bms.disable_charger -= 1;
        }
    }

    last_state = bms.cell[module].error_state_volt;
    bms.cell[module].error_state_volt = nimh_bms_get_voltage_state(module);
    if(last_state != bms.cell[module].error_state_volt){
        if(bms.cell[module].error_state_volt == BMS_ERROR_STATE_OVERVOLTAGE){
            bms.disable_charger += 1;
        }else if (last_state == BMS_ERROR_STATE_OVERVOLTAGE){
            bms.disable_charger -= 1;
        }
        if(bms.cell[module].error_state_volt == BMS_ERROR_STATE_UNDERVOLTAGE){
            bms.disable_load += 1;
        }else if (last_state == BMS_ERROR_STATE_UNDERVOLTAGE){
            bms.disable_load -= 1;
        }
    }
    }
    
    switch (bms.cell[module].state)
    {
    case BMS_STATE_CRITICAL:
        if(bms.cell[module].error_state_temp == BMS_ERROR_STATE_NONE && bms.cell[module].error_state_volt == BMS_ERROR_STATE_NONE){
            if(is_temperature_increasing(module) || is_voltage_increasing(module)){
            }else{
                bms.cell[module].state = BMS_STATE_DISCHARGING;
            }
        }
    break;

    case BMS_STATE_INITIALIZED:
        if(is_temperature_increasing(module) || is_voltage_increasing(module)){
            bms.cell[module].state = BMS_STATE_CHARGING;
        }else{
            bms.cell[module].state = BMS_STATE_DISCHARGING;
        }
    break;

    case BMS_STATE_CHARGING:
        if(nimh_bms_dT_dt(module)){
           bms.cell[module].state = BMS_STATE_CHARGED;
        }
        if(is_temperature_increasing(module) || is_voltage_increasing(module)){
            bms.cell[module].state = BMS_STATE_CHARGING;
        }else{
            bms.cell[module].state = BMS_STATE_DISCHARGING;
        }
    break;

    case BMS_STATE_CHARGED:
        if(nimh_bms_dT_dt(module)){
           bms.cell[module].state = BMS_STATE_CRITICAL;
        }

        if((is_temperature_increasing(module) || is_voltage_increasing(module))){
        }else{
            bms.cell[module].state = BMS_STATE_DISCHARGING;
        }
    break;

    case BMS_STATE_DISCHARGING:
        if(is_temperature_increasing(module) || is_voltage_increasing(module)){
            bms.cell[module].state = BMS_STATE_CHARGING;
        }else{
            bms.cell[module].state = BMS_STATE_DISCHARGING;
        }
    break;
    
    default:
        break;
    }

    nimh_bms_shift_samples(module);
    apply_relay_state();
}

nimh_bms* nih_bms_get_struct(){
    return &bms;
}

static void apply_relay_state(){
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

static inline uint8_t nimh_bms_dT_dt(uint8_t module){
    return ((bms.cell[module].max_temp[CURREN] - bms.cell[module].min_temp[CURREN]) * 10 > (bms.cell[module].max_temp[PREVIOUS] - bms.cell[module].min_temp[PREVIOUS]) * 15) && ((bms.cell[module].max_temp[PREVIOUS] - bms.cell[module].min_temp[PREVIOUS]) > 0);
}

static inline uint8_t is_temperature_increasing(uint8_t module){
    return (bms.cell[module].max_temp[CURREN] > bms.cell[module].max_temp[PREVIOUS]) || (bms.cell[module].min_temp[CURREN] > bms.cell[module].min_temp[PREVIOUS]);
}

static inline uint8_t is_voltage_increasing(uint8_t module){
    return (bms.cell[module].max_voltage[CURREN] >= bms.cell[module].max_voltage[PREVIOUS]) || (bms.cell[module].max_voltage[CURREN] >= bms.cell[module].max_voltage[PREVIOUS]);
}

static uint8_t nimh_bms_get_temperature_state(uint8_t module)
{
    if (bms.cell[module].max_temp[CURREN] > bms.upper_temperature_limit)
    {
        return BMS_ERROR_STATE_TEMPERATURE_HIGH;
    }

    if (bms.cell[module].min_temp[CURREN] < bms.lower_temperature_limit)
    {
        return BMS_ERROR_STATE_TEMPERATURE_LOW;
    }

    return BMS_ERROR_STATE_NONE;
}

static uint8_t nimh_bms_get_voltage_state(uint8_t module)
{
    if (bms.cell[module].max_voltage[CURREN] < bms.lower_voltage_limit)
    {
        return BMS_ERROR_STATE_UNDERVOLTAGE;
    }

    if (bms.cell[module].min_voltage[CURREN] > bms.upper_voltage_limit)
    {
        return BMS_ERROR_STATE_OVERVOLTAGE;
    }

    return BMS_ERROR_STATE_NONE;
}

const char * nimh_decode_state(uint8_t state){
    switch (state){
        case BMS_STATE_CRITICAL:
        return "\"BMS_STATE_CRITICAL\"";

        case BMS_STATE_INITIALIZED:
        return "\"BMS_STATE_INITIALIZED\"";

        case BMS_STATE_CHARGING:
        return "\"BMS_STATE_CHARGING\"";

        case BMS_STATE_CHARGED:
        return "\"BMS_STATE_CHARGED\"";

        case BMS_STATE_DISCHARGING:
        return "\"BMS_STATE_DISCHARGING\"";

        default:
        return "\"error\"";
    }
}

const char * nimh_decode_error_state(uint8_t state){

    switch (state){
        case BMS_ERROR_STATE_NONE:
        return "\"BMS_ERROR_STATE_NONE\"";

        case BMS_ERROR_STATE_OVERVOLTAGE:
        return "\"BMS_ERROR_STATE_OVERVOLTAGE\"";

        case BMS_ERROR_STATE_UNDERVOLTAGE:
        return "\"BMS_ERROR_STATE_UNDERVOLTAGE\"";

        case BMS_ERROR_STATE_TEMPERATURE_HIGH:
        return "\"BMS_ERROR_STATE_TEMPERATURE_HIGH\"";

        case BMS_ERROR_STATE_TEMPERATURE_LOW:
        return "\"BMS_ERROR_STATE_TEMPERATURE_LOW\"";

        default:
        return "\"error\"";
    }
}
