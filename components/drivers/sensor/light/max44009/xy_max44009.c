#include "xy_max44009.h"
#include "xy_hal_sys.h"

#include <string.h>

static int max44009_transport_ready(const xy_max44009_t* dev) {
    return dev != NULL && dev->i2c_dev.base.initialized && dev->i2c_dev.i2c_handle != NULL;
}

static int max44009_ready(const xy_max44009_t* dev) {
    return max44009_transport_ready(dev) && dev->initialized;
}

static xy_error_t max44009_read_reg(xy_max44009_t* dev, uint8_t reg, uint8_t* data) {
    if (!max44009_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, 1U);
}

static int max44009_valid_addr(uint8_t addr) {
    return addr == XY_MAX44009_ADDR_LOW || addr == XY_MAX44009_ADDR_HIGH;
}

static int max44009_live_owner(const xy_max44009_t* dev) {
    return max44009_ready(dev) && max44009_valid_addr(dev->i2c_dev.dev_addr);
}

xy_error_t xy_max44009_init(xy_max44009_t* dev, void* i2c_handle, uint8_t addr) {
    xy_max44009_t candidate;
    xy_error_t ret;
    int preserve_live_owner;

    if (dev == NULL || i2c_handle == NULL || !max44009_valid_addr(addr)) {
        return XY_DEVICE_INVALID_PARAM;
    }

    preserve_live_owner = max44009_live_owner(dev);
    memset(&candidate, 0, sizeof(candidate));
    ret = xy_i2c_device_init(&candidate.i2c_dev, i2c_handle, addr, 1000U);
    if (ret != XY_DEVICE_OK || !max44009_transport_ready(&candidate)) {
        if (!preserve_live_owner) {
            memset(dev, 0, sizeof(*dev));
        }
        return ret != XY_DEVICE_OK ? ret : XY_DEVICE_INVALID_PARAM;
    }
    candidate.initialized = 1U;
    *dev = candidate;
    return XY_DEVICE_OK;
}

xy_error_t xy_max44009_deinit(xy_max44009_t* dev) {
    if (!max44009_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_max44009_read(xy_max44009_t* dev, xy_max44009_sample_t* sample) {
    uint8_t bytes[2];
    xy_max44009_sample_t next;
    xy_error_t ret;
    uint8_t exponent;
    uint16_t mantissa;
    uint32_t lux_milli;

    if (!max44009_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = max44009_read_reg(dev, XY_MAX44009_REG_LUX_HIGH, bytes);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    ret = max44009_read_reg(dev, XY_MAX44009_REG_LUX_LOW, &bytes[1]);
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
