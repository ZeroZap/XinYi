#ifndef _BANK_BATTERY_ATL_H_
#define _BANK_BATTERY_ATL_H_
#include "bank-battery.h"
#ifdef __cplusplus
extern "c" {
#endif

#if (BANK_CFG_BATT_TEMP_BAND_MAX == 3 && BANK_CFG_BATT_AGE_MAX == 3)

const bank_batt_target_profile_t
    batt_atl_profile[BANK_CFG_BATT_AGE_MAX][BANK_CFG_BATT_TEMP_BAND_MAX] = {
        /* Age 1. */
        { /* 00 - 10 deg C: targeted voltage 4208 mV, current 480 mA. */
          { .volt = 4208, .current = 480 },
          /* 11 - 45 deg C: targeted voltage 4208 mV, current 1680 mA. */
          { .volt = 4208, .current = 1680 },
          /* 46 - 60 deg C: targeted voltage 4096 mV, current 1680 mA. */
          { .volt = 4096, .current = 1680 } },

        /* Age 2. */
        { /* 00 - 10 deg C: targeted voltage 4208 mV, current 480 mA. */
          { .volt = 4096, .current = 480 },
          /* 11 - 45 deg C: targeted voltage 4208 mV, current 1680 mA. */
          { .volt = 4096, .current = 1680 },
          /* 46 - 60 deg C: targeted voltage 4096 mV, current 1680 mA. */
          { .volt = 3888, .current = 1680 } },

        /* Age 3. */
        { /* 00 - 10 deg C: targeted voltage 4208 mV, current 480 mA. */
          { .volt = 4016, .current = 480 },
          /* 11 - 45 deg C: targeted voltage 4208 mV, current 1680 mA. */
          { .volt = 4016, .current = 1680 },
          /* 46 - 60 deg C: targeted voltage 4096 mV, current 1680 mA. */
          { .volt = 3840, .current = 1680 } },
    };

/**
 * Table of voltage value for 100% in mV when system is idle for ATL.
 */
const uint16_t
    batt_atl_v100_idle[BANK_CFG_BATT_AGE_MAX][BANK_CFG_BATT_TEMP_BAND_MAX] = {
        /* Age 1. */
        { 4192,   /* 00 - 10 deg C: v100 voltage 4192 mV. */
          4192,   /* 11 - 45 deg C: v100 voltage 4192 mV. */
          4080 }, /* 46 - 60 deg C: v100 voltage 4080 mV. */

        /* Age 2. */
        { 4080,   /* 00 - 10 deg C: v100 voltage 4080 mV. */
          4080,   /* 11 - 45 deg C: v100 voltage 4080 mV. */
          3872 }, /* 46 - 60 deg C: v100 voltage 3872 mV. */

        /* Age 3. */
        { 4000,  /* 00 - 10 deg C: v100 voltage 4000 mV. */
          4000,  /* 11 - 45 deg C: v100 voltage 4000 mV. */
          3824 } /* 46 - 60 deg C: v100 voltage 3824 mV. */
    };

/**
 * Table of voltage value for 100% in mV when system is charging for ATL.
 */
const uint16_t batt_atl_v100_charging
    [BANK_CFG_BATT_AGE_MAX][BANK_CFG_BATT_TEMP_BAND_MAX] = {
        /* Age 1. */
        { 4200,   /* 00 - 10 deg C: v100 voltage 4200 mV. */
          4200,   /* 11 - 45 deg C: v100 voltage 4200 mV. */
          4088 }, /* 46 - 60 deg C: v100 voltage 4088 mV. */

        /* Age 2. */
        { 4088,   /* 00 - 10 deg C: v100 voltage 4088 mV. */
          4088,   /* 11 - 45 deg C: v100 voltage 4088 mV. */
          3880 }, /* 46 - 60 deg C: v100 voltage 3880 mV. */

        /* Age 3. */
        { 4008,  /* 00 - 10 deg C: v100 voltage 4008 mV. */
          4008,  /* 11 - 45 deg C: v100 voltage 4008 mV. */
          3832 } /* 46 - 60 deg C: v100 voltage 3832 mV. */
    };

const uint16_t
    batt_atl_target_volt[BANK_CFG_BATT_AGE_MAX][BANK_CFG_BATT_TEMP_BAND_MAX] = {
        /* Age 1. */
        { 4192,   /* 00 - 10 deg C: v100 voltage 4.208V. */
          4192,   /* 11 - 45 deg C: v100 voltage 4.208V. */
          4080 }, /* 46 - 60 deg C: v100 voltage 4.096V. */

        /* Age 2. */
        { 4080,   /* 00 - 10 deg C: v100 voltage 4.080V. */
          4080,   /* 11 - 45 deg C: v100 voltage 4.080V. */
          3872 }, /* 46 - 60 deg C: v100 voltage 3.872V. */

        /* Age 3. */
        { 4000,  /* 00 - 10 deg C: v100 voltage 4.000V. */
          4000,  /* 11 - 45 deg C: v100 voltage 4.000V. */
          3824 } /* 46 - 60 deg C: v100 voltage 4.824V. */
    };
#else
#error \
    "BANK_CFG_BATT_AGE_MAX & BANK_CFG_BATT_TEMP_BAND_MAX defined group value is not supported"
#endif


#if (                                                              \
    BANK_CFG_BATT_TEMP_BAND_MAX == 3 && BANK_CFG_BATT_AGE_MAX == 3 \
    && BANK_CFG_BATT_SOC_LEVEL_SIZE == 4)


const bank_batt_soc_element_t batt_atl_soc_idle
    [BANK_CFG_BATT_AGE_MAX][BANK_CFG_BATT_TEMP_BAND_MAX]
    [BANK_CFG_BATT_SOC_LEVEL_SIZE] = {
        /* Age 1. */
        { /* 00 - 10 deg C: targeted voltage 4.208V, current 0.512 A. */
          { { 3893, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3663, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3562, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3000, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } },
          /* 11 - 45 deg C: targeted voltage 4.208V, current 1.728 A. */
          { { 3893, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3663, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3562, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3000, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } },
          /* 46 - 60 deg C: targeted voltage 4.096V, current 1.728 A. */
          { { 3814, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3642, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3548, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3000, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } } },
        /* Age 2. */
        { /* 00 - 10 deg C: targeted voltage 4.096V, current 0.512 A. */
          { { 3814, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3642, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3548, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3000, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } },
          /* 11 - 45 deg C: targeted voltage 4.096V, current 1.728 A. */
          { { 3814, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3642, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3548, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3000, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } },
          /* 46 - 60 deg C: targeted voltage 3.888V, current 1.728 A. */
          { { 3676, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3607, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3499, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3000, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } } },
        /* Age 3. */
        { /* 00 - 10 deg C: targeted voltage 4.016V, current 0.512 A. */
          { { 3776, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3635, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3543, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3000, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } },
          /* 11 - 45 deg C: targeted voltage 4.016V, current 1.728 A. */
          { { 3776, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3635, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3543, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3000, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } },
          /* 46 - 60 deg C: targeted voltage 3.840V, current 1.728 A. */
          { { 3661, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3598, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3491, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3000, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } } }
    };

const bank_batt_soc_element_t batt_atl_soc_charging
    [BANK_CFG_BATT_AGE_MAX][BANK_CFG_BATT_TEMP_BAND_MAX]
    [BANK_CFG_BATT_SOC_LEVEL_SIZE] = {
        /* Age 1. */
        { /* 00 - 10 deg C: targeted voltage 4.208V, current 0.512 A. */
          { { 3958, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3740, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3649, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3058, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } },
          /* 11 - 45 deg C: targeted voltage 4.208V, current 1.728 A. */
          { { 4081, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3866, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3760, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3154, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } },
          /* 46 - 60 deg C: targeted voltage 4.096V, current 1.728 A. */
          { { 4004, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3844, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3751, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3145, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } } },
        /* Age 2. */
        { /* 00 - 10 deg C: targeted voltage 4.096V, current 0.512 A. */
          { { 3890, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3719, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3634, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3061, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } },
          /* 11 - 45 deg C: targeted voltage 4.096V, current 1.728 A. */
          { { 4004, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3844, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3751, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3145, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } },
          /* 46 - 60 deg C: targeted voltage 3.888V, current 1.728 A. */
          { { 3871, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3824, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3732, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3164, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } } },
        /* Age 3. */
        { /* 00 - 10 deg C: targeted voltage 4.016V, current 0.512 A. */
          { { 3844, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3716, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3621, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3064, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } },
          /* 11 - 45 deg C: targeted voltage 4.016V, current 1.728 A. */
          { { 3953, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3815, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3732, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3159, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } },
          /* 46 - 60 deg C: targeted voltage 3.840V, current 1.728 A. */
          { { 3824, BANK_BATT_SOC_LEVEL_75, bank_batt_level_full },
            { 3815, BANK_BATT_SOC_LEVEL_50, bank_batt_level_high },
            { 3724, BANK_BATT_SOC_LEVEL_25, bank_batt_level_medium },
            { 3166, BANK_BATT_SOC_LEVEL_0, bank_batt_level_low } } }
    };
#else
#error \
    "BANK_CFG_BATT_AGE_MAX & BANK_CFG_BATT_TEMP_BAND_MAX" \
    "& BANK_CFG_BATT_SOC_LEVEL_SIZE defined group value is not supported"
#endif

#ifdef __cplusplus
}
#endif

#endif /** _BANK_BATTERY_ATL_H_ */
