/**
 * @file main.c
 * @brief STM32U5 FOTA integration skeleton with fail-closed board hooks
 *
 * This project intentionally does not provide a production flash, bootloader,
 * metadata, or reset backend. Board owners must replace the callbacks below
 * before an update can be handed off or confirmed.
 */

#include "xy_fota.h"

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

static int board_boot_handoff(uint8_t slot, uint32_t version, void *user_data)
{
    (void)slot;
    (void)version;
    (void)user_data;
    return XY_FOTA_NOT_SUPPORTED;
}

static int board_boot_confirm(uint8_t slot, uint32_t version, void *user_data)
{
    (void)slot;
    (void)version;
    (void)user_data;
    return XY_FOTA_NOT_SUPPORTED;
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

    ret = xy_fota_set_boot_handoff(&fota, board_boot_handoff, NULL);
    if (ret != XY_FOTA_OK) {
        printf("FOTA handoff registration failed: %d\n", ret);
        return ret;
    }

    ret = xy_fota_set_boot_confirm(&fota, board_boot_confirm, NULL);
    if (ret != XY_FOTA_OK) {
        printf("FOTA confirmation registration failed: %d\n", ret);
        return ret;
    }

    printf("FOTA core ready; board flash/metadata/bootloader backends remain unsupported\n");

    for (;;) {
        /* Product integration supplies transport and board event processing. */
    }
}
