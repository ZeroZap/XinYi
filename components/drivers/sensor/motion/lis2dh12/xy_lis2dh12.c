#include "xy_lis2dh12.h"
#include "xy_hal_sys.h"

#include <string.h>

static int transport_ready(const xy_lis2dh12_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized && dev->i2c_dev.i2c_handle != NULL;
}

static int ready(const xy_lis2dh12_t *dev)
{
    return transport_ready(dev) && dev->initialized;
}

static xy_error_t write_reg(xy_lis2dh12_t *dev, uint8_t reg, uint8_t value)
{
    if (!transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

xy_error_t xy_lis2dh12_init(xy_lis2dh12_t *dev, void *i2c_handle)
{
    uint8_t id;
    xy_error_t result;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_LIS2DH12_ADDR, 1000U);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }
    if (!transport_ready(dev)) {
        memset(dev, 0, sizeof(*dev));
        return XY_DEVICE_INVALID_PARAM;
    }

    result = xy_i2c_device_read_reg(&dev->i2c_dev, XY_LIS2DH12_REG_WHO_AM_I, &id, 1U);
    if (result == XY_DEVICE_OK && id != XY_LIS2DH12_WHO_AM_I) {
        result = XY_DEVICE_NOT_FOUND;
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(dev, XY_LIS2DH12_REG_CTRL1, XY_LIS2DH12_CTRL1_10HZ_XYZ);
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(dev, XY_LIS2DH12_REG_CTRL4, XY_LIS2DH12_CTRL4_HR_2G);
    }
    if (result == XY_DEVICE_OK) {
        result = write_reg(dev, XY_LIS2DH12_REG_TEMP_CFG, XY_LIS2DH12_TEMP_ENABLE);
    }
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }

    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_lis2dh12_deinit(xy_lis2dh12_t *dev)
{
    xy_error_t result;

    if (!ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = write_reg(dev, XY_LIS2DH12_REG_CTRL1, 0U);
    if (result != XY_DEVICE_OK) {
        return result;
    }

    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_lis2dh12_read(xy_lis2dh12_t *dev, xy_lis2dh12_sample_t *sample)
{
    uint8_t data[6];
    xy_lis2dh12_sample_t next;
    xy_error_t result;

    if (!ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    result = xy_i2c_device_read_reg(&dev->i2c_dev,
                                    XY_LIS2DH12_REG_OUT_X_L | XY_LIS2DH12_AUTO_INCREMENT, data,
                                    sizeof(data));
    if (result != XY_DEVICE_OK) {
        return result;
    }

    next.raw_x = (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8)) >> 4;
    next.raw_y = (int16_t)((uint16_t)data[2] | ((uint16_t)data[3] << 8)) >> 4;
    next.raw_z = (int16_t)((uint16_t)data[4] | ((uint16_t)data[5] << 8)) >> 4;
    next.x_mg = next.raw_x;
    next.y_mg = next.raw_y;
    next.z_mg = next.raw_z;
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}
