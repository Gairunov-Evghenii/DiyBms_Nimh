#include "nimh_bms.h"

nimh_bms bms __attribute__ ((section (".noinit")));

void nimh_bms_init(){
    if (bms.state == BMS_STATE_UNKNOWN){
        bms.state = BMS_STATE_INITIALIZED;
        bms.temperature = 0;
        bms.voltage_mV = 0;
        bms.temp_slope = 0;
        for(uint8_t u = 0; u < 2; u++){
            bms.max_temp[u] = 0;
            bms.min_temp[u] = 0;
            bms.max_voltage[u] = 0;
            bms.min_voltage[u] = 0;

        }
        bms.timer = 0;
    }
}

void nimh_bms_read_temperature(uint16_t temp){
    bms.timer += 1;
    if(0x8000 & temp){
        bms.temperature = -(temp & 0x7fffU);
    }
    else{
      bms.temperature = temp;
    }
    if(bms.timer == 1){
        bms.max_temp[CURREN] = temp;
        bms.min_temp[CURREN] = temp;
        bms.max_temp[PREVIOUS] = temp;
        bms.min_temp[PREVIOUS] = temp;
    }else if (bms.timer % SAMPLE_RANGE == 1){
        bms.max_temp[CURREN] = temp;
        bms.min_temp[CURREN] = temp;
    }

    if(bms.temperature > bms.max_temp[0]){
        bms.max_temp[0] = bms.temperature;
    }
    if(bms.temperature < bms.min_temp[0]){
        bms.min_temp[0]= bms.temperature;
    }

    if(bms.temperature > TEMPERATURE_CUT_OFF){
        bms.state |= BMS_STATE_TEMPERATURE_CUT_OFF;
    }else{
        bms.state &= ~BMS_STATE_TEMPERATURE_CUT_OFF;
    }

    if(bms.temperature > TEMPERATURE_HIGH){
        bms.state |= BMS_STATE_TEMPERATURE_HIGH;
    }else{
        bms.state &= ~BMS_STATE_TEMPERATURE_HIGH;
    }

    if(bms.temperature < TEMPERATURE_LOW){
        bms.state |= BMS_STATE_TEMPERATURE_LOW;
    }else{
        bms.state &= ~BMS_STATE_TEMPERATURE_LOW;
    }
}

void nimh_bms_read_voltage(uint16_t voltage){
    bms.voltage_mV = voltage;

    if(bms.voltage_mV > bms.max_voltage[0]){
        bms.max_voltage[0] = bms.voltage_mV;
    }
    if(bms.voltage_mV < bms.min_voltage[0]){
        bms.min_voltage[0] = bms.voltage_mV;
    }

    if(bms.voltage_mV < VOLTAGE_LOW){
        bms.state |= BMS_STATE_UNDERVOLTAGE;
    }else{
        bms.state &= ~BMS_STATE_UNDERVOLTAGE;
    }

    if(bms.voltage_mV > VOLTAGE_HIGH){
        bms.state |= BMS_STATE_OVERVOLTAGE;
    }else{
        bms.state &= ~BMS_STATE_OVERVOLTAGE;
    }
}

static void nimh_bms_shift_samples(){
    bms.max_temp[1] = bms.max_temp[0];
    bms.min_temp[1] = bms.min_temp[0];
    bms.max_voltage[1] = bms.max_voltage[0];
    bms.min_voltage[1] = bms.min_voltage[0];
}

void nimh_bms_sample_range_tick(){
    if(bms.timer % SAMPLE_RANGE){//will enter the function every SAmple_range seconds
        return;
    }

    if(bms.timer == SAMPLE_RANGE){
        //first iteration
        nimh_bms_shift_samples();
        return;
    }
    // int16_t slope0 = bms.max_temp[0] - bms.min_temp[0];
    // int16_t slope1 = bms.max_temp[1] - bms.min_temp[1];
    // if(slope0 * 100 > slope1 * 150){
    //     bms.state |= BMS_STATE_CHARGED;
    // }
    // else{
    //     bms.state &= ~BMS_STATE_CHARGED;
    // }
    if(bms.max_temp[0] * 100 > bms.max_temp[1] * 150){
        bms.temp_slope = bms.max_temp[0] - bms.min_temp[0];
    }else{
        bms.temp_slope = bms.min_temp[0] - bms.max_temp[0];
    }

    nimh_bms_shift_samples();
}

uint16_t nimh_bms_check_state(){
    return bms.state;
}

int16_t nimh_bms_check_temperature_slope(){
    // return bms.max_temp[0] - bms.min_temp[0];
    return bms.temp_slope;
}

