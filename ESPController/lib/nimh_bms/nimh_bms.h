#ifndef NIHM_BMS_H_
#define NIHM_BMS_H_

#include <stdint.h>

#define relay_charger_state_PIN 0
#define relay_load_state_PIN 1

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
typedef struct nimh_bms{
    uint8_t state;
    uint8_t error_state_temp;
    uint8_t error_state_volt;
    uint8_t module_count;
    uint8_t relay_charger_state;
    uint8_t relay_load_state;
}nimh_bms;

void nimh_bms_init(uint16_t (*get_module_count_Ptr)(),
void (*change_relay_state_Ptr)(uint8_t, uint8_t)
);
void nimh_bms_check_state(uint16_t states, uint8_t current_module);
nimh_bms nih_bms_get_struct();

#endif