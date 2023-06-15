#ifndef NIMH_BMS_H_
#define NIMH_BMS_H_

#include <stdint.h>

#define READ_INTERVAL 1
 //seconds
#define CUT_OFF_TIMER (5400 / READ_INTERVAL)

enum BMS_STATE {
    BMS_STATE_INITIALIZED = 0,
    BMS_STATE_CHARGING,
    BMS_STATE_CHARGED,
    BMS_STATE_DISCHARGING,

};

enum BMS_ERROR_STATE {
    BMS_ERROR_STATE_NONE = 0,
    BMS_ERROR_STATE_OVERVOLTAGE,
    BMS_ERROR_STATE_UNDERVOLTAGE,
    BMS_ERROR_STATE_TEMPERATURE_HIGH,
    BMS_ERROR_STATE_TEMPERATURE_LOW,
};

enum SAMPLE{
    CURREN = 0,
    PREVIOUS
};

#define SAMPLE_RANGE 32
typedef struct nimh_bms{
    uint8_t state;
    uint8_t error_state_volt;
    int16_t max_temp[2];
    int16_t min_temp[2];
    int16_t max_voltage[2];
    int16_t min_voltage[2];
    uint16_t timer;
    uint16_t upper_voltage_limit;
}nimh_bms;

extern nimh_bms bms;

void nimh_bms_init(uint16_t upper_voltage_limit);
void nimh_bms_read_temperature(uint16_t temp);
void nimh_bms_read_voltage(uint16_t voltage);
void nimh_bms_sample_range_tick();
uint16_t nimh_bms_check_state();
int16_t nimh_bms_check_temperature_state();
uint8_t nimh_bms_check_bypass();

#endif