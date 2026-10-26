#include "xy_sys.h"

#include "xy_error.h"
#include "xy_hal_sys.h"

static uint32_t pandora_reset_reason;

void xy_sys_init(void)
{
    pandora_reset_reason = xy_hal_sys_get_reset_reason();
    (void)xy_hal_sys_clear_reset_reason();
}

int xy_sys_reset(int reset_by)
{
    if (reset_by < 0) {
        return XY_ERROR_INVALID_PARAM;
    }

    return xy_hal_sys_software_reset() == XY_HAL_OK ? XY_OK : XY_ERROR;
}

int xy_sys_reboot_reason(void *data)
{
    if (data == NULL) {
        return XY_ERROR_INVALID_PARAM;
    }

    *(uint32_t *)data = pandora_reset_reason;
    return XY_OK;
}

int xy_sys_get_chip_id(void *data)
{
    uint32_t *chip_id = data;

    if (chip_id == NULL) {
        return XY_ERROR_INVALID_PARAM;
    }

    return xy_hal_sys_get_unique_id(chip_id) == XY_HAL_OK ? XY_OK : XY_ERROR;
}

int xy_sys_get_mac_addr(void *data)
{
    uint32_t uid[3];
    uint8_t *mac = data;

    if (mac == NULL) {
        return XY_ERROR_INVALID_PARAM;
    }

    if (xy_hal_sys_get_unique_id(uid) != XY_HAL_OK) {
        return XY_ERROR;
    }

    /* Derive a deterministic unicast, locally-administered MAC from the
     * 96-bit unique ID (little-endian low bytes of uid[0..1]). */
    mac[0] = (uint8_t)(uid[0] & 0xFFU);
    mac[1] = (uint8_t)((uid[0] >> 8U) & 0xFFU);
    mac[2] = (uint8_t)((uid[0] >> 16U) & 0xFFU);
    mac[3] = (uint8_t)((uid[0] >> 24U) & 0xFFU);
    mac[4] = (uint8_t)(uid[1] & 0xFFU);
    mac[5] = (uint8_t)((uid[1] >> 8U) & 0xFFU);

    /* unicast (bit 0 clear) + locally administered (bit 1 set) */
    mac[0] = (uint8_t)((mac[0] & 0xFCU) | 0x02U);

    return XY_OK;
}
