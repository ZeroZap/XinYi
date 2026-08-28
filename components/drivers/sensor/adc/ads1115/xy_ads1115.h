/**
 * @file xy_ads1115.h
 * @brief Canonical ADS1115 4-channel 16-bit ADC Device driver
 */

#ifndef XY_ADS1115_H
#define XY_ADS1115_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>

#define ADS1115_ADDR_GND 0x48
#define ADS1115_ADDR_VDD 0x49
#define ADS1115_ADDR_SDA 0x4A
#define ADS1115_ADDR_SCL 0x4B

#define ADS1115_REG_CONVERT 0x00
#define ADS1115_REG_CONFIG 0x01
#define ADS1115_REG_LO_THRESH 0x02
#define ADS1115_REG_HI_THRESH 0x03

#define ADS1115_CONFIG_OS_SINGLE (1U << 15)
#define ADS1115_CONFIG_MUX_DIFF_0_1 (0U << 12)
#define ADS1115_CONFIG_MUX_DIFF_0_3 (1U << 12)
#define ADS1115_CONFIG_MUX_DIFF_1_3 (2U << 12)
#define ADS1115_CONFIG_MUX_DIFF_2_3 (3U << 12)
#define ADS1115_CONFIG_MUX_SINGLE_0 (4U << 12)
#define ADS1115_CONFIG_MUX_SINGLE_1 (5U << 12)
#define ADS1115_CONFIG_MUX_SINGLE_2 (6U << 12)
#define ADS1115_CONFIG_MUX_SINGLE_3 (7U << 12)
#define ADS1115_CONFIG_MODE_SINGLE (0U << 8)
#define ADS1115_CONFIG_COMP_DISABLE (3U << 0)

typedef enum {
    ADS1115_PGA_6_144V = 0,
    ADS1115_PGA_4_096V,
    ADS1115_PGA_2_048V,
    ADS1115_PGA_1_024V,
    ADS1115_PGA_0_512V,
    ADS1115_PGA_0_256V,
} xy_ads1115_pga_t;

typedef enum {
    ADS1115_DR_8SPS = 0,
    ADS1115_DR_16SPS,
    ADS1115_DR_32SPS,
    ADS1115_DR_64SPS,
    ADS1115_DR_128SPS,
    ADS1115_DR_250SPS,
    ADS1115_DR_475SPS,
    ADS1115_DR_860SPS,
} xy_ads1115_dr_t;

#define XY_ADS1115_OK 0
#define XY_ADS1115_ERROR (-1)
#define XY_ADS1115_INVALID_PARAM (-2)
#define XY_ADS1115_NOT_FOUND (-3)
#define XY_ADS1115_TIMEOUT (-4)

typedef struct {
    xy_i2c_device_t i2c_dev;
    uint8_t addr;
    xy_ads1115_pga_t pga;
    xy_ads1115_dr_t dr;
    int16_t last_value;
    uint8_t initialized;
} xy_ads1115_t;

int xy_ads1115_init(xy_ads1115_t *dev, void *i2c_handle, uint8_t addr);
int xy_ads1115_deinit(xy_ads1115_t *dev);
int xy_ads1115_read_single(xy_ads1115_t *dev, uint8_t channel, int16_t *value);
int xy_ads1115_read_diff(xy_ads1115_t *dev, uint8_t channel_p, uint8_t channel_n, int16_t *value);
int xy_ads1115_read_voltage(xy_ads1115_t *dev, uint8_t channel, int32_t *voltage_mv);
int xy_ads1115_set_pga(xy_ads1115_t *dev, xy_ads1115_pga_t pga);
int xy_ads1115_set_dr(xy_ads1115_t *dev, xy_ads1115_dr_t dr);

#ifdef __cplusplus
}
#endif

#endif /* XY_ADS1115_H */
