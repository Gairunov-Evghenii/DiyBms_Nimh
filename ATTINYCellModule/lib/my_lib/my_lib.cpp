#include "my_lib.h"

#define VOLTAGE_sh                  (0)
#define CURRENT_sh                  (1)
#define TEMPERATURE_sh              (2)
#define RESISTANCE_sh               (3)
#define DIFF_VOLTAGE_sh             (4)
#define DIFF_CURRENT_sh             (5)
#define DIFF_TEMPERATURE_sh         (6)
#define DIFF_RESISTANCE_sh          (7)

#define ERR_MASK                    (0x03)
#define ERR_MASK_WIDTH              (2)

#define ERR_VOLTAGE                 (ERR_MASK<<(ERR_MASK_WIDTH*VOLTAGE_sh))
#define ERR_CURRENT                 (ERR_MASK<<(ERR_MASK_WIDTH*CURRENT_sh))
#define ERR_TEMPERATURE             (ERR_MASK<<(ERR_MASK_WIDTH*TEMPERATURE_sh))
#define ERR_RESISTANCE              (ERR_MASK<<(ERR_MASK_WIDTH*RESISTANCE_sh))
#define ERR_DIFF_VOLTAGE            (ERR_MASK<<(ERR_MASK_WIDTH*DIFF_VOLTAGE_sh))
#define ERR_DIFF_CURRENT            (ERR_MASK<<(ERR_MASK_WIDTH*DIFF_CURRENT_sh))
#define ERR_DIFF_TEMPERATURE        (ERR_MASK<<(ERR_MASK_WIDTH*DIFF_TEMPERATURE_sh))
#define ERR_DIFF_RESISTANCE         (ERR_MASK<<(ERR_MASK_WIDTH*DIFF_RESISTANCE_sh))

#define VOLTAGE_OVERHIGH            (0x0001)
#define VOLTADE_UNDERLOW            (0x0002)
#define CURRENT_OVERHIGH            (0x0004)
#define CURRENT_UNDERLOW            (0x0008)
#define TEMPERATURE_OVERHIGH        (0x0010)
#define TEMPERATURE_UNDERLOW        (0x0020)
#define RESISTANCE_OVERHIGH         (0x0040)
#define RESISTANCE_UNDERLOW         (0x0080)
#define DIFF_VOLTAGE_OVERHIGH       (0x0100)
#define DIFF_VOLTADE_UNDERLOW       (0x0200)
#define DIFF_CURRENT_OVERHIGH       (0x0400)
#define DIFF_CURRENT_UNDERLOW       (0x0800)
#define DIFF_TEMPERATURE_OVERHIGH   (0x1000)
#define DIFF_TEMPERATURE_UNDERLOW   (0x2000)
#define DIFF_RESISTANCE_OVERHIGH    (0x4000)
#define DIFF_RESISTANCE_UNDERLOW    (0x8000)

typedef enum ML_ERR_enum
{
    OK          = 0,
    OVERHIGH    = 1,
    UNDERLOW    = 2,
}   ML_ERROR;

typedef union INT16_union
{
    int16_t s;
    uint16_t u;
} INT16_UNION;

void my_lib_bat::set_limites(MY_LIB_limites* limites, uint16_t mask)
{
    if(mask & MAX_VOLTAGE_MASK) lim_voltage[0]=limites->lim_voltage[0];
    if(mask & MIN_VOLTAGE_MASK) lim_voltage[1]=limites->lim_voltage[1];
    if(mask & MAX_CURRENT_MASK) lim_current[0]=limites->lim_current[0];
    if(mask & MIN_CURRENT_MASK) lim_current[1]=limites->lim_current[1];
    if(mask & MAX_REZISTANCE_MASK) lim_resistance[0]=limites->lim_resistance[0];
    if(mask & MIN_REZISTANCE_MASK) lim_resistance[1]=limites->lim_resistance[1];
    if(mask & MAX_TEMPERATURE_MASK) lim_temperature[0]=limites->lim_temperature[0];
    if(mask & MIN_TEMPERATURE_MASK) lim_temperature[1]=limites->lim_temperature[1];
    if(mask & MAX_VOLTAGE_CHANGE_MASK) lim_diff_voltage[0]=limites->lim_diff_voltage[0];
    if(mask & MIN_VOLTAGE_CHANGE_MASK) lim_diff_voltage[1]=limites->lim_diff_voltage[1];
    if(mask & MAX_CURRENT_CHANGE_MASK) lim_diff_current[0]=limites->lim_diff_current[0];
    if(mask & MIN_CURRENT_CHANGE_MASK) lim_diff_current[1]=limites->lim_diff_current[1];
    if(mask & MAX_REZISTANCE_CHANGE_MASK) lim_diff_resistance[0]=limites->lim_diff_resistance[0];
    if(mask & MIN_REZISTANCE_CHANGE_MASK) lim_diff_resistance[1]=limites->lim_diff_resistance[1];
    if(mask & MAX_TEMPERATURE_CHANGE_MASK) lim_diff_temperature[0]=limites->lim_diff_temperature[0];
    if(mask & MIN_TEMPERATURE_CHANGE_MASK) lim_diff_temperature[1]=limites->lim_diff_temperature[1];
}

void my_lib_bat::set_limites(uint16_t *vect, uint16_t mask)
{
    LIMITES_UNION limun;
    limun.vector = vect;
    set_limites(limun.structure,mask);
}

void my_lib_bat::get_limites(MY_LIB_limites* lim)
{
    lim->lim_voltage[0] = lim_voltage[0];
    lim->lim_voltage[1] = lim_voltage[1];
    lim->lim_current[0] = lim_current[0];
    lim->lim_current[1] = lim_current[1];
    lim->lim_resistance[0] = lim_resistance[0];
    lim->lim_resistance[1] = lim_resistance[1];
    lim->lim_temperature[0] = lim_temperature[0];
    lim->lim_temperature[1] = lim_temperature[1];
    lim->lim_diff_voltage[0] = lim_diff_voltage[0];
    lim->lim_diff_voltage[1] = lim_diff_voltage[1];
    lim->lim_diff_current[0] = lim_diff_current[0];
    lim->lim_diff_current[1] = lim_diff_current[1];
    lim->lim_diff_resistance[0] = lim_diff_resistance[0];
    lim->lim_diff_resistance[1] = lim_diff_resistance[1];
    lim->lim_diff_temperature[0] = lim_diff_temperature[0];
    lim->lim_diff_temperature[1] = lim_diff_temperature[1];
}

void my_lib_bat::get_params(uint16_t *buf)
{
    INT16_UNION temp;
    buf[0] = voltage[CUR];
    temp.s = current[CUR]; buf[1] = temp.u;
    buf[2] = resistance[CUR];
    temp.s = temperature[CUR]; buf[3] = temp.u;
    temp.s = diff_voltage; buf[4] = temp.u;
    temp.s = diff_current; buf[5] = temp.u;
    temp.s = diff_resistance; buf[6] = temp.u;
    temp.s = diff_temperature; buf[7] = temp.u;

    buf[8] = error_state;
    buf[9] = error_save;
}

void my_lib_bat::calc()
{
    uint16_t dt = time[CUR] - time[PRE];
    diff_voltage = (float)(voltage[CUR]-voltage[PRE])*1000.0/((float)(dt));
    diff_current = (float)(current[CUR]-current[PRE])*1000.0/((float)(dt));
    diff_resistance = (float)(resistance[CUR]-resistance[PRE])*1000.0/((float)(dt));
    diff_temperature = (float)(temperature[CUR]-temperature[PRE])*1000.0/((float)(dt));

    error_state = 0;
    if(voltage[CUR] > lim_voltage[MAX]) error_state |= VOLTAGE_OVERHIGH;
    if(voltage[CUR] < lim_voltage[MIN]) error_state |= VOLTADE_UNDERLOW;
    if(current[CUR] > lim_current[MAX]) error_state |= CURRENT_OVERHIGH;
    if(current[CUR] < lim_current[MAX]) error_state |= CURRENT_UNDERLOW;
    if(resistance[CUR] > lim_resistance[MAX]) error_state |= RESISTANCE_OVERHIGH;
    if(resistance[CUR] < lim_resistance[MIN]) error_state |= RESISTANCE_UNDERLOW;
    if(temperature[CUR] > lim_temperature[MAX]) error_state |= TEMPERATURE_OVERHIGH;
    if(temperature[CUR] < lim_temperature[MIN]) error_state |= TEMPERATURE_UNDERLOW;
    if(diff_voltage > lim_diff_voltage[MAX]) error_state |= DIFF_VOLTAGE_OVERHIGH;
    if(diff_voltage < lim_diff_voltage[MIN]) error_state |= DIFF_VOLTADE_UNDERLOW;
    if(diff_current > lim_diff_current[MAX]) error_state |= DIFF_CURRENT_OVERHIGH;
    if(diff_current < lim_diff_current[MAX]) error_state |= DIFF_CURRENT_UNDERLOW;
    if(diff_resistance > lim_diff_resistance[MAX]) error_state |= DIFF_RESISTANCE_OVERHIGH;
    if(diff_resistance < lim_diff_resistance[MIN]) error_state |= DIFF_RESISTANCE_UNDERLOW;
    if(diff_temperature > lim_diff_temperature[MAX]) error_state |= DIFF_TEMPERATURE_OVERHIGH;
    if(diff_temperature < lim_diff_temperature[MIN]) error_state |= DIFF_TEMPERATURE_UNDERLOW;
    error_save |= error_state;
}

my_lib_bat bat_my_lib;
MY_LIB_limites ML_limites;