#ifndef MY_LIB
#define MY_LIB
#include "stdint.h"

#define MAX_VOLTAGE_MASK            (uint16_t)(0x0001)
#define MIN_VOLTAGE_MASK            (uint16_t)(0x0002)
#define MAX_INT_TEMP_MASK           (uint16_t)(0x0004)
#define MIN_INT_TEMP_MASK           (uint16_t)(0x0008)
#define MAX_REZISTANCE_MASK         (uint16_t)(0x0010)
#define MIN_REZISTANCE_MASK         (uint16_t)(0x0020)
#define MAX_EXT_TEMP_MASK           (uint16_t)(0x0040)
#define MIN_EXT_TEMP_MASK           (uint16_t)(0x0080)
#define MAX_VOLTAGE_CHANGE_MASK     (uint16_t)(0x0100)
#define MIN_VOLTAGE_CHANGE_MASK     (uint16_t)(0x0200)
#define MAX_INT_TEMP_CHANGE_MASK    (uint16_t)(0x0400)
#define MIN_INT_TEMP_CHANGE_MASK    (uint16_t)(0x0800)
#define MAX_REZISTANCE_CHANGE_MASK  (uint16_t)(0x1000)
#define MIN_REZISTANCE_CHANGE_MASK  (uint16_t)(0x2000)
#define MAX_EXT_TEMP_CHANGE_MASK    (uint16_t)(0x4000)
#define MIN_EXT_TEMP_CHANGE_MASK    (uint16_t)(0x8000)

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
    int16_t     lim_int_temp[2];
    uint16_t    lim_resistance[2];
    int16_t     lim_ext_temp[2];
    int16_t     lim_diff_voltage[2];
    int16_t     lim_diff_int_temp[2];
    int16_t     lim_diff_resistance[2];
    int16_t     lim_diff_ext_temp[2];
};

extern volatile MY_LIB_limites ML_limites;

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
    volatile uint64_t    time[2];                     //временные метки в миллисекундах: текущая и предыдущая
    volatile uint16_t    dt;                          //дифференциал времени
    volatile uint16_t    voltage[2];                  //уровни напряжения в децивольтах: текущий и предыдущий
    volatile int16_t     int_temp[2];                 //температура демпфера в дециградусах: текущая и предыдущая
    volatile uint16_t    resistance[2];               //сопротивление: текущее и предыдущее
    volatile int16_t     ext_temp[2];                 //температура батареи в дециградусах: текущая и предыдущая
    volatile int16_t     diff_voltage;                //производная напряжения
    volatile int16_t     diff_int_temp;               //производная температуры демпфера
    volatile int16_t     diff_resistance;             //производная сопротивления
    volatile int16_t     diff_ext_temp;               //производная температуры батареи
    volatile uint16_t    lim_voltage[2];              //пределы допустимости напряжения: верхний и нижний
    volatile int16_t     lim_int_temp[2];             //пределы допустимости температуры демпфера: верхний и нижний
    volatile uint16_t    lim_resistance[2];           //пределы допустимости сопротивления: верхний и нижний
    volatile int16_t     lim_ext_temp[2];             //пределы допустимости температуры батареи: верхний и нижний
    volatile int16_t     lim_diff_voltage[2];         //пределы допустимости производной напряжения: верхний и нижний
    volatile int16_t     lim_diff_int_temp[2];        //пределы допустимости производной температуры демпфера: верхний и нижний
    volatile int16_t     lim_diff_resistance[2];      //пределы допустимости производной сопротивления: верхний и нижний
    volatile int16_t     lim_diff_ext_temp[2];        //пределы допустимости производной температуры батареи: верхний и нижний
    volatile uint16_t    error_state;                 //состояние ошибки по последним измерениям
    volatile uint16_t    error_save;                  //все возникшие ошибки с момента последнего сброса состояния ошибки
    volatile uint8_t     state;                       //состояние элементов управления (ключи, светодиоды и тд)
    volatile uint8_t     charge_level;                //уровень текущего заряда

  public:
    void set_limites(MY_LIB_limites*, uint16_t);
    void set_limites(uint16_t*, uint16_t);
    inline void set_time(uint64_t t) {time[PRE] = time[CUR]; time[CUR] = t; dt = time[CUR] - time[PRE];};
    inline void set_voltage(uint16_t v) {voltage[PRE] = voltage[CUR]; voltage[CUR] = v;};
    inline void set_int_temp(int16_t c) {int_temp[PRE] = int_temp[CUR]; int_temp[CUR] = c;};
    inline void set_resistance(uint16_t r) {resistance[PRE] = resistance[CUR]; resistance[CUR] = r;};
    inline void set_ext_temp(int16_t t) {ext_temp[PRE] = ext_temp[CUR]; ext_temp[CUR] = t;};
    void calc();
    inline void error_clear() {error_save = 0;};
    inline void set_capacity(uint16_t value) {capacity_nominal = value;};
    inline uint16_t get_real_capacity() {return capacity_real;};
    inline uint16_t get_nominal_capacity() {return capacity_nominal;};
    inline uint16_t get_time() {return time[CUR];};
    inline uint16_t get_voltage() {return voltage[CUR];};
    inline int16_t get_int_temp() {return int_temp[CUR];};
    inline uint16_t get_resistance() {return resistance[CUR];};
    inline int16_t get_ext_temp() {return ext_temp[CUR];};
    inline int16_t get_diff_voltage() {return diff_voltage;};
    inline int16_t get_diff_int_temp() {return diff_int_temp;};
    inline int16_t get_diff_resistance() {return diff_resistance;};
    inline int16_t get_diff_ext_temp() {return diff_ext_temp;};
    inline uint16_t get_error_state() {return error_state;};
    inline uint16_t get_error_save() {return error_save;};
    inline uint8_t get_state() {return state;};
    inline uint8_t get_charge_level() {return charge_level;};
    void get_limites(MY_LIB_limites*);
    void get_params(uint16_t*);
    volatile bool        need_reread;                 //
};

extern my_lib_bat bat_my_lib;
#endif