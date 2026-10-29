#include "xy_sys.h"

#include "pandora_sys.h"
#include "xy_error.h"
#include "xy_hal_rtc.h"
#include "xy_hal_sys.h"

/* Retained reset-request record. The RTC backup domain survives a system
 * reset (VDD stays powered), so the reason handed to xy_sys_reset() can be
 * reported by the next boot. Slots 0..2 are owned by the W25Q128/FOTA flows. */
#define PANDORA_RESET_REQUEST_INDEX 3U
#define PANDORA_RESET_REQUEST_MAGIC 0x58595200UL
#define PANDORA_RESET_REQUEST_REASON_MASK 0xFFUL

/* Every bit xy_hal_sys_get_reset_reason() can report. Anything outside it means
 * the HAL grew a cause this board has not been taught to attribute. */
#define PANDORA_RESET_REASON_KNOWN                                                        \
    (XY_HAL_SYS_RESET_REASON_EXTERNAL_PIN | XY_HAL_SYS_RESET_REASON_BROWNOUT |            \
     XY_HAL_SYS_RESET_REASON_SOFTWARE | XY_HAL_SYS_RESET_REASON_WATCHDOG |                \
     XY_HAL_SYS_RESET_REASON_LOW_POWER)

static const char *const pandora_reset_kind_names[] = {
    [PANDORA_RESET_KIND_UNKNOWN] = "UNKNOWN",
    [PANDORA_RESET_KIND_POWER_ON] = "POWER_ON",
    [PANDORA_RESET_KIND_EXTERNAL_PIN] = "EXTERNAL_PIN",
    [PANDORA_RESET_KIND_BROWNOUT] = "BROWNOUT",
    [PANDORA_RESET_KIND_SOFTWARE] = "SOFTWARE",
    [PANDORA_RESET_KIND_WATCHDOG] = "WATCHDOG",
    [PANDORA_RESET_KIND_LOW_POWER] = "LOW_POWER",
};

static uint32_t pandora_reset_reason;
static uint32_t pandora_reset_request;
static pandora_reset_kind_t pandora_reset_kind = PANDORA_RESET_KIND_UNKNOWN;

pandora_reset_kind_t pandora_reset_kind_from_reason(uint32_t reason)
{
    uint32_t known = reason & PANDORA_RESET_REASON_KNOWN;

    if ((reason & ~PANDORA_RESET_REASON_KNOWN) != 0U) {
        return PANDORA_RESET_KIND_UNKNOWN;
    }
    /* An internal reset pulls NRST low, so it also reports the pin flag: the
     * named cause has to outrank the pin bit it asserts. */
    if ((known & XY_HAL_SYS_RESET_REASON_WATCHDOG) != 0U) {
        return PANDORA_RESET_KIND_WATCHDOG;
    }
    if ((known & XY_HAL_SYS_RESET_REASON_SOFTWARE) != 0U) {
        return PANDORA_RESET_KIND_SOFTWARE;
    }
    if ((known & XY_HAL_SYS_RESET_REASON_BROWNOUT) != 0U) {
        return PANDORA_RESET_KIND_BROWNOUT;
    }
    if ((known & XY_HAL_SYS_RESET_REASON_LOW_POWER) != 0U) {
        return PANDORA_RESET_KIND_LOW_POWER;
    }
    if ((known & XY_HAL_SYS_RESET_REASON_EXTERNAL_PIN) != 0U) {
        return PANDORA_RESET_KIND_EXTERNAL_PIN;
    }
    return PANDORA_RESET_KIND_POWER_ON;
}

const char *pandora_reset_kind_name(pandora_reset_kind_t kind)
{
    if ((unsigned int)kind >=
        sizeof(pandora_reset_kind_names) / sizeof(pandora_reset_kind_names[0])) {
        return pandora_reset_kind_names[PANDORA_RESET_KIND_UNKNOWN];
    }
    return pandora_reset_kind_names[kind];
}

pandora_reset_kind_t pandora_sys_reset_kind(void)
{
    return pandora_reset_kind;
}

void xy_sys_init(void)
{
    uint32_t retained = 0U;

    pandora_reset_reason = xy_hal_sys_get_reset_reason();
    pandora_reset_kind = pandora_reset_kind_from_reason(pandora_reset_reason);
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
