#include "xy_ap3216c.h"
#include "xy_hal_delay.h"

#include <string.h>

static int xy_ap3216c_mode_valid(uint8_t mode)
{
    return mode == XY_AP3216C_MODE_ALS || mode == XY_AP3216C_MODE_PS ||
           mode == XY_AP3216C_MODE_ALS_PS;
}

static int xy_ap3216c_ready(const xy_ap3216c_t *dev)
{
    return dev != NULL && dev->initialized != 0U && dev->i2c_dev.base.initialized != 0U;
}

xy_error_t xy_ap3216c_init(xy_ap3216c_t *dev, void *i2c_handle, uint8_t address,
                           uint8_t mode)
{
    uint8_t value;
    xy_error_t result;

    if (dev == NULL || i2c_handle == NULL ||
        (address != 0U && address != XY_AP3216C_DEFAULT_ADDRESS) ||
        !xy_ap3216c_mode_valid(mode)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    result = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_AP3216C_DEFAULT_ADDRESS, 100U);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }

    value = XY_AP3216C_MODE_RESET;
    result = xy_i2c_device_write_reg(&dev->i2c_dev, XY_AP3216C_REG_SYSTEM_CONFIG, &value, 1U);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }
    xy_hal_delay_ms(50U);

    value = mode;
    result = xy_i2c_device_write_reg(&dev->i2c_dev, XY_AP3216C_REG_SYSTEM_CONFIG, &value, 1U);
    if (result != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return result;
    }
    xy_hal_delay_ms(50U);

    dev->mode = mode;
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_ap3216c_deinit(xy_ap3216c_t *dev)
{
    uint8_t value = XY_AP3216C_MODE_POWER_DOWN;
    xy_error_t result;

    if (!xy_ap3216c_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = xy_i2c_device_write_reg(&dev->i2c_dev, XY_AP3216C_REG_SYSTEM_CONFIG, &value, 1U);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    return XY_DEVICE_OK;
}

xy_error_t xy_ap3216c_read_light(xy_ap3216c_t *dev, uint32_t *illuminance_millilux)
{
    uint8_t frame[2];
    uint32_t next;
    xy_error_t result;

    if (!xy_ap3216c_ready(dev) || illuminance_millilux == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = xy_i2c_device_read_reg(&dev->i2c_dev, XY_AP3216C_REG_ALS_DATA_L, frame,
                                    sizeof(frame));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    next = (((uint32_t)frame[1] << 8) | frame[0]) * 350U;
    dev->data.illuminance_millilux = next;
    *illuminance_millilux = next;
    return XY_DEVICE_OK;
}

xy_error_t xy_ap3216c_read_proximity(xy_ap3216c_t *dev, uint16_t *proximity_raw)
{
    uint8_t frame[2];
    uint16_t next;
    xy_error_t result;

    if (!xy_ap3216c_ready(dev) || proximity_raw == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = xy_i2c_device_read_reg(&dev->i2c_dev, XY_AP3216C_REG_PS_DATA_L, frame,
                                    sizeof(frame));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    if ((frame[0] & 0x40U) != 0U) {
        return XY_DEVICE_IO_ERROR;
    }
    next = (uint16_t)(((uint16_t)(frame[1] & 0x3FU) << 4) | (frame[0] & 0x0FU));
    dev->data.proximity_raw = next;
    *proximity_raw = next;
    return XY_DEVICE_OK;
}

xy_error_t xy_ap3216c_read_ir(xy_ap3216c_t *dev, uint16_t *infrared_raw)
{
    uint8_t frame[2];
    uint16_t next;
    xy_error_t result;

    if (!xy_ap3216c_ready(dev) || infrared_raw == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    result = xy_i2c_device_read_reg(&dev->i2c_dev, XY_AP3216C_REG_IR_DATA_L, frame,
                                    sizeof(frame));
    if (result != XY_DEVICE_OK) {
        return result;
    }
    next = (uint16_t)(((uint16_t)(frame[1] & 0x03U) << 8) | frame[0]);
    dev->data.infrared_raw = next;
    *infrared_raw = next;
    return XY_DEVICE_OK;
}