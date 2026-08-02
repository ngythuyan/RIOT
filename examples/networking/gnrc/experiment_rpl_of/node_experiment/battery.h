#include <inttypes.h>
#include "periph/adc.h"

#define ADC_RES               ADC_RES_12BIT
#define ADC_MAX_VAL           (1 << 12)
#define ADC_VREF_MV           (3300)
#define ADC_BAT_RESISTOR_DIV  (2)
//#define VBAT_MAX_MV           (4250)
#define VBAT_MAX_MV           (4150)
//#define VBAT_MIN_MV           (3500)
#define AVG_CNT (16)
#define ADC_BATTERY_LINE ADC_LINE(5)

uint16_t get_voltage(void);