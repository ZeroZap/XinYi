/**
 * @file pandora_icm20608_owner.c
 * @brief Board-owned canonical ICM20608 owner for Pandora.
 */

#include "pandora_icm20608_owner.h"

#include <string.h>

static int icm20608_owner_readable(const pandora_icm20608_owner_t *owner)
{
    return owner != NULL && owner->device.initialized != 0U &&
           owner->device.configuration_synchronized != 0U &&
           owner->device.i2c_dev.base.initialized != 0U &&
           owner->device.i2c_dev.i2c_handle != NULL;
}

xy_error_t pandora_icm20608_owner_init(pandora_icm20608_owner_t *owner, void *i2c_handle)
{
    pandora_icm20608_owner_t candidate;
    xy_error_t result;

    if (owner == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(&candidate, 0, sizeof(candidate));
    result = xy_icm20608_init_i2c(&candidate.device, i2c_handle, XY_ICM20608_ADDR_DEFAULT);
    if (result != XY_DEVICE_OK) {
        /* Fail closed: the caller keeps whatever owner state it already had. */
        return result;
    }
    *owner = candidate;
    return XY_DEVICE_OK;
}

xy_error_t pandora_icm20608_owner_read_reg(pandora_icm20608_owner_t *owner, uint8_t reg,
                                           uint8_t *data, uint16_t len)
{
    if (!icm20608_owner_readable(owner) || data == NULL || len == 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&owner->device.i2c_dev, reg, data, len);
}

xy_error_t pandora_icm20608_owner_read_raw(pandora_icm20608_owner_t *owner,
                                           uint8_t raw[PANDORA_ICM20608_RAW_LEN])
{
    if (!icm20608_owner_readable(owner) || raw == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&owner->device.i2c_dev, XY_ICM20608_REG_ACCEL_XOUT_H, raw,
                                  PANDORA_ICM20608_RAW_LEN);
}

xy_error_t pandora_icm20608_owner_sample(pandora_icm20608_owner_t *owner,
                                         xy_icm20608_sample_t *sample)
{
    xy_icm20608_sample_t next;
    xy_error_t result;

    if (!icm20608_owner_readable(owner) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(&next, 0, sizeof(next));
    result = xy_icm20608_read_sample(&owner->device, &next);
    if (result != XY_DEVICE_OK) {
        /* Preserve the caller's previous sample on a partial/failed read. */
        return result;
    }
    *sample = next;
    return XY_DEVICE_OK;
}
