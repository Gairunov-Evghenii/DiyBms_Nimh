#include "nimh_bms.h"

nimh_bms bms __attribute__ ((section (".noinit")));

static void nimh_bms_update_state(){
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

    if(bms.timer > CUT_OFF_TIMER){
        bms.state |= BMS_STATE_CHARGED;
    }

}

void nimh_bms_init(){
    if (bms.state == BMS_STATE_UNKNOWN){
        bms.state = BMS_STATE_INITIALIZED;
        bms.temperature = 0;
        bms.voltage_mV = 0;
        bms.temperature_slope = 0;
        bms.voltage_slope = 0;
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
        bms.temperature_slope = temp;
    }else{
        bms.temperature_slope = (bms.temperature * (SAMPLES-1) + temp)/SAMPLES;
    }
    bms.temperature_slope = temp - bms.temperature_slope;
    
    nimh_bms_update_state();
}

void nimh_bms_read_voltage(uint16_t voltage){
    bms.voltage_mV = voltage;


    if(bms.timer == 1){
        bms.voltage_slope = voltage;
    }else{
        bms.voltage_slope = (bms.voltage_mV * (SAMPLES-1) + voltage)/SAMPLES;
    }
    bms.voltage_slope = voltage - bms.voltage_slope;

    nimh_bms_update_state();
}

uint16_t nimh_bms_check_state(){
    return bms.state;
}

int16_t nimh_bms_check_temperature_slope(){
    return bms.temperature_slope;
}
int16_t nimh_bms_check_voltage_slope(){
    return bms.voltage_slope;
}
