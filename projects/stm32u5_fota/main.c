/**
 * @file main.c
 * @brief STM32U5 FOTA integration skeleton with fail-closed board hooks
 *
 * This project intentionally does not provide production flash or bootloader
 * implementations. Board owners must bind the metadata backend below to a
 * reserved internal-Flash region before an update can be handed off or confirmed.
 */

#include "xy_fota.h"
#include "xy_fota_metadata.h"

#include <stdint.h>
#include <stdio.h>

static xy_fota_config_t fota_config = {
    .mode = XY_FOTA_MODE_DUAL_BANK,
    .flash_base_addr = 0x08020000U,
    .slot_size = 256U * 1024U,
    .slot_count = 2U,
    .backup_addr = 0x08060000U,
    .backup_size = 256U * 1024U,
    .enable_secure_boot = false,
    .enable_rollback = true,
    .min_version = 1U,
};

static xy_fota_metadata_flash_t metadata_backend = {
    .ops = NULL,
    .base_addr = 0U,
    .erase_size = 0U,
};

static int board_boot_handoff(uint8_t slot, uint32_t version, void *user_data)
{
    return xy_fota_metadata_boot_handoff(slot, version, user_data);
}

static int board_boot_confirm(uint8_t slot, uint32_t version, void *user_data)
{
    return xy_fota_metadata_boot_confirm(slot, version, user_data);
}

static int board_record_boot_attempt(uint8_t max_attempts, bool *rollback_required)
{
    return xy_fota_metadata_boot_attempt(max_attempts, rollback_required, &metadata_backend);
}

int main(void)
{
    xy_fota_t fota;
    int ret;

    printf("XinYi STM32U5 FOTA integration skeleton\n");

    ret = xy_fota_init(&fota, &fota_config);
    if (ret != XY_FOTA_OK) {
        printf("FOTA init failed: %d\n", ret);
        return ret;
    }

    ret = xy_fota_metadata_flash_validate(&metadata_backend);
    if (ret != XY_FOTA_OK) {
        printf("FOTA metadata backend unavailable: %d\n", ret);
        return ret;
    }

    ret = xy_fota_set_boot_handoff(&fota, board_boot_handoff, &metadata_backend);
    if (ret != XY_FOTA_OK) {
        printf("FOTA handoff registration failed: %d\n", ret);
        return ret;
    }

    ret = xy_fota_set_boot_confirm(&fota, board_boot_confirm, &metadata_backend);
    if (ret != XY_FOTA_OK) {
        printf("FOTA confirmation registration failed: %d\n", ret);
        return ret;
    }

    printf("FOTA core ready; bootloader must call board_record_boot_attempt before candidate boot\n");

    (void)board_record_boot_attempt;

    for (;;) {
        /* Product integration supplies transport and board event processing. */
    }
}
