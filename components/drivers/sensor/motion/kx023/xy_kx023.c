#include "xy_kx023.h"
#include "xy_device_timing.h"
#include "xy_hal_sys.h"

#include <string.h>

static int transport_ready(const xy_kx023_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized && dev->i2c_dev.i2c_handle != NULL;
}

static int ready(const xy_kx023_t *dev)
{
    return transport_ready(dev) && dev->initialized;
}

static xy_error_t read_reg(xy_kx023_t *dev, uint8_t reg, uint8_t *data, size_t length)
{
    if (!transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, length);
}

static xy_error_t write_reg(xy_kx023_t *dev, uint8_t reg, uint8_t value)
{
    if (!transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

xy_error_t xy_kx023_init(xy_kx023_t *dev, void *i2c_handle)
{
    xy_kx023_t candidate;
    uint8_t id;
    xy_error_t result;
    uint8_t had_live_owner;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    had_live_owner = (uint8_t)(dev->initialized == 1U && transport_ready(dev));
    memset(&candidate, 0, sizeof(candidate));
    result = xy_i2c_device_init(&candidate.i2c_dev, i2c_handle, XY_KX023_ADDR, 1000U);
    if (result != XY_DEVICE_OK || !transport_ready(&candidate)) {
        if (result == XY_DEVICE_OK) {
            result = XY_DEVICE_INVALID_PARAM;
        }
        if (had_live_owner == 0U) {
            memset(dev, 0, sizeof(*dev));
        }
        return result;
    }

    result = read_reg(&candidate, XY_KX023_REG_WHO_AM_I, &id, 1U);
    if (result == XY_DEVICE_OK && id != XY_KX023_WHO_AM_I) {
        result = XY_DEVICE_NOT_FOUND;
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(&candidate, XY_KX023_REG_SOFT_RESET, 0x80U);
    }
    if (result == XY_DEVICE_OK) {
        xy_device_delay_ms(10U);
        result = write_reg(&candidate, XY_KX023_REG_CNTL1, XY_KX023_MODE_STANDBY);
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(&candidate, XY_KX023_REG_ODCNTL, XY_KX023_ODR_12_5HZ);
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(&candidate, XY_KX023_REG_CNTL1, XY_KX023_MODE_LOW_POWER);
    }
    if (result != XY_DEVICE_OK) {
        if (had_live_owner == 0U) {
            memset(dev, 0, sizeof(*dev));
        }
        return result;
    }

    candidate.initialized = 1U;
    *dev = candidate;
    return XY_DEVICE_OK;
}

xy_error_t xy_kx023_deinit(xy_kx023_t *dev)
{
    xy_error_t result;

    if (!ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = write_reg(dev, XY_KX023_REG_CNTL1, XY_KX023_MODE_STANDBY);
    if (result != XY_DEVICE_OK) {
        return result;
    }

    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_kx023_read(xy_kx023_t *dev, xy_kx023_sample_t *sample)
{
    uint8_t data[6];
    xy_kx023_sample_t next;
    xy_error_t result;

    if (!ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = read_reg(dev, XY_KX023_REG_XOUT_L, data, sizeof(data));
    if (result != XY_DEVICE_OK) {
        return result;
    }

    next.raw_x = (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
    next.raw_y = (int16_t)((uint16_t)data[2] | ((uint16_t)data[3] << 8));
    next.raw_z = (int16_t)((uint16_t)data[4] | ((uint16_t)data[5] << 8));
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}
