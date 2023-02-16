#include "nimh_bms.h"

nimh_bms bms __attribute__((section(".noinit")));

static void nimh_bms_check_temperature_safety();
static void nimh_bms_check_voltage_safety();

void nimh_bms_init()
{
    if (bms.state == BMS_STATE_UNKNOWN)
    {
        bms.state = BMS_STATE_INITIALIZED;
        bms.state |= BMS_STATE_CHARGING;
        bms.temperature = 0;
        bms.voltage_mV = 0;
        for (uint8_t u = 0; u < 2; u++)
        {
            bms.max_temp[u] = 0;
            bms.min_temp[u] = 0;
            bms.max_voltage[u] = 0;
            bms.min_voltage[u] = 0;
            bms.avg_temp_slope[u] = 0;
        }
        bms.timer = 0;
    }
}

void nimh_bms_read_temperature(uint16_t temp)
{
    bms.timer += 1;
    int16_t temperature = 0;
    if (0x8000 & temp)
    {
        temperature = -(temp & 0x7fffU);
    }
    else
    {
        temperature = temp;
    }

    if (bms.timer % SAMPLE_RANGE == 1)
    {
        bms.max_temp[CURREN] = temperature;
        bms.min_temp[CURREN] = temperature;
        bms.avg_temp_slope[CURREN] = 0;
    }

    bms.avg_temp_slope[CURREN] += temperature - bms.temperature;
    bms.temperature = temperature;

    if (bms.temperature > bms.max_temp[CURREN])
    {
        bms.max_temp[CURREN] = bms.temperature;
    }
    if (bms.temperature < bms.min_temp[CURREN])
    {
        bms.min_temp[CURREN] = bms.temperature;
    }

    nimh_bms_check_temperature_safety();
}

void nimh_bms_read_voltage(uint16_t voltage)
{
    bms.voltage_mV = voltage;

    // if(bms.voltage_mV > bms.max_voltage[CURREN]){
    //     bms.max_voltage[CURREN] = bms.voltage_mV;
    // }
    // if(bms.voltage_mV < bms.min_voltage[CURREN]){
    //     bms.min_voltage[CURREN] = bms.voltage_mV;
    // }

    nimh_bms_check_voltage_safety();
}

static void nimh_bms_shift_samples()
{
    bms.max_temp[PREVIOUS] = bms.max_temp[CURREN];
    bms.min_temp[PREVIOUS] = bms.min_temp[CURREN];
    bms.avg_temp_slope[PREVIOUS] = bms.avg_temp_slope[CURREN];
    bms.max_voltage[PREVIOUS] = bms.max_voltage[CURREN];
    bms.min_voltage[PREVIOUS] = bms.min_voltage[CURREN];
}

void nimh_bms_sample_range_tick()
{
    if (bms.timer % SAMPLE_RANGE)
    { // will enter the function every SAmple_range seconds
        return;
    }

    if(bms.avg_temp_slope[CURREN] >= 0){
        bms.avg_temp_slope[CURREN] = bms.max_temp[CURREN] - bms.min_temp[CURREN];
    }
    else
    {
        bms.avg_temp_slope[CURREN] = -(bms.max_temp[CURREN] - bms.min_temp[CURREN]);
    }

    if (bms.timer == SAMPLE_RANGE)
    {
        nimh_bms_shift_samples();
        return;
    }

    if (bms.avg_temp_slope[PREVIOUS] > 0 && bms.avg_temp_slope[CURREN] * 10 > bms.avg_temp_slope[PREVIOUS] * 15)
    {
        bms.state |= BMS_STATE_CHARGED;
        bms.state &= ~BMS_STATE_CHARGING;
        bms.timer = 0;
    }

    if(bms.avg_temp_slope[CURREN] < 0 && bms.timer > CHARGED_TIME_REST){
        bms.state &= ~BMS_STATE_CHARGED;
        bms.state |= BMS_STATE_CHARGING;
    }

    nimh_bms_shift_samples();
}

uint16_t nimh_bms_check_state()
{
    return bms.state;
}

int16_t nimh_bms_check_temperature_state()
{
    return (
        (((uint8_t)(bms.max_temp[PREVIOUS] - 150))<<8)|
        ((uint8_t)(bms.min_temp[PREVIOUS] - 150))
    );
    return bms.avg_temp_slope[PREVIOUS];
}

static void nimh_bms_check_temperature_safety()
{
    if (bms.temperature > TEMPERATURE_CUT_OFF)
    {
        bms.state |= BMS_STATE_TEMPERATURE_CUT_OFF;
    }
    else
    {
        bms.state &= ~BMS_STATE_TEMPERATURE_CUT_OFF;
    }

    if (bms.temperature > TEMPERATURE_HIGH)
    {
        bms.state |= BMS_STATE_TEMPERATURE_HIGH;
    }
    else
    {
        bms.state &= ~BMS_STATE_TEMPERATURE_HIGH;
    }

    if (bms.temperature < TEMPERATURE_LOW)
    {
        bms.state |= BMS_STATE_TEMPERATURE_LOW;
    }
    else
    {
        bms.state &= ~BMS_STATE_TEMPERATURE_LOW;
    }
}

static void nimh_bms_check_voltage_safety()
{
    if (bms.voltage_mV < VOLTAGE_LOW)
    {
        bms.state |= BMS_STATE_UNDERVOLTAGE;
    }
    else
    {
        bms.state &= ~BMS_STATE_UNDERVOLTAGE;
    }

    if (bms.voltage_mV > VOLTAGE_HIGH)
    {
        bms.state |= BMS_STATE_OVERVOLTAGE;
    }
    else
    {
        bms.state &= ~BMS_STATE_OVERVOLTAGE;
    }
}