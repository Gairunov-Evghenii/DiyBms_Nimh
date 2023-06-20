#ifndef MY_LIB
#define MY_LIB
#include "stdint.h"

#define MAX_VOLTAGE_MASK            (uint16_t)(0x0001)
#define MIN_VOLTAGE_MASK            (uint16_t)(0x0002)
#define MAX_CURRENT_MASK            (uint16_t)(0x0004)
#define MIN_CURRENT_MASK            (uint16_t)(0x0008)
#define MAX_REZISTANCE_MASK         (uint16_t)(0x0010)
#define MIN_REZISTANCE_MASK         (uint16_t)(0x0020)
#define MAX_TEMPERATURE_MASK        (uint16_t)(0x0040)
#define MIN_TEMPERATURE_MASK        (uint16_t)(0x0080)
#define MAX_VOLTAGE_CHANGE_MASK     (uint16_t)(0x0100)
#define MIN_VOLTAGE_CHANGE_MASK     (uint16_t)(0x0200)
#define MAX_CURRENT_CHANGE_MASK     (uint16_t)(0x0400)
#define MIN_CURRENT_CHANGE_MASK     (uint16_t)(0x0800)
#define MAX_REZISTANCE_CHANGE_MASK  (uint16_t)(0x1000)
#define MIN_REZISTANCE_CHANGE_MASK  (uint16_t)(0x2000)
#define MAX_TEMPERATURE_CHANGE_MASK (uint16_t)(0x4000)
#define MIN_TEMPERATURE_CHANGE_MASK (uint16_t)(0x8000)

#define INIT_LIMITES                {{0xFFFF,0x0000},{0x7FFF,0x8000},{0xFFFF,0x0000},{0x7FFF,0x8000},{0x7FFF,0x8000},{0x7FFF,0x8000},{0x7FFF,0x8000},{0x7FFF,0x8000}}

enum 
{
  CUR=0,
  PRE=1
};

enum
{
  MAX=0,
  MIN=1
};

struct MY_LIB_limites
{
    uint16_t    lim_voltage[2];
    int16_t     lim_current[2];
    uint16_t    lim_resistance[2];
    int16_t     lim_temperature[2];
    int16_t     lim_diff_voltage[2];
    int16_t     lim_diff_current[2];
    int16_t     lim_diff_resistance[2];
    int16_t     lim_diff_temperature[2];
};

extern MY_LIB_limites ML_limites;

union LIMITES_UNION
{
    uint16_t* vector;
    MY_LIB_limites* structure;
};

class my_lib_bat
{
  private:
    volatile uint16_t    capacity_nominal;            //номинальная ёмкость, устанавливается извне
    volatile uint16_t    capacity_real;               //реальная ёмкость, должна рассчитываться внутри
    volatile uint16_t    time[2];                     //временные метки в миллисекундах: текущая и предыдущая
    volatile uint16_t    voltage[2];                  //уровни напряжения в децивольтах: текущий и предыдущий
    volatile int16_t     current[2];                  //величина тока: текущая и предыдущая
    volatile uint16_t    resistance[2];               //сопротивление: текущее и предыдущее
    volatile int16_t     temperature[2];              //температура в дециградусах: текущая и предыдущая
    volatile int16_t     diff_voltage;                //производная напряжения
    volatile int16_t     diff_current;                //производная тока
    volatile int16_t     diff_resistance;             //производная сопротивления
    volatile int16_t     diff_temperature;            //производная температуры
    volatile uint16_t    lim_voltage[2];              //пределы допустимости по напряжению: верхний и нижний
    volatile int16_t     lim_current[2];              //пределы допустимости по току: верхний и нижний
    volatile uint16_t    lim_resistance[2];           //пределы допустимости по сопротивлению: верхний и нижний
    volatile int16_t     lim_temperature[2];          //пределы допустимости по температуре: верхний и нижний
    volatile int16_t     lim_diff_voltage[2];         //пределы допустимости по производной напряжения: верхний и нижний
    volatile int16_t     lim_diff_current[2];         //пределы допустимости по производной тока: верхний и нижний
    volatile int16_t     lim_diff_resistance[2];      //пределы допустимости по производной сопротивления: верхний и нижний
    volatile int16_t     lim_diff_temperature[2];     //пределы допустимости по производной температуры: верхний и нижний
    volatile uint16_t    error_state;                 //состояние ошибки по последним измерениям
    volatile uint16_t    error_save;                  //все возникшие ошибки с момента последнего сброса состояния ошибки
    volatile uint8_t     state;                       //состояние элементов управления (ключи, светодиоды и тд)
    volatile uint8_t     charge_level;                //уровень текущего заряда

  public:
    void set_limites(MY_LIB_limites*, uint16_t);
    void set_limites(uint16_t*, uint16_t);
    inline void set_time(uint16_t t) {time[PRE] = time[CUR]; time[CUR] = t;};
    inline void set_voltage(uint16_t v) {voltage[PRE] = voltage[CUR]; voltage[CUR] = v;};
    inline void set_current(int16_t c) {current[PRE] = current[CUR]; current[CUR] = c;};
    inline void set_resistance(uint16_t r) {resistance[PRE] = resistance[CUR]; resistance[CUR] = r;};
    inline void set_temperature(int16_t t) {temperature[PRE] = temperature[CUR]; temperature[CUR] = t;};
    void calc();
    inline void error_clear() {error_save = 0;};
    inline void set_capacity(uint16_t value) {capacity_nominal = value;};
    inline uint16_t get_real_capacity() {return capacity_real;};
    inline uint16_t get_nominal_capacity() {return capacity_nominal;};
    inline uint16_t get_time() {return time[CUR];};
    inline uint16_t get_voltage() {return voltage[CUR];};
    inline int16_t get_current() {return current[CUR];};
    inline uint16_t get_resistance() {return resistance[CUR];};
    inline int16_t get_temperature() {return temperature[CUR];};
    inline int16_t get_diff_voltage() {return diff_voltage;};
    inline int16_t get_diff_current() {return diff_current;};
    inline int16_t get_diff_resistance() {return diff_resistance;};
    inline int16_t get_diff_temperature() {return diff_temperature;};
    inline uint16_t get_error_state() {return error_state;};
    inline uint16_t get_error_save() {return error_save;};
    inline uint8_t get_state() {return state;};
    inline uint8_t get_charge_level() {return charge_level;};
    void get_limites(MY_LIB_limites*);
    void get_params(uint16_t*);
};

extern my_lib_bat bat_my_lib;
#endif