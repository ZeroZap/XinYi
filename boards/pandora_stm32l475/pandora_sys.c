#include "xy_sys.h"

#include "xy_error.h"
#include "xy_hal_rtc.h"
#include "xy_hal_sys.h"

/* Retained reset-request record. The RTC backup domain survives a system
 * reset (VDD stays powered), so the reason handed to xy_sys_reset() can be
 * reported by the next boot. Slots 0..2 are owned by the W25Q128/FOTA flows. */
#define PANDORA_RESET_REQUEST_INDEX 3U
#define PANDORA_RESET_REQUEST_MAGIC 0x58595200UL
#define PANDORA_RESET_REQUEST_REASON_MASK 0xFFUL

static uint32_t pandora_reset_reason;
static uint32_t pandora_reset_request;

void xy_sys_init(void)
{
    uint32_t retained = 0U;

    pandora_reset_reason = xy_hal_sys_get_reset_reason();
    (void)xy_hal_sys_clear_reset_reason();

    pandora_reset_request = XY_SYS_RESET_REQUEST_UNSPECIFIED;
    if (xy_hal_rtc_backup_read(PANDORA_RESET_REQUEST_INDEX, &retained) == XY_HAL_OK &&
        (retained & ~PANDORA_RESET_REQUEST_REASON_MASK) == PANDORA_RESET_REQUEST_MAGIC) {
        pandora_reset_request = retained & PANDORA_RESET_REQUEST_REASON_MASK;
    }

    /* One-shot: the record describes exactly the reset that just happened, so a
     * later watchdog or pin reset must not re-report a stale request. Clearing
     * unconditionally also drops foreign words from other backup-register users. */
    (void)xy_hal_rtc_backup_write(PANDORA_RESET_REQUEST_INDEX, 0U);
}

int xy_sys_reset(int reset_by)
{
    xy_hal_error_t record_result;
    xy_hal_error_t reset_result;

    if (reset_by < 0 || (uint32_t)reset_by > PANDORA_RESET_REQUEST_REASON_MASK) {
        return XY_ERROR_INVALID_PARAM;
    }

    record_result =
        xy_hal_rtc_backup_write(PANDORA_RESET_REQUEST_INDEX,
                                PANDORA_RESET_REQUEST_MAGIC | (uint32_t)reset_by);
    /* Recorded before the reset so the reason is durable when the HAL software
     * reset diverges. A failed record is reported, but the reset is still
     * issued: stranding the board in a half-reset state is the worse failure. */
    reset_result = xy_hal_sys_software_reset();

    if (record_result != XY_HAL_OK) {
        return XY_ERROR;
    }

    return reset_result == XY_HAL_OK ? XY_OK : XY_ERROR;
}

int xy_sys_reboot_reason(void *data)
{
    if (data == NULL) {
        return XY_ERROR_INVALID_PARAM;
    }

    *(uint32_t *)data = pandora_reset_reason;
    return XY_OK;
}

int xy_sys_reset_request(void *data)
{
    if (data == NULL) {
        return XY_ERROR_INVALID_PARAM;
    }

    *(uint32_t *)data = pandora_reset_request;
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
