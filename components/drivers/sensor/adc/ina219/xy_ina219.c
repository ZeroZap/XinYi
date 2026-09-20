#include "xy_ina219.h"

#include <limits.h>
#include <string.h>

#define INA219_CALIBRATION_NUMERATOR 40960000000ULL
#define INA219_SHUNT_LSB_UV 10
#define INA219_BUS_LSB_MV 4U

static int ina219_ready(const xy_ina219_t *dev)
{
    return dev != NULL && dev->initialized != 0U && dev->i2c_dev.base.initialized != 0U;
}

static xy_error_t ina219_read_word(xy_ina219_t *dev, uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    xy_error_t result = xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, sizeof(data));

    if (result != XY_DEVICE_OK) {
        return result;
    }
    *value = ((uint16_t)data[0] << 8) | data[1];
    return XY_DEVICE_OK;
}

static xy_error_t ina219_write_word(xy_ina219_t *dev, uint8_t reg, uint16_t value)
{
    uint8_t data[2] = {(uint8_t)(value >> 8), (uint8_t)value};
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, data, sizeof(data));
}

static xy_error_t ina219_read_shunt_staged(xy_ina219_t *dev, int32_t *voltage_uv)
{
    uint16_t value;
    xy_error_t result = ina219_read_word(dev, XY_INA219_REG_SHUNT_VOLTAGE, &value);

    if (result != XY_DEVICE_OK) {
        return result;
    }
    *voltage_uv = (int32_t)(int16_t)value * INA219_SHUNT_LSB_UV;
    return XY_DEVICE_OK;
}

static xy_error_t ina219_read_bus_staged(xy_ina219_t *dev, uint32_t *voltage_mv)
{
    uint16_t value;
    xy_error_t result = ina219_read_word(dev, XY_INA219_REG_BUS_VOLTAGE, &value);

    if (result != XY_DEVICE_OK) {
        return result;
    }
    *voltage_mv = (uint32_t)(value >> 3) * INA219_BUS_LSB_MV;
    return XY_DEVICE_OK;
}

static xy_error_t ina219_read_current_staged(xy_ina219_t *dev, int32_t *current_ua)
{
    uint16_t value;
    int64_t scaled;
    xy_error_t result = ina219_read_word(dev, XY_INA219_REG_CURRENT, &value);

    if (result != XY_DEVICE_OK) {
        return result;
    }
    scaled = (int64_t)(int16_t)value * dev->config.current_lsb_ua;
    if (scaled < INT32_MIN || scaled > INT32_MAX) {
        return XY_ERROR_OVERFLOW;
    }
    *current_ua = (int32_t)scaled;
    return XY_DEVICE_OK;
}

static xy_error_t ina219_read_power_staged(xy_ina219_t *dev, uint32_t *power_uw)
{
    uint16_t value;
    uint64_t scaled;
    xy_error_t result = ina219_read_word(dev, XY_INA219_REG_POWER, &value);

    if (result != XY_DEVICE_OK) {
        return result;
    }
    scaled = (uint64_t)value * 20ULL * dev->config.current_lsb_ua;
    if (scaled > UINT32_MAX) {
        return XY_ERROR_OVERFLOW;
    }
    *power_uw = (uint32_t)scaled;
    return XY_DEVICE_OK;
}

xy_error_t xy_ina219_init(xy_ina219_t *dev, void *i2c_handle, uint8_t addr,
                          const xy_ina219_config_t *config)
{
    uint64_t denominator;
    uint64_t calibration;
    xy_error_t result;

    if (dev == NULL || i2c_handle == NULL || config == NULL || addr < XY_INA219_ADDR_MIN ||
        addr > XY_INA219_ADDR_MAX || config->shunt_resistance_uohm == 0U ||
        config->current_lsb_ua == 0U || config->config_register == 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }

    denominator = (uint64_t)config->current_lsb_ua * config->shunt_resistance_uohm;
    calibration = INA219_CALIBRATION_NUMERATOR / denominator;
    if (calibration == 0U || calibration > UINT16_MAX) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, addr, 1000U);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }

    result = ina219_write_word(dev, XY_INA219_REG_CONFIG, XY_INA219_CONFIG_RESET);
    if (result == XY_DEVICE_OK) {
        result = ina219_write_word(dev, XY_INA219_REG_CONFIG, config->config_register);
    }
    if (result == XY_DEVICE_OK) {
        result = ina219_write_word(dev, XY_INA219_REG_CALIBRATION, (uint16_t)calibration);
    }
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }

    dev->config = *config;
    dev->calibration_register = (uint16_t)calibration;
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_ina219_deinit(xy_ina219_t *dev)
{
    xy_error_t result;

    if (!ina219_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = ina219_write_word(dev, XY_INA219_REG_CONFIG, 0U);
    if (result == XY_DEVICE_OK) {
        dev->initialized = 0U;
        dev->i2c_dev.base.initialized = 0U;
    }
    return result;
}

xy_error_t xy_ina219_read_shunt_voltage(xy_ina219_t *dev, int32_t *voltage_uv)
{
    int32_t next;
    xy_error_t result;

    if (!ina219_ready(dev) || voltage_uv == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = ina219_read_shunt_staged(dev, &next);
    if (result == XY_DEVICE_OK) {
        dev->sample.shunt_voltage_uv = next;
        *voltage_uv = next;
    }
    return result;
}

xy_error_t xy_ina219_read_bus_voltage(xy_ina219_t *dev, uint32_t *voltage_mv)
{
    uint32_t next;
    xy_error_t result;

    if (!ina219_ready(dev) || voltage_mv == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = ina219_read_bus_staged(dev, &next);
    if (result == XY_DEVICE_OK) {
        dev->sample.bus_voltage_mv = next;
        *voltage_mv = next;
    }
    return result;
}

xy_error_t xy_ina219_read_current(xy_ina219_t *dev, int32_t *current_ua)
{
    int32_t next;
    xy_error_t result;

    if (!ina219_ready(dev) || current_ua == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = ina219_read_current_staged(dev, &next);
    if (result == XY_DEVICE_OK) {
        dev->sample.current_ua = next;
        *current_ua = next;
    }
    return result;
}

xy_error_t xy_ina219_read_power(xy_ina219_t *dev, uint32_t *power_uw)
{
    uint32_t next;
    xy_error_t result;

    if (!ina219_ready(dev) || power_uw == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = ina219_read_power_staged(dev, &next);
    if (result == XY_DEVICE_OK) {
        dev->sample.power_uw = next;
        *power_uw = next;
    }
    return result;
}

xy_error_t xy_ina219_read_sample(xy_ina219_t *dev, xy_ina219_sample_t *sample)
{
    xy_ina219_sample_t next;
    xy_error_t result;

    if (!ina219_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = ina219_read_shunt_staged(dev, &next.shunt_voltage_uv);
    if (result == XY_DEVICE_OK) {
        result = ina219_read_bus_staged(dev, &next.bus_voltage_mv);
    }
    if (result == XY_DEVICE_OK) {
        result = ina219_read_current_staged(dev, &next.current_ua);
    }
    if (result == XY_DEVICE_OK) {
        result = ina219_read_power_staged(dev, &next.power_uw);
    }
    if (result == XY_DEVICE_OK) {
        dev->sample = next;
        *sample = next;
    }
    return result;
}
