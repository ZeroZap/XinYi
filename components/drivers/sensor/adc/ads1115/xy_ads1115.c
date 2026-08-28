/**
 * @file xy_ads1115.c
 * @brief Canonical ADS1115 4-channel 16-bit ADC Device driver
 */

#include "xy_ads1115.h"
#include "xy_os.h"
#include <string.h>

static const float g_lsb_mv[] = {
    0.187500F,
    0.125000F,
    0.062500F,
    0.031250F,
    0.015625F,
    0.007812F,
};

static int ads1115_write_config(xy_ads1115_t *dev, uint16_t config)
{
    uint8_t data[3] = {
        ADS1115_REG_CONFIG,
        (uint8_t)(config >> 8),
        (uint8_t)config,
    };

    return xy_i2c_device_write_reg(&dev->i2c_dev, ADS1115_REG_CONFIG, data, sizeof(data));
}

static int ads1115_read_word(xy_ads1115_t *dev, uint8_t reg, int16_t *value)
{
    uint8_t data[2];
    int ret = xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, sizeof(data));

    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    *value = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    return XY_DEVICE_OK;
}

static uint16_t ads1115_base_config(const xy_ads1115_t *dev)
{
    return ADS1115_CONFIG_OS_SINGLE | ((uint16_t)dev->pga << 9) |
           ADS1115_CONFIG_MODE_SINGLE | ((uint16_t)dev->dr << 5) |
           ADS1115_CONFIG_COMP_DISABLE;
}

static int ads1115_convert(xy_ads1115_t *dev, uint16_t mux, int16_t *value)
{
    int16_t sample;
    int ret = ads1115_write_config(dev, ads1115_base_config(dev) | mux);

    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    xy_os_delay(10);
    ret = ads1115_read_word(dev, ADS1115_REG_CONVERT, &sample);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    dev->last_value = sample;
    *value = sample;
    return XY_ADS1115_OK;
}

int xy_ads1115_init(xy_ads1115_t *dev, void *i2c_handle, uint8_t addr)
{
    int16_t config;
    int ret;

    if (dev == NULL || i2c_handle == NULL || addr < ADS1115_ADDR_GND || addr > ADS1115_ADDR_SCL) {
        return XY_ADS1115_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    ret = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, addr, 1000U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    dev->addr = addr;
    dev->pga = ADS1115_PGA_2_048V;
    dev->dr = ADS1115_DR_128SPS;

    ret = ads1115_read_word(dev, ADS1115_REG_CONFIG, &config);
    if (ret != XY_DEVICE_OK) {
        return XY_ADS1115_NOT_FOUND;
    }

    dev->initialized = 1U;
    return XY_ADS1115_OK;
}

int xy_ads1115_deinit(xy_ads1115_t *dev)
{
    if (dev == NULL) {
        return XY_ADS1115_INVALID_PARAM;
    }
    dev->initialized = 0U;
    return XY_ADS1115_OK;
}

int xy_ads1115_read_single(xy_ads1115_t *dev, uint8_t channel, int16_t *value)
{
    if (dev == NULL || value == NULL || dev->initialized == 0U || channel > 3U) {
        return XY_ADS1115_INVALID_PARAM;
    }
    return ads1115_convert(dev, (uint16_t)(4U + channel) << 12, value);
}

int xy_ads1115_read_diff(xy_ads1115_t *dev, uint8_t channel_p, uint8_t channel_n, int16_t *value)
{
    uint16_t mux;

    if (dev == NULL || value == NULL || dev->initialized == 0U) {
        return XY_ADS1115_INVALID_PARAM;
    }
    if (channel_p == 0U && channel_n == 1U) {
        mux = ADS1115_CONFIG_MUX_DIFF_0_1;
    } else if (channel_p == 0U && channel_n == 3U) {
        mux = ADS1115_CONFIG_MUX_DIFF_0_3;
    } else if (channel_p == 1U && channel_n == 3U) {
        mux = ADS1115_CONFIG_MUX_DIFF_1_3;
    } else if (channel_p == 2U && channel_n == 3U) {
        mux = ADS1115_CONFIG_MUX_DIFF_2_3;
    } else {
        return XY_ADS1115_INVALID_PARAM;
    }

    return ads1115_convert(dev, mux, value);
}

int xy_ads1115_read_voltage(xy_ads1115_t *dev, uint8_t channel, int32_t *voltage_mv)
{
    int16_t sample;
    int ret;

    if (dev == NULL || voltage_mv == NULL || channel > 3U) {
        return XY_ADS1115_INVALID_PARAM;
    }
    ret = xy_ads1115_read_single(dev, channel, &sample);
    if (ret != XY_ADS1115_OK) {
        return ret;
    }
    *voltage_mv = (int32_t)((float)sample * g_lsb_mv[dev->pga]);
    return XY_ADS1115_OK;
}

int xy_ads1115_set_pga(xy_ads1115_t *dev, xy_ads1115_pga_t pga)
{
    if (dev == NULL || pga > ADS1115_PGA_0_256V) {
        return XY_ADS1115_INVALID_PARAM;
    }
    dev->pga = pga;
    return XY_ADS1115_OK;
}

int xy_ads1115_set_dr(xy_ads1115_t *dev, xy_ads1115_dr_t dr)
{
    if (dev == NULL || dr > ADS1115_DR_860SPS) {
        return XY_ADS1115_INVALID_PARAM;
    }
    dev->dr = dr;
    return XY_ADS1115_OK;
}
