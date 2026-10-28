/**
 * @file pandora_ap3216c_owner.c
 * @brief Board-owned canonical AP3216C owner for Pandora.
 */

#include "pandora_ap3216c_owner.h"

#include <string.h>

static int ap3216c_owner_readable(const pandora_ap3216c_owner_t *owner)
{
    return owner != NULL && owner->device.initialized != 0U &&
           owner->device.i2c_dev.base.initialized != 0U &&
           owner->device.i2c_dev.i2c_handle != NULL;
}

xy_error_t pandora_ap3216c_owner_init(pandora_ap3216c_owner_t *owner, void *i2c_handle)
{
    pandora_ap3216c_owner_t candidate;
    xy_error_t result;

    if (owner == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(&candidate, 0, sizeof(candidate));
    result = xy_ap3216c_init(&candidate.device, i2c_handle, XY_AP3216C_DEFAULT_ADDRESS,
                             XY_AP3216C_MODE_ALS_PS);
    if (result != XY_DEVICE_OK) {
        /* Fail closed: the caller keeps whatever owner state it already had. */
        return result;
    }
    *owner = candidate;
    return XY_DEVICE_OK;
}

xy_error_t pandora_ap3216c_owner_read_reg(pandora_ap3216c_owner_t *owner, uint8_t reg,
                                          uint8_t *data, uint16_t len)
{
    if (!ap3216c_owner_readable(owner) || data == NULL || len == 0U) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&owner->device.i2c_dev, reg, data, len);
}

xy_error_t pandora_ap3216c_owner_read_raw(pandora_ap3216c_owner_t *owner,
                                          uint8_t raw[PANDORA_AP3216C_RAW_LEN])
{
    if (!ap3216c_owner_readable(owner) || raw == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_i2c_device_read_reg(&owner->device.i2c_dev, XY_AP3216C_REG_IR_DATA_L, raw,
                                  PANDORA_AP3216C_RAW_LEN);
}

xy_error_t pandora_ap3216c_owner_sample(pandora_ap3216c_owner_t *owner,
                                        pandora_ap3216c_sample_t *sample)
{
    pandora_ap3216c_sample_t next;
    xy_error_t result;

    if (!ap3216c_owner_readable(owner) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(&next, 0, sizeof(next));
    result = xy_ap3216c_read_light(&owner->device, &next.illuminance_millilux);
    if (result == XY_DEVICE_OK) {
        result = xy_ap3216c_read_proximity(&owner->device, &next.proximity_raw);
    }
    if (result == XY_DEVICE_OK) {
        result = xy_ap3216c_read_ir(&owner->device, &next.infrared_raw);
    }
    if (result != XY_DEVICE_OK) {
        return result;
    }
    *sample = next;
    return XY_DEVICE_OK;
}
