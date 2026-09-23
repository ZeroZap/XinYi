#include "xy_apds9960.h"

#include "xy_hal_sys.h"

#include <string.h>

static int apds9960_ready(const xy_apds9960_t *dev)
{
    return dev != NULL && dev->initialized && dev->i2c_dev.base.initialized &&
           dev->i2c_dev.i2c_handle != NULL;
}

static uint16_t apds9960_decode_le16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

xy_error_t xy_apds9960_init(xy_apds9960_t *dev, void *i2c_handle)
{
    uint8_t id;
    uint8_t enable = XY_APDS9960_ENABLE_PON_AEN_PEN_GEN;
    xy_error_t error;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    error = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_APDS9960_ADDR, 1000U);
    if (error != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return error;
    }

    error = xy_i2c_device_read_reg(&dev->i2c_dev, XY_APDS9960_REG_ID, &id, 1U);
    if (error == XY_DEVICE_OK && id != 0xABU && id != 0x9CU) {
        error = XY_DEVICE_NOT_FOUND;
    }
    if (error == XY_DEVICE_OK) {
        error = xy_i2c_device_write_reg(&dev->i2c_dev, XY_APDS9960_REG_ENABLE, &enable, 1U);
    }
    if (error != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return error;
    }

    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_apds9960_deinit(xy_apds9960_t *dev)
{
    uint8_t enable = 0U;
    xy_error_t error;

    if (!apds9960_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    error = xy_i2c_device_write_reg(&dev->i2c_dev, XY_APDS9960_REG_ENABLE, &enable, 1U);
    if (error != XY_DEVICE_OK) {
        return error;
    }

    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_apds9960_read_rgb(xy_apds9960_t *dev, xy_apds9960_rgb_t *sample)
{
    uint8_t data[8];
    xy_apds9960_rgb_t next;
    xy_error_t error;

    if (!apds9960_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    error = xy_i2c_device_read_reg(&dev->i2c_dev, XY_APDS9960_REG_CDATAL, data, sizeof(data));
    if (error != XY_DEVICE_OK) {
        return error;
    }

    next.clear = apds9960_decode_le16(data);
    next.red = apds9960_decode_le16(data + 2);
    next.green = apds9960_decode_le16(data + 4);
    next.blue = apds9960_decode_le16(data + 6);
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->rgb = next;
    return XY_DEVICE_OK;
}

xy_error_t xy_apds9960_read_proximity(xy_apds9960_t *dev,
                                      xy_apds9960_proximity_t *sample)
{
    xy_apds9960_proximity_t next;
    xy_error_t error;

    if (!apds9960_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    error = xy_i2c_device_read_reg(&dev->i2c_dev, XY_APDS9960_REG_PDATA, &next.proximity, 1U);
    if (error != XY_DEVICE_OK) {
        return error;
    }

    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->proximity = next;
    return XY_DEVICE_OK;
}

xy_error_t xy_apds9960_read_gesture_fifo(xy_apds9960_t *dev,
                                         xy_apds9960_gesture_fifo_t *sample)
{
    uint8_t status;
    uint8_t level;
    xy_apds9960_gesture_fifo_t next;
    xy_error_t error;

    if (!apds9960_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(&next, 0, sizeof(next));
    error = xy_i2c_device_read_reg(&dev->i2c_dev, XY_APDS9960_REG_GSTATUS, &status, 1U);
    if (error != XY_DEVICE_OK) {
        return error;
    }

    if ((status & XY_APDS9960_GVALID) != 0U) {
        error = xy_i2c_device_read_reg(&dev->i2c_dev, XY_APDS9960_REG_GFLVL, &level, 1U);
        if (error != XY_DEVICE_OK) {
            return error;
        }
        if (level > 32U) {
            return XY_DEVICE_INVALID_PARAM;
        }
        next.level = level;
        if (level != 0U) {
            error = xy_i2c_device_read_reg(&dev->i2c_dev, XY_APDS9960_REG_GFIFO_U, next.data,
                                           (size_t)level * 4U);
            if (error != XY_DEVICE_OK) {
                return error;
            }
        }
    }

    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->gesture = next;
    return XY_DEVICE_OK;
}
