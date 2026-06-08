#include "battery.h"
#include "periph/adc.h"
#include <stdio.h>

uint16_t get_voltage(void) {

    if (adc_init(ADC_BATTERY_LINE) < 0) {
        printf("battery_info_init: ADC_LINE(%u) init [FAILED]\n", ADC_BATTERY_LINE);
        return 1;
    }
//    printf("battery_info_init: ADC_LINE(%u) init [OK]\n", ADC_BATTERY_LINE);

    int sample = 0;
    for (unsigned i = 0; i < AVG_CNT; i++) {
        sample += adc_sample(ADC_BATTERY_LINE, ADC_RES);
    }
    uint16_t vbat_mv = sample * ADC_BAT_RESISTOR_DIV / AVG_CNT * ADC_VREF_MV / ADC_MAX_VAL;
    return vbat_mv;
}