#include "xy_as5600.h"
#include "xy_hal_sys.h"

#include <string.h>

static int as5600_transport_ready(const xy_as5600_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized &&
           dev->i2c_dev.i2c_handle != NULL;
}

static int as5600_ready(const xy_as5600_t *dev)
{
    return as5600_transport_ready(dev) && dev->initialized;
}

static xy_error_t as5600_read_angle(xy_as5600_t *dev, uint8_t data[2])
{
    if (!as5600_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, XY_AS5600_REG_ANGLE_H, data, 2U);
}

xy_error_t xy_as5600_init(xy_as5600_t *dev, void *i2c_handle)
{
    xy_as5600_t candidate;
    xy_error_t ret;
    int preserve_live_owner;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    preserve_live_owner = dev->initialized == 1U && dev->i2c_dev.base.initialized == 1U &&
                          dev->i2c_dev.i2c_handle != NULL;
    memset(&candidate, 0, sizeof(candidate));
    ret = xy_i2c_device_init(&candidate.i2c_dev, i2c_handle, XY_AS5600_ADDR, 1000U);
    if (ret != XY_DEVICE_OK || !as5600_transport_ready(&candidate)) {
        if (!preserve_live_owner) {
            memset(dev, 0, sizeof(*dev));
        }
        return ret != XY_DEVICE_OK ? ret : XY_DEVICE_INVALID_PARAM;
    }
    candidate.initialized = 1U;
    *dev = candidate;
    return XY_DEVICE_OK;
}

xy_error_t xy_as5600_deinit(xy_as5600_t *dev)
{
    if (!as5600_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_as5600_read(xy_as5600_t *dev, xy_as5600_sample_t *sample)
{
    uint8_t bytes[2];
    xy_as5600_sample_t next;
    xy_error_t ret;

    if (!as5600_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = as5600_read_angle(dev, bytes);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    next = dev->sample;
    next.angle_raw = (((uint16_t)bytes[0] << 8) | bytes[1]) & 0x0FFFU;
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}
