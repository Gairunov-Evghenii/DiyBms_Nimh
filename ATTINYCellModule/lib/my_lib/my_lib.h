#ifndef MY_LIB
#define MY_LIB
#include "stdint.h"

class my_lib_bat
{
  private:
    volatile uint16_t    capacity_nominal;
    volatile uint16_t    capacity_real;
    volatile uint16_t    time[2];
    volatile uint16_t    voltage[2];
    volatile int16_t     current[2];
    volatile uint16_t    resistance[2];
    volatile int16_t     temperature[2];
    volatile int16_t     diff_voltage;
    volatile int16_t     diff_current;
    volatile int16_t     diff_resistance;
    volatile int16_t     diff_temperature;
    volatile uint16_t    lim_voltage[2];
    volatile int16_t     lim_current[2];
    volatile uint16_t    lim_resistance[2];
    volatile int16_t     lim_temperature[2];
    volatile int16_t     lim_diff_voltage[2];
    volatile int16_t     lim_diff_current[2];
    volatile int16_t     lim_diff_resistance[2];
    volatile int16_t     lim_diff_temperature[2];
    volatile uint16_t    error_state;
    volatile uint8_t     state;
    volatile uint8_t     charge_level;


    public:
    inline void init();
    inline void calc();
    inline uint16_t get_real_capacity() {return capacity_real;};
    inline uint16_t get_nominal_capacity() {return capacity_nominal;};
    inline uint16_t get_voltage() {return voltage[0];};
    inline int16_t get_current() {return current[0];};
    inline uint16_t get_resistance() {return resistance[0];};
    inline int16_t get_temperature() {return temperature[0];};
    inline int16_t get_diff_voltage() {return diff_voltage;};
    inline int16_t get_diff_current() {return diff_current;};
    inline int16_t get_diff_resistance() {return diff_resistance;};
    inline int16_t get_diff_temperature() {return diff_temperature;};
    inline uint16_t get_error_state() {return error_state;};
    inline uint8_t get_state() {return state;};
    inline uint8_t get_charge_level() {return charge_level;};
};
#endif