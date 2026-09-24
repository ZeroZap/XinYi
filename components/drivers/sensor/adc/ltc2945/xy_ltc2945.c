#include "xy_ltc2945.h"
#include "xy_hal_sys.h"

#include <string.h>

static bool ltc2945_transport_ready(const xy_ltc2945_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized &&
           dev->i2c_dev.i2c_handle != NULL;
}

static bool ltc2945_ready(const xy_ltc2945_t *dev)
{
    return ltc2945_transport_ready(dev) && dev->initialized;
}

static bool ltc2945_config_valid(uint8_t address, const xy_ltc2945_config_t *config)
{
    return config != NULL && address >= XY_LTC2945_ADDR_MIN && address <= XY_LTC2945_ADDR_MAX &&
           config->shunt_resistance_uohm != 0U;
}

static int ltc2945_read_u8(xy_ltc2945_t *dev, uint8_t reg, uint8_t *value)
{
    if (!ltc2945_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, value, 1U);
}

static int ltc2945_read_u12(xy_ltc2945_t *dev, uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    int ret;

    if (!ltc2945_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, sizeof(data));

    if (ret == XY_DEVICE_OK) {
        *value = ((uint16_t)data[0] << 4) | ((uint16_t)data[1] >> 4);
    }
    return ret;
}

static int ltc2945_read_u24(xy_ltc2945_t *dev, uint8_t reg, uint32_t *value)
{
    uint8_t data[3];
    int ret;

    if (!ltc2945_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, sizeof(data));

    if (ret == XY_DEVICE_OK) {
        *value = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
    }
    return ret;
}

static int ltc2945_write_u8(xy_ltc2945_t *dev, uint8_t reg, uint8_t value)
{
    if (!ltc2945_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

int xy_ltc2945_init(xy_ltc2945_t *dev, void *i2c_handle, uint8_t address,
                    const xy_ltc2945_config_t *config)
{
    int ret;
    uint8_t status;

    if (dev == NULL || i2c_handle == NULL || !ltc2945_config_valid(address, config)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    ret = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, address, 1000U);
    if (ret != XY_DEVICE_OK || !ltc2945_transport_ready(dev)) {
        memset(dev, 0, sizeof(*dev));
        return ret != XY_DEVICE_OK ? ret : XY_DEVICE_INVALID_PARAM;
    }

    ret = ltc2945_read_u8(dev, XY_LTC2945_REG_STATUS, &status);
    if (ret != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return ret;
    }
    ret = ltc2945_write_u8(dev, XY_LTC2945_REG_CONTROL, config->control_register);
    if (ret != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return ret;
    }
    ret = ltc2945_write_u8(dev, XY_LTC2945_REG_ALERT, config->alert_register);
    if (ret != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return ret;
    }

    dev->config = *config;
    dev->sample.status = status;
    dev->initialized = true;
    return XY_DEVICE_OK;
}

int xy_ltc2945_deinit(xy_ltc2945_t *dev)
{
    if (!ltc2945_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->initialized = false;
    dev->i2c_dev.base.initialized = false;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

int xy_ltc2945_read_sample(xy_ltc2945_t *dev, xy_ltc2945_sample_t *sample)
{
    xy_ltc2945_sample_t next;
    uint16_t vin_raw;
    uint16_t sense_raw;
    uint32_t power_raw;
    int ret;

    if (!ltc2945_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    next = dev->sample;
    ret = ltc2945_read_u12(dev, XY_LTC2945_REG_VIN_MSB, &vin_raw);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    ret = ltc2945_read_u12(dev, XY_LTC2945_REG_SENSE_MSB, &sense_raw);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    ret = ltc2945_read_u24(dev, XY_LTC2945_REG_POWER_MSB, &power_raw);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    ret = ltc2945_read_u8(dev, XY_LTC2945_REG_STATUS, &next.status);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    ret = ltc2945_read_u8(dev, XY_LTC2945_REG_FAULT, &next.fault);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    next.bus_voltage_mv = (uint32_t)vin_raw * 25U;
    next.shunt_voltage_uv = (uint32_t)sense_raw * 25U;
    next.current_ua = (uint32_t)(((uint64_t)next.shunt_voltage_uv * 1000000ULL) /
                                 dev->config.shunt_resistance_uohm);
    if ((dev->config.control_register & XY_LTC2945_CONTROL_MULT_SELECT) != 0U) {
        next.power_uw = (uint32_t)(((uint64_t)power_raw * 625000ULL) /
                                  dev->config.shunt_resistance_uohm);
    } else {
        next.power_uw = (uint32_t)(((uint64_t)power_raw * 12500ULL) /
                                  dev->config.shunt_resistance_uohm);
    }
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}

int xy_ltc2945_set_alert_mask(xy_ltc2945_t *dev, uint8_t alert_mask)
{
    int ret;

    if (!ltc2945_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = ltc2945_write_u8(dev, XY_LTC2945_REG_ALERT, alert_mask);
    if (ret == XY_DEVICE_OK) {
        dev->config.alert_register = alert_mask;
    }
    return ret;
}

int xy_ltc2945_clear_faults(xy_ltc2945_t *dev, uint8_t fault_mask)
{
    if (!ltc2945_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return ltc2945_write_u8(dev, XY_LTC2945_REG_FAULT, fault_mask);
}
