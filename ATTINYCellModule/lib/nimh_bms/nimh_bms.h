#ifndef NIMH_BMS_H_
#define NIMH_BMS_H_

#include <stdint.h>

#define NUMBER_OF_CELLS 6
#if defined(NUMBER_OF_CELLS) && NUMBER_OF_CELLS <= 6
    #define VOLTAGE_LOW (1000 * NUMBER_OF_CELLS)
#else
    #define VOLTAGE_LOW (1150*(NUMBER_OF_CELLS-1) - 200)
#endif

#define VOLTAGE_HIGH (1465 * NUMBER_OF_CELLS)

#define TEMPERATURE_CUT_OFF 400
#define TEMPERATURE_LOW 100
#define TEMPERATURE_HIGH 350
#define READ_INTERVAL 1
 //seconds
#define CUT_OFF_TIMER (5400 / READ_INTERVAL)

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
    BMS_ERROR_STATE_TEMPERATURE_LOW,
    BMS_ERROR_STATE_TEMPERATURE_CUT_OFF,
};

enum SAMPLE{
    CURREN = 0,
    PREVIOUS
};

#define SAMPLE_RANGE 32
typedef struct nimh_bms{
    uint8_t state;
    uint8_t error_state_temp;
    uint8_t error_state_volt;
    int16_t max_temp[2];
    int16_t min_temp[2];
    int16_t max_voltage[2];
    int16_t min_voltage[2];
    uint16_t timer;
}nimh_bms;

void nimh_bms_init();
void nimh_bms_read_temperature(uint16_t temp);
void nimh_bms_read_voltage(uint16_t voltage);
void nimh_bms_sample_range_tick();
uint16_t nimh_bms_check_state();
int16_t nimh_bms_check_temperature_state();
uint8_t nimh_bms_check_bypass();

#endif