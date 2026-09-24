#include "xy_ak09918.h"
#include "xy_device_timing.h"
#include "xy_hal_sys.h"

#include <string.h>

static int ak09918_transport_ready(const xy_ak09918_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized &&
           dev->i2c_dev.i2c_handle != NULL;
}

static int ak09918_ready(const xy_ak09918_t *dev)
{
    return ak09918_transport_ready(dev) && dev->initialized != 0U;
}

static xy_error_t ak09918_read_reg(xy_ak09918_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    if (!ak09918_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, len);
}

static xy_error_t ak09918_write_reg(xy_ak09918_t *dev, uint8_t reg, uint8_t value)
{
    if (!ak09918_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

xy_error_t xy_ak09918_init(xy_ak09918_t *dev, void *i2c_handle)
{
    uint8_t id[2];
    xy_error_t ret;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(dev, 0, sizeof(*dev));
    ret = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_AK09918_ADDR, 1000U);
    if (ret != XY_DEVICE_OK || !ak09918_transport_ready(dev)) {
        memset(dev, 0, sizeof(*dev));
        return ret != XY_DEVICE_OK ? ret : XY_DEVICE_INVALID_PARAM;
    }
    ret = ak09918_read_reg(dev, XY_AK09918_REG_WIA1, id, sizeof(id));
    if (ret != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return ret;
    }
    if (id[0] != XY_AK09918_WIA1 || id[1] != XY_AK09918_WIA2) {
        memset(dev, 0, sizeof(*dev));
        return XY_DEVICE_NOT_FOUND;
    }
    ret = ak09918_write_reg(dev, XY_AK09918_REG_CNTL3, XY_AK09918_RESET);
    if (ret == XY_DEVICE_OK) {
        xy_device_delay_ms(1U);
        ret = ak09918_write_reg(dev, XY_AK09918_REG_CNTL2, XY_AK09918_MODE_CONTINUOUS_100HZ);
    }
    if (ret != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return ret;
    }
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_ak09918_deinit(xy_ak09918_t *dev)
{
    xy_error_t ret;

    if (!ak09918_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = ak09918_write_reg(dev, XY_AK09918_REG_CNTL2, XY_AK09918_MODE_POWER_DOWN);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_ak09918_read(xy_ak09918_t *dev, xy_ak09918_sample_t *sample)
{
    uint8_t status;
    uint8_t bytes[6];
    xy_ak09918_sample_t next;
    xy_error_t ret;

    if (!ak09918_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = ak09918_read_reg(dev, XY_AK09918_REG_ST1, &status, 1U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    if ((status & 0x01U) == 0U) {
        return XY_DEVICE_BUSY;
    }
    ret = ak09918_read_reg(dev, XY_AK09918_REG_HXL, bytes, sizeof(bytes));
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    next.raw_x = (int16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
    next.raw_y = (int16_t)((uint16_t)bytes[2] | ((uint16_t)bytes[3] << 8));
    next.raw_z = (int16_t)((uint16_t)bytes[4] | ((uint16_t)bytes[5] << 8));
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}
