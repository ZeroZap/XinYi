#include "xy_vcnl4040.h"
#include "xy_hal_sys.h"

#include <string.h>

static int vcnl4040_transport_ready(const xy_vcnl4040_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized &&
           dev->i2c_dev.i2c_handle != NULL;
}

static int vcnl4040_ready(const xy_vcnl4040_t *dev)
{
    return vcnl4040_transport_ready(dev) && dev->initialized;
}

static xy_error_t vcnl4040_read_reg(xy_vcnl4040_t *dev, uint8_t reg, uint8_t *data,
                                    size_t length)
{
    if (!vcnl4040_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, length);
}

xy_error_t xy_vcnl4040_init(xy_vcnl4040_t *dev, void *i2c_handle)
{
    xy_error_t ret;
    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(dev, 0, sizeof(*dev));
    ret = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_VCNL4040_ADDR, 1000U);
    if (ret != XY_DEVICE_OK || !vcnl4040_transport_ready(dev)) {
        memset(dev, 0, sizeof(*dev));
        return ret != XY_DEVICE_OK ? ret : XY_DEVICE_INVALID_PARAM;
    }
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_vcnl4040_deinit(xy_vcnl4040_t *dev)
{
    if (!vcnl4040_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_vcnl4040_read(xy_vcnl4040_t *dev, xy_vcnl4040_sample_t *sample)
{
    uint8_t bytes[2];
    xy_vcnl4040_sample_t next;
    xy_error_t ret;

    if (!vcnl4040_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = vcnl4040_read_reg(dev, XY_VCNL4040_REG_PS_DATA_L, bytes, 2U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    next.proximity_raw = (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}
