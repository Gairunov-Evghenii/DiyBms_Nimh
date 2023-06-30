#include "my_lib.h"

#define VOLTAGE_sh                  (0)
#define INT_TEMP_sh                 (1)
#define EXT_TEMP_sh                 (2)
#define RESISTANCE_sh               (3)
#define DIFF_VOLTAGE_sh             (4)
#define DIFF_INT_TEMP_sh            (5)
#define DIFF_EXT_TEMP_sh            (6)
#define DIFF_RESISTANCE_sh          (7)

#define ERR_MASK                    (0x03)
#define ERR_MASK_WIDTH              (2)

#define ERR_VOLTAGE                 (ERR_MASK<<(ERR_MASK_WIDTH*VOLTAGE_sh))
#define ERR_INT_TEMP                (ERR_MASK<<(ERR_MASK_WIDTH*INT_TEMP_sh))
#define ERR_EXT_TEMP                (ERR_MASK<<(ERR_MASK_WIDTH*EXT_TEMP_sh))
#define ERR_RESISTANCE              (ERR_MASK<<(ERR_MASK_WIDTH*RESISTANCE_sh))
#define ERR_DIFF_VOLTAGE            (ERR_MASK<<(ERR_MASK_WIDTH*DIFF_VOLTAGE_sh))
#define ERR_DIFF_INT_TEMP           (ERR_MASK<<(ERR_MASK_WIDTH*DIFF_INT_TEMP_sh))
#define ERR_DIFF_EXT_TEMP           (ERR_MASK<<(ERR_MASK_WIDTH*DIFF_EXT_TEMP_sh))
#define ERR_DIFF_RESISTANCE         (ERR_MASK<<(ERR_MASK_WIDTH*DIFF_RESISTANCE_sh))

#define VOLTAGE_OVERHIGH            (0x0001)
#define VOLTADE_UNDERLOW            (0x0002)
#define INT_TEMP_OVERHIGH           (0x0004)
#define INT_TEMP_UNDERLOW           (0x0008)
#define EXT_TEMP_OVERHIGH           (0x0010)
#define EXT_TEMP_UNDERLOW           (0x0020)
#define RESISTANCE_OVERHIGH         (0x0040)
#define RESISTANCE_UNDERLOW         (0x0080)
#define DIFF_VOLTAGE_OVERHIGH       (0x0100)
#define DIFF_VOLTADE_UNDERLOW       (0x0200)
#define DIFF_INT_TEMP_OVERHIGH      (0x0400)
#define DIFF_INT_TEMP_UNDERLOW      (0x0800)
#define DIFF_EXT_TEMP_OVERHIGH      (0x1000)
#define DIFF_EXT_TEMP_UNDERLOW      (0x2000)
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

typedef union
{
    uint16_t w[4];
    uint64_t qw;
} UINT64_UNION;

void my_lib_bat::set_limites(MY_LIB_limites* limites, uint16_t mask)
{
    if(mask & MAX_VOLTAGE_MASK) lim_voltage[0]=limites->lim_voltage[0];
    if(mask & MIN_VOLTAGE_MASK) lim_voltage[1]=limites->lim_voltage[1];
    if(mask & MAX_INT_TEMP_MASK) lim_int_temp[0]=limites->lim_int_temp[0];
    if(mask & MIN_INT_TEMP_MASK) lim_int_temp[1]=limites->lim_int_temp[1];
    if(mask & MAX_REZISTANCE_MASK) lim_resistance[0]=limites->lim_resistance[0];
    if(mask & MIN_REZISTANCE_MASK) lim_resistance[1]=limites->lim_resistance[1];
    if(mask & MAX_EXT_TEMP_MASK) lim_ext_temp[0]=limites->lim_ext_temp[0];
    if(mask & MIN_EXT_TEMP_MASK) lim_ext_temp[1]=limites->lim_ext_temp[1];
    if(mask & MAX_VOLTAGE_CHANGE_MASK) lim_diff_voltage[0]=limites->lim_diff_voltage[0];
    if(mask & MIN_VOLTAGE_CHANGE_MASK) lim_diff_voltage[1]=limites->lim_diff_voltage[1];
    if(mask & MAX_INT_TEMP_CHANGE_MASK) lim_diff_int_temp[0]=limites->lim_diff_int_temp[0];
    if(mask & MIN_INT_TEMP_CHANGE_MASK) lim_diff_int_temp[1]=limites->lim_diff_int_temp[1];
    if(mask & MAX_REZISTANCE_CHANGE_MASK) lim_diff_resistance[0]=limites->lim_diff_resistance[0];
    if(mask & MIN_REZISTANCE_CHANGE_MASK) lim_diff_resistance[1]=limites->lim_diff_resistance[1];
    if(mask & MAX_EXT_TEMP_CHANGE_MASK) lim_diff_ext_temp[0]=limites->lim_diff_ext_temp[0];
    if(mask & MIN_EXT_TEMP_CHANGE_MASK) lim_diff_ext_temp[1]=limites->lim_diff_ext_temp[1];
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
    lim->lim_int_temp[0] = lim_int_temp[0];
    lim->lim_int_temp[1] = lim_int_temp[1];
    lim->lim_resistance[0] = lim_resistance[0];
    lim->lim_resistance[1] = lim_resistance[1];
    lim->lim_ext_temp[0] = lim_ext_temp[0];
    lim->lim_ext_temp[1] = lim_ext_temp[1];
    lim->lim_diff_voltage[0] = lim_diff_voltage[0];
    lim->lim_diff_voltage[1] = lim_diff_voltage[1];
    lim->lim_diff_int_temp[0] = lim_diff_int_temp[0];
    lim->lim_diff_int_temp[1] = lim_diff_int_temp[1];
    lim->lim_diff_resistance[0] = lim_diff_resistance[0];
    lim->lim_diff_resistance[1] = lim_diff_resistance[1];
    lim->lim_diff_ext_temp[0] = lim_diff_ext_temp[0];
    lim->lim_diff_ext_temp[1] = lim_diff_ext_temp[1];
}

void my_lib_bat::get_params(uint16_t *buf)
{
    INT16_UNION temp;
    UINT64_UNION l;
    temp.u = voltage[CUR]; buf[0] = temp.u;
    temp.s = int_temp[CUR]; buf[1] = temp.u;
    temp.u = resistance[CUR]; buf[2] = temp.u;
    temp.s = ext_temp[CUR]; buf[3] = temp.u;
    temp.s = diff_voltage; buf[4] = temp.u;
    temp.s = diff_int_temp; buf[5] = temp.u;
    temp.s = diff_resistance; buf[6] = temp.u;
    temp.s = diff_ext_temp; buf[7] = temp.u;

    l.qw = time[CUR];
    buf[8] = l.w[0];
    buf[9] = l.w[1];
    buf[10] = l.w[2];
    buf[11] = l.w[3];

    buf[12] = error_state;
    buf[13] = error_save;

    buf[14] = dt;
}

void my_lib_bat::calc()
{
    float coef = 10000.0 / (float)dt;
    diff_voltage = (float)(voltage[CUR] - voltage[PRE]) * coef;
    diff_int_temp = (float)(int_temp[CUR] - int_temp[PRE]) * coef;
    diff_resistance = (float)(resistance[CUR] - resistance[PRE]) * coef;
    diff_ext_temp = (float)(ext_temp[CUR] - ext_temp[PRE]) * coef;
/*
    diff_voltage = (float)(voltage[CUR]-voltage[PRE])*1000.0/((float)(dt));
    diff_int_temp = (float)(int_temp[CUR]-int_temp[PRE])*1000.0/((float)(dt));
    diff_resistance = (float)(resistance[CUR]-resistance[PRE])*1000.0/((float)(dt));
    diff_ext_temp = (float)(ext_temp[CUR]-ext_temp[PRE])*1000.0/((float)(dt));
*/

    error_state = 0;
    if(voltage[CUR] > lim_voltage[MAX]) error_state |= VOLTAGE_OVERHIGH;
    if(voltage[CUR] < lim_voltage[MIN]) error_state |= VOLTADE_UNDERLOW;
    if(int_temp[CUR] > lim_int_temp[MAX]) error_state |= INT_TEMP_OVERHIGH;
    if(int_temp[CUR] < lim_int_temp[MIN]) error_state |= INT_TEMP_UNDERLOW;
    if(resistance[CUR] > lim_resistance[MAX]) error_state |= RESISTANCE_OVERHIGH;
    if(resistance[CUR] < lim_resistance[MIN]) error_state |= RESISTANCE_UNDERLOW;
    if(ext_temp[CUR] > lim_ext_temp[MAX]) error_state |= EXT_TEMP_OVERHIGH;
    if(ext_temp[CUR] < lim_ext_temp[MIN]) error_state |= EXT_TEMP_UNDERLOW;
    if(diff_voltage > lim_diff_voltage[MAX]) error_state |= DIFF_VOLTAGE_OVERHIGH;
    if(diff_voltage < lim_diff_voltage[MIN]) error_state |= DIFF_VOLTADE_UNDERLOW;
    if(diff_int_temp > lim_diff_int_temp[MAX]) error_state |= DIFF_INT_TEMP_OVERHIGH;
    if(diff_int_temp < lim_diff_int_temp[MIN]) error_state |= DIFF_INT_TEMP_UNDERLOW;
    if(diff_resistance > lim_diff_resistance[MAX]) error_state |= DIFF_RESISTANCE_OVERHIGH;
    if(diff_resistance < lim_diff_resistance[MIN]) error_state |= DIFF_RESISTANCE_UNDERLOW;
    if(diff_ext_temp > lim_diff_ext_temp[MAX]) error_state |= DIFF_EXT_TEMP_OVERHIGH;
    if(diff_ext_temp < lim_diff_ext_temp[MIN]) error_state |= DIFF_EXT_TEMP_UNDERLOW;
    error_save |= error_state;
}

my_lib_bat bat_my_lib;
volatile MY_LIB_limites ML_limites = INIT_LIMITES;