/**
 * @file pandora_aht10_owner.c
 * @brief Board-owned canonical AHT10 owner for Pandora.
 */

#include "pandora_aht10_owner.h"

#include <string.h>

static int aht10_owner_readable(const pandora_aht10_owner_t* owner) {
    return owner != NULL && owner->device.initialized != 0U &&
           owner->device.i2c_dev.base.initialized != 0U && owner->device.i2c_dev.i2c_handle != NULL;
}

xy_error_t pandora_aht10_owner_init(pandora_aht10_owner_t* owner, void* i2c_handle) {
    pandora_aht10_owner_t candidate;
    xy_error_t result;

    if (owner == NULL || i2c_handle == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(&candidate, 0, sizeof(candidate));
    result = xy_aht10_init(&candidate.device, i2c_handle, XY_AHT10_DEFAULT_ADDRESS);
    if (result != XY_DEVICE_OK) {
        /* Fail closed: the caller keeps whatever owner state it already had. */
        return result;
    }
    *owner = candidate;
    return XY_DEVICE_OK;
}

xy_error_t pandora_aht10_owner_deinit(pandora_aht10_owner_t* owner) {
    if (owner == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    return xy_aht10_deinit(&owner->device);
}

xy_error_t pandora_aht10_owner_sample(pandora_aht10_owner_t* owner, xy_aht10_data_t* sample) {
    xy_aht10_data_t next;
    xy_error_t result;

    if (!aht10_owner_readable(owner) || sample == NULL) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(&next, 0, sizeof(next));
    result = xy_aht10_read(&owner->device, &next);
    if (result != XY_DEVICE_OK) {
        return result;
    }
    *sample = next;
    return XY_DEVICE_OK;
}
