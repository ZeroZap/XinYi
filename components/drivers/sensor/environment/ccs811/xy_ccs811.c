#include "xy_ccs811.h"

#include "xy_hal_sys.h"

#include <string.h>

static int ccs811_transport_ready(const xy_ccs811_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized && dev->i2c_dev.i2c_handle != NULL;
}

static int ccs811_ready(const xy_ccs811_t *dev)
{
    return ccs811_transport_ready(dev) && dev->initialized;
}

xy_error_t xy_ccs811_init(xy_ccs811_t *dev, void *i2c_handle)
{
    uint8_t id;
    uint8_t command = XY_CCS811_APP_START;
    uint8_t mode = 0x10U;
    xy_error_t error;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    memset(dev, 0, sizeof(*dev));
    error = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, XY_CCS811_ADDR, 1000U);
    if (error != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return error;
    }
    if (!ccs811_transport_ready(dev)) {
        memset(dev, 0, sizeof(*dev));
        return XY_DEVICE_INVALID_PARAM;
    }

    error = xy_i2c_device_read_reg(&dev->i2c_dev, XY_CCS811_REG_HW_ID, &id, 1U);
    if (error == XY_DEVICE_OK && id != XY_CCS811_HW_ID_VALUE) {
        error = XY_DEVICE_NOT_FOUND;
    }
    if (error == XY_DEVICE_OK) {
        error = xy_i2c_device_write(&dev->i2c_dev, &command, 1U);
    }
    if (error == XY_DEVICE_OK) {
        error = xy_i2c_device_write_reg(&dev->i2c_dev, XY_CCS811_REG_MEAS_MODE, &mode, 1U);
    }
    if (error != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return error;
    }

    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_ccs811_deinit(xy_ccs811_t *dev)
{
    uint8_t mode = 0U;
    xy_error_t error;

    if (!ccs811_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    error = xy_i2c_device_write_reg(&dev->i2c_dev, XY_CCS811_REG_MEAS_MODE, &mode, 1U);
    if (error != XY_DEVICE_OK) {
        return error;
    }

    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_ccs811_read(xy_ccs811_t *dev, xy_ccs811_sample_t *sample)
{
    uint8_t status;
    uint8_t data[8];
    xy_ccs811_sample_t next;
    xy_error_t error;

    if (!ccs811_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    error = xy_i2c_device_read_reg(&dev->i2c_dev, XY_CCS811_REG_STATUS, &status, 1U);
    if (error != XY_DEVICE_OK) {
        return error;
    }
    if ((status & XY_CCS811_STATUS_DATA_READY) == 0U) {
        return XY_DEVICE_BUSY;
    }

    error = xy_i2c_device_read_reg(&dev->i2c_dev, XY_CCS811_REG_ALG_RESULT, data,
                                   sizeof(data));
    if (error != XY_DEVICE_OK) {
        return error;
    }

    next.eco2_ppm = ((uint16_t)data[0] << 8) | data[1];
    next.tvoc_ppb = ((uint16_t)data[2] << 8) | data[3];
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}
