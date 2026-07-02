#include "mrhof_energy.h"

//#define ADC_BATTERY_LINE (5)

static uint32_t initial_energy;
static uint32_t initial_time;

uint32_t get_remaining_energy(void) 
{
    int sample = 0;
    for (unsigned i = 0; i < AVG_CNT; i++) {
        sample += adc_sample(ADC_BATTERY_LINE, ADC_RES);
    }
    uint32_t vbat_mv = sample * ADC_BAT_RESISTOR_DIV / AVG_CNT * ADC_VREF_MV / ADC_MAX_VAL;

    return vbat_mv;
}

uint8_t get_energetic_happiness(void)
{
    double e_bat = (double) get_remaining_energy();
    double time_passed = (double) ztimer_now(ZTIMER_USEC) - initial_time;

    double energy = e_bat / initial_energy;
    if (e_bat >= initial_energy) {
        energy = 1;
    }
    double time = (DESIRED_LIFETIME_US - (double) time_passed) / DESIRED_LIFETIME_US;
    double E_E = energy * time;
    
    uint8_t E_E_p = (uint8_t) (E_E * 100);
    if (E_E_p < 20) 
    {
        return 0;
    }
    return E_E_p;
}

void init_mrhof_energy(void)
{
    initial_time = ztimer_now(ZTIMER_USEC);
    initial_energy = get_remaining_energy();

    if (adc_init(ADC_BATTERY_LINE) < 0) {
        printf("battery_info_init: ADC_LINE(%u) init [FAILED]\n", ADC_BATTERY_LINE);
    }
}