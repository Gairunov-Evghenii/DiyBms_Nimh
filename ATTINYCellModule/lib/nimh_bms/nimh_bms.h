#ifndef NIMH_BMS_H_
#define NIMH_BMS_H_

#include <stdint.h>

#define NUMBER_OF_CELLS 6
#if defined(NUMBER_OF_CELLS) && NUMBER_OF_CELLS <= 6
    #define VOLTAGE_LOW (1000 * NUMBER_OF_CELLS)
#else
    #define VOLTAGE_LOW (1150*(NUMBER_OF_CELLS-1) - 200)
#endif

#define VOLTAGE_HIGH (1400 * NUMBER_OF_CELLS)

#define TEMPERATURE_CUT_OFF 550
#define TEMPERATURE_LOW 100
#define TEMPERATURE_HIGH 400

#define READ_INTERVAL 1
 //seconds
#define CUT_OFF_TIMER (3600 / READ_INTERVAL)

enum BMS_STATE {
    BMS_STATE_UNKNOWN = 0,
    BMS_STATE_INITIALIZED = 1,
    BMS_STATE_CHARGING = 2,
    BMS_STATE_CHARGED = 4,
    BMS_STATE_OVERVOLTAGE = 8,
    BMS_STATE_UNDERVOLTAGE = 16,
    BMS_STATE_TEMPERATURE_HIGH = 32,
    BMS_STATE_TEMPERATURE_LOW = 64,
    BMS_STATE_TEMPERATURE_CUT_OFF = 128,
};

enum SAMPLE{
    CURREN = 0,
    PREVIOUS
};

#define SAMPLE_RANGE 4
typedef struct nimh_bms{
    uint16_t state;
    int16_t temperature;
    uint16_t voltage_mV;
    int16_t max_temp[2];
    int16_t min_temp[2];
    int16_t temp_slope;
    int16_t max_voltage[2];
    int16_t min_voltage[2];
    uint16_t timer;
}nimh_bms;

void nimh_bms_init();
void nimh_bms_read_temperature(uint16_t temp);
void nimh_bms_read_voltage(uint16_t voltage);
void nimh_bms_sample_range_tick();
uint16_t nimh_bms_check_state();
int16_t nimh_bms_check_temperature_slope();

#endif