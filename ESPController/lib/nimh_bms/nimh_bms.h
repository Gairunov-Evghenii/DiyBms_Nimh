#ifndef NIHM_BMS_H_
#define NIHM_BMS_H_

#include <stdint.h>
#include <stddef.h>

#define RELAY_CHARGER_PIN 0
#define RELAY_LOAD_PIN 1

#define READ_INTERVAL 1
 //seconds

enum BMS_STATE {
    BMS_STATE_CRITICAL = 0,
    BMS_STATE_INITIALIZED,
    BMS_STATE_CHARGING,
    BMS_STATE_CHARGED,
    BMS_STATE_DISCHARGING,

};

enum BMS_ERROR_STATE {
    BMS_ERROR_STATE_NONE = 0,
    BMS_ERROR_STATE_OVERVOLTAGE,
    BMS_ERROR_STATE_UNDERVOLTAGE,
    BMS_ERROR_STATE_TEMPERATURE_HIGH,
    BMS_ERROR_STATE_TEMPERATURE_LOW
};

enum SAMPLE{
    CURREN = 0,
    PREVIOUS
};

#define SAMPLE_RANGE 64
typedef struct nimh_bms_cell{
    uint8_t state;
    uint8_t error_state_temp;
    uint8_t error_state_volt;
    int16_t max_temp[2];
    int16_t min_temp[2];
    int16_t max_voltage[2];
    int16_t min_voltage[2];
    uint16_t timer;
}nimh_bms_cell;

#define MAX_MODULE_COUNT 100
typedef struct nimh_bms{
    nimh_bms_cell cell[MAX_MODULE_COUNT];
    uint8_t module_count;
    uint8_t disable_charger;
    uint8_t disable_load;
    uint16_t upper_voltage_limit;
    uint16_t lower_voltage_limit;
    int16_t upper_temperature_limit;
    int16_t lower_temperature_limit;
}nimh_bms;

void nimh_bms_init(uint16_t (*get_module_count_Ptr)(),
void (*relay_on)(uint8_t pin),
void (*relay_off)(uint8_t pin)
);

void nimh_bms_set_limits(
    uint16_t upper_voltage_limit,
    uint16_t lower_voltage_limit,
    int16_t lower_temperature_limit,
    int16_t upper_temperature_limit);

void nimh_bms_read_temperature(int16_t temperature, uint8_t module);
void nimh_bms_read_voltage(uint16_t voltage, uint8_t module);

nimh_bms* nih_bms_get_struct();

const char * nimh_decode_state(uint8_t state);
const char * nimh_decode_error_state(uint8_t state);

#endif