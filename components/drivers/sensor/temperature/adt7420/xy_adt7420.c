#include "xy_adt7420.h"

#include <string.h>

static int adt7420_transport_ready(const xy_adt7420_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized &&
           dev->i2c_dev.i2c_handle != NULL;
}

static int adt7420_ready(const xy_adt7420_t *dev)
{
    return adt7420_transport_ready(dev) && dev->initialized != 0U;
}

static xy_error_t adt7420_read_reg(xy_adt7420_t *dev, uint8_t reg, uint8_t *data,
                                   size_t len)
{
    if (!adt7420_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, len);
}

static xy_error_t adt7420_write_reg(xy_adt7420_t *dev, uint8_t reg, uint8_t value)
{
    if (!adt7420_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

xy_error_t xy_adt7420_init(xy_adt7420_t *dev, void *i2c_handle, uint8_t address)
{
    xy_adt7420_t candidate;
    uint8_t id;
    uint8_t addr;
    xy_error_t ret;
    int preserve_live_owner;

    if (dev == NULL || i2c_handle == NULL ||
        (address != 0U &&
         (address < XY_ADT7420_ADDR_DEFAULT || address > XY_ADT7420_ADDR_ALT3))) {
        return XY_DEVICE_INVALID_PARAM;
    }

    preserve_live_owner = dev->initialized == 1U &&
                          dev->i2c_dev.base.initialized == 1U &&
                          dev->i2c_dev.i2c_handle != NULL;
    memset(&candidate, 0, sizeof(candidate));
    addr = (address == 0U) ? XY_ADT7420_ADDR_DEFAULT : address;
    ret = xy_i2c_device_init(&candidate.i2c_dev, i2c_handle, addr, 1000U);
    if (ret != XY_DEVICE_OK || !adt7420_transport_ready(&candidate)) {
        if (!preserve_live_owner) {
            memset(dev, 0, sizeof(*dev));
        }
        return ret != XY_DEVICE_OK ? ret : XY_DEVICE_INVALID_PARAM;
    }
    ret = adt7420_read_reg(&candidate, XY_ADT7420_REG_ID, &id, 1U);
    if (ret != XY_DEVICE_OK) {
        if (!preserve_live_owner) {
            memset(dev, 0, sizeof(*dev));
        }
        return ret;
    }
    if (id != XY_ADT7420_ID_VALUE) {
        if (!preserve_live_owner) {
            memset(dev, 0, sizeof(*dev));
        }
        return XY_DEVICE_NOT_FOUND;
    }
    ret = adt7420_write_reg(&candidate, XY_ADT7420_REG_CONFIG,
                            XY_ADT7420_CONFIG_RES_16BIT);
    if (ret != XY_DEVICE_OK) {
        if (!preserve_live_owner) {
            memset(dev, 0, sizeof(*dev));
        }
        return ret;
    }
    candidate.initialized = 1U;
    *dev = candidate;
    return XY_DEVICE_OK;
}

xy_error_t xy_adt7420_deinit(xy_adt7420_t *dev)
{
    if (!adt7420_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_adt7420_read(xy_adt7420_t *dev, xy_adt7420_data_t *out)
{
    uint8_t buf[2];
    int16_t raw;
    xy_adt7420_data_t next;
    xy_error_t ret;

    if (!adt7420_ready(dev) || out == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = adt7420_read_reg(dev, XY_ADT7420_REG_TEMP_MSB, buf, sizeof(buf));
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    raw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    /* 16-bit resolution: 0.0078125 C/LSB (raw / 128), in centi-C */
    next.temperature_centi_c = (int32_t)(((int64_t)raw * 100) / 128);
    dev->data = next;
    *out = next;
    return XY_DEVICE_OK;
}
