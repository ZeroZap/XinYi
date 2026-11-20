#include "xy_cmm905.h"
#include "xy_hal_sys.h"

#include <string.h>

static int cmm905_transport_ready(const xy_cmm905_t *dev)
{
    return dev != NULL && dev->i2c_dev.base.initialized && dev->i2c_dev.i2c_handle != NULL;
}

static int cmm905_ready(const xy_cmm905_t *dev)
{
    return cmm905_transport_ready(dev) && dev->initialized;
}

static xy_error_t cmm905_read_reg(xy_cmm905_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    if (!cmm905_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, len);
}

static xy_error_t cmm905_write_u8(xy_cmm905_t *dev, uint8_t reg, uint8_t value)
{
    if (!cmm905_transport_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1U);
}

xy_error_t xy_cmm905_init(xy_cmm905_t *dev, void *i2c_handle)
{
    xy_cmm905_t candidate;
    const int preserve_live_owner = cmm905_ready(dev) && dev->i2c_dev.dev_addr == XY_CMM905_ADDR;
    xy_error_t ret;

    if (dev == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(&candidate, 0, sizeof(candidate));
    ret = xy_i2c_device_init(&candidate.i2c_dev, i2c_handle, XY_CMM905_ADDR, 1000U);
    if (ret != XY_DEVICE_OK || !cmm905_transport_ready(&candidate)) {
        if (!preserve_live_owner) {
            memset(dev, 0, sizeof(*dev));
        }
        return ret != XY_DEVICE_OK ? ret : XY_DEVICE_INVALID_PARAM;
    }
    candidate.initialized = 1U;
    *dev = candidate;
    return XY_DEVICE_OK;
}

xy_error_t xy_cmm905_deinit(xy_cmm905_t *dev)
{
    xy_error_t ret;

    if (!cmm905_ready(dev)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    ret = cmm905_write_u8(dev, XY_CMM905_REG_STATUS, 0x00U);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    dev->initialized = 0U;
    dev->i2c_dev.base.initialized = 0U;
    dev->i2c_dev.i2c_handle = NULL;
    return XY_DEVICE_OK;
}

xy_error_t xy_cmm905_read(xy_cmm905_t *dev, xy_cmm905_sample_t *sample)
{
    uint8_t bytes[6];
    xy_cmm905_sample_t next;
    xy_error_t ret;
    size_t i;

    if (!cmm905_ready(dev) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }

    /* The MM905 exposes X/Y/Z as six single-byte registers from 0x04; the legacy owner read them one
     * register at a time (no auto-increment), so this owner preserves that access pattern verbatim. */
    for (i = 0; i < 6U; i++) {
        ret = cmm905_read_reg(dev, (uint8_t)(XY_CMM905_REG_DATA + i), &bytes[i], 1U);
        if (ret != XY_DEVICE_OK) {
            return ret;
        }
    }
    next.raw_x = (int16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
    next.raw_y = (int16_t)((uint16_t)bytes[2] | ((uint16_t)bytes[3] << 8));
    next.raw_z = (int16_t)((uint16_t)bytes[4] | ((uint16_t)bytes[5] << 8));
    next.timestamp = xy_hal_sys_get_tick_count();
    *sample = next;
    dev->sample = next;
    return XY_DEVICE_OK;
}
