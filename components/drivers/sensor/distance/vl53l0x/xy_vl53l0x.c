#include "xy_vl53l0x.h"
#include "xy_device_timing.h"
#include "xy_hal_sys.h"

#include <string.h>

static int vl53l0x_transport_ready(const xy_vl53l0x_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized != 0U &&
           dev->i2c_dev.i2c_handle != NULL;
}

static int vl53l0x_ready(const xy_vl53l0x_t *dev)
{
    return vl53l0x_transport_ready(dev) && dev->initialized != 0U;
}

static xy_error_t vl53l0x_read_reg(xy_vl53l0x_t *dev, uint8_t reg, uint8_t *data, size_t length)
{
    if (!vl53l0x_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, length);
}

static xy_error_t vl53l0x_write_reg(xy_vl53l0x_t *dev, uint8_t reg, uint8_t value)
{
    if (!vl53l0x_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

xy_error_t xy_vl53l0x_init(xy_vl53l0x_t *dev, void *i2c_handle)
{
    uint8_t model;
    xy_error_t ret;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(dev, 0, sizeof(*dev));
    ret = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_VL53L0X_ADDR, 1000U);
    if (ret != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return ret;
    }
    if (!vl53l0x_transport_ready(dev)) {
        memset(dev, 0, sizeof(*dev));
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = vl53l0x_read_reg(dev, XY_VL53L0X_REG_MODEL_ID, &model, 1U);
    if (ret == XY_DEVICE_OK && model != XY_VL53L0X_MODEL_ID) {
        ret = XY_DEVICE_NOT_FOUND;
    }
    if (ret != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return ret;
    }
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_vl53l0x_deinit(xy_vl53l0x_t *dev)
{
    if (!vl53l0x_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_vl53l0x_read(xy_vl53l0x_t *dev, xy_vl53l0x_sample_t *sample)
{
    uint8_t start = 1U;
    uint8_t range[12];
    xy_vl53l0x_sample_t next;
    xy_error_t ret;

    if (!vl53l0x_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = vl53l0x_write_reg(dev, XY_VL53L0X_REG_SYSRANGE_START, start);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    xy_device_delay_ms(50U);
    ret = vl53l0x_read_reg(dev, XY_VL53L0X_REG_RANGE_STATUS, range, sizeof(range));
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    next.distance_mm = (uint16_t)(((uint16_t)range[10] << 8) | range[11]);
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}
