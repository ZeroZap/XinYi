#include "xy_as5048b.h"
#include "xy_hal_sys.h"

#include <string.h>

static int as5048b_ready(const xy_as5048b_t *dev)
{
    return dev != NULL && dev->initialized && dev->i2c_dev.base.initialized &&
           dev->i2c_dev.i2c_handle != NULL;
}

xy_error_t xy_as5048b_init(xy_as5048b_t *dev, void *i2c_handle)
{
    xy_error_t ret;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(dev, 0, sizeof(*dev));
    ret = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_AS5048B_ADDR, 1000U);
    if (ret != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return ret;
    }
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_as5048b_deinit(xy_as5048b_t *dev)
{
    if (!as5048b_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_as5048b_read(xy_as5048b_t *dev, xy_as5048b_sample_t *sample)
{
    uint8_t bytes[2];
    xy_as5048b_sample_t next;
    xy_error_t ret;

    if (!as5048b_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = xy_i2c_device_read_reg(&dev->i2c_dev, XY_AS5048B_REG_ANGLE_MSB, bytes, 2U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    next = dev->sample;
    next.angle_raw = (((uint16_t)bytes[0] << 6) | (bytes[1] & 0x3FU)) & 0x3FFFU;
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}
