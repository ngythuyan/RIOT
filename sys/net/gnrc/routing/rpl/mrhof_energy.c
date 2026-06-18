#include "mrhof_energy.h"

static uint32_t initial_energy;
static uint32_t initial_time;

uint32_t get_remaining_energy(void) 
{

    if (adc_init(ADC_BATTERY_LINE) < 0) {
        printf("battery_info_init: ADC_LINE(%u) init [FAILED]\n", ADC_BATTERY_LINE);
        return 1;
    }

    int sample = 0;
    for (unsigned i = 0; i < AVG_CNT; i++) {
        sample += adc_sample(ADC_BATTERY_LINE, ADC_RES);
    }
    uint32_t vbat_mv = sample * ADC_BAT_RESISTOR_DIV / AVG_CNT * ADC_VREF_MV / ADC_MAX_VAL;

    return (uint8_t) VBAT_MULTIPLICATOR * (vbat_mv - VBAT_MIN_MV);
}

uint8_t get_energetic_happiness(void)
{
    uint32_t e_bat = get_remaining_energy();
    uint32_t time_passed = ztimer_now(ZTIMER_USEC) - initial_time;

    return (uint8_t) (e_bat / (initial_energy * (DESIRED_LIFETIME_US - time_passed)) / DESIRED_LIFETIME_US);
}

void init_mrhof_energy(void)
{
    initial_time = ztimer_now(ZTIMER_USEC);
    initial_energy = get_remaining_energy();
}