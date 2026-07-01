#pragma once

/**
 * @ingroup     net_gnrc_rpl
 * @{
 * @file
 * @brief       Battery measurement for Minimum Rank with Hysteresis Objective Function
 *
 * Header-file, which provides a battery measurement
 *
 * @author      Thuy An Nguyen
 */

#include <stdint.h>
#include <stdio.h>
#include "periph/adc.h"
#include "ztimer.h"
#include "inttypes.h"

/* Battery defines*/
#define ADC_RES               ADC_RES_12BIT
#define ADC_MAX_VAL           (1 << 12)
#define ADC_VREF_MV           (3300)
#define ADC_BAT_RESISTOR_DIV  (2)
#define AVG_CNT               (16)
#define VBAT_MIN_MV           (3700)
#define VBAT_MULTIPLICATOR    (0.9404)

//#define DESIRED_LIFETIME_US (64800000000)  /* 64.800.000.000 us = 18 h*/
//#define DESIRED_LIFETIME_US (3600000000) /* 1 h*/
#define DESIRED_LIFETIME_US (129600000000) /* 36 h*/

/**
 * @brief   Measure current voltage via ADC_BATTERY_LINE and return remaining energy
 *
 * @return  remaining energy in mV
 */
uint32_t get_remaining_energy_test(void);

/**
 * @brief   Calculate current energetic happiness according to RFC6551 (Section 3.2)
 *
 * @return  current energetic happiness in units of percent
 */
uint8_t get_energetic_happiness_test(void);

/**
 * @brief   Initialize energy variables
 */
void init_mrhof_energy_test(void);