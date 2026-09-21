#include "xy_max44009.h"
#include "xy_hal_sys.h"

#include <string.h>

static int max44009_ready(const xy_max44009_t *dev)
{
    return dev != NULL && dev->initialized && dev->i2c_dev.base.initialized &&
           dev->i2c_dev.i2c_handle != NULL;
}

static int max44009_valid_addr(uint8_t addr)
{
    return addr == XY_MAX44009_ADDR_LOW || addr == XY_MAX44009_ADDR_HIGH;
}

xy_error_t xy_max44009_init(xy_max44009_t *dev, void *i2c_handle, uint8_t addr)
{
    xy_error_t ret;
    if (dev == NULL || i2c_handle == NULL || !max44009_valid_addr(addr)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(dev, 0, sizeof(*dev));
    ret = xy_i2c_device_init(&dev->i2c_dev, i2c_handle, addr, 1000U);
    if (ret != XY_DEVICE_OK) {
        memset(dev, 0, sizeof(*dev));
        return ret;
    }
    dev->initialized = 1U;
    return XY_DEVICE_OK;
}

xy_error_t xy_max44009_deinit(xy_max44009_t *dev)
{
    if (!max44009_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_max44009_read(xy_max44009_t *dev, xy_max44009_sample_t *sample)
{
    uint8_t bytes[2];
    xy_max44009_sample_t next;
    xy_error_t ret;
    uint8_t exponent;
    uint16_t mantissa;
    uint32_t lux_milli;

    if (!max44009_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = xy_i2c_device_read_reg(&dev->i2c_dev, XY_MAX44009_REG_LUX_HIGH, bytes, 1U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    ret = xy_i2c_device_read_reg(&dev->i2c_dev, XY_MAX44009_REG_LUX_LOW, &bytes[1], 1U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    exponent = (uint8_t)(bytes[0] >> 4);
    mantissa = (uint16_t)(((bytes[0] & 0x0FU) << 4) | (bytes[1] & 0x0FU));
    lux_milli = (uint32_t)mantissa * 45U;
    if (exponent < 31U) {
        lux_milli <<= exponent;
    }
    next.illuminance_mlux = lux_milli;
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}
