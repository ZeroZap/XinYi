#include "xy_ist8310.h"
#include "xy_hal_sys.h"

#include <string.h>

static int transport_ready(const xy_ist8310_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized && dev->i2c_dev.i2c_handle != NULL;
}

static int ready(const xy_ist8310_t *dev)
{
    return transport_ready(dev) && dev->initialized;
}

static xy_error_t read_reg(xy_ist8310_t *dev, uint8_t reg, uint8_t *data, size_t length)
{
    if (!transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, length);
}

static xy_error_t write_reg(xy_ist8310_t *dev, uint8_t reg, uint8_t value)
{
    if (!transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

xy_error_t xy_ist8310_init(xy_ist8310_t *dev, void *i2c_handle)
{
    uint8_t id;
    xy_error_t result;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_IST8310_ADDR, 1000U);
    if (result != XY_DEVICE_OK || !transport_ready(dev)) {
        memset(dev, 0, sizeof(*dev));
        return result != XY_DEVICE_OK ? result : XY_DEVICE_INVALID_PARAM;
    }

    result = read_reg(dev, XY_IST8310_REG_WHOAMI, &id, 1U);
    if (result == XY_DEVICE_OK && id != XY_IST8310_WHOAMI) {
        result = XY_DEVICE_NOT_FOUND;
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(dev, XY_IST8310_REG_CTRL1, XY_IST8310_CTRL1_CONTINUOUS_100HZ);
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(dev, XY_IST8310_REG_CTRL2, XY_IST8310_CTRL2_ENABLE);
    }
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }

    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_ist8310_deinit(xy_ist8310_t *dev)
{
    xy_error_t result;

    if (!ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = write_reg(dev, XY_IST8310_REG_CTRL1, 0U);
    if (result != XY_DEVICE_OK) {
        return result;
    }

    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_ist8310_read(xy_ist8310_t *dev, xy_ist8310_sample_t *sample)
{
    uint8_t data[6];
    xy_ist8310_sample_t next;
    xy_error_t result;

    if (!ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = read_reg(dev, XY_IST8310_REG_DATA, data, sizeof(data));
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
