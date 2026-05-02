/**
 * @file example_basic.c
 * @brief Basic FOTA Usage Example
 *
 * This example demonstrates basic FOTA functionality:
 * - FOTA initialization
 * - Check for updates
 * - Download image
 * - Verify signature (basic CRC)
 * - Apply update
 * - Rollback handling
 */

#include "xy_fota.h"
#include <stdio.h>
#include <string.h>

/* ==================== Mock Flash Operations ==================== */

static uint8_t mock_flash[512 * 1024] = {0};  /* 512KB mock flash */

static int mock_flash_init(void)
{
    printf("[Flash] Mock flash initialized\r\n");
    return XY_FOTA_OK;
}

static int mock_flash_write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    if (addr + size > sizeof(mock_flash)) {
        return XY_FOTA_ERROR;
    }
    memcpy(&mock_flash[addr], data, size);
    printf("[Flash] Wrote %u bytes at 0x%08X\r\n", size, addr);
    return XY_FOTA_OK;
}

static int mock_flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (addr + size > sizeof(mock_flash)) {
        return XY_FOTA_ERROR;
    }
    memcpy(data, &mock_flash[addr], size);
    return XY_FOTA_OK;
}

static int mock_flash_erase(uint32_t addr, uint32_t size)
{
    if (addr + size > sizeof(mock_flash)) {
        return XY_FOTA_ERROR;
    }
    memset(&mock_flash[addr], 0xFF, size);
    printf("[Flash] Erased %u bytes at 0x%08X\r\n", size, addr);
    return XY_FOTA_OK;
}

static int mock_flash_deinit(void)
{
    printf("[Flash] Mock flash deinitialized\r\n");
    return XY_FOTA_OK;
}

/* Flash operations structure */
static const xy_fota_flash_ops_t g_flash_ops = {
    .init   = mock_flash_init,
    .write  = mock_flash_write,
    .read   = mock_flash_read,
    .erase  = mock_flash_erase,
    .deinit = mock_flash_deinit,
};

/* ==================== Progress Callback ==================== */

static void progress_callback(uint32_t current, uint32_t total, void *user_data)
{
    uint8_t progress = (uint8_t)((current * 100) / total);
    printf("[FOTA] Progress: %u%% (%u/%u bytes)\r\n", progress, current, total);
    (void)user_data;
}

/* ==================== Main Example ==================== */

int main(void)
{
    int ret;
    xy_fota_t fota;
    xy_fota_config_t config;

    printf("\r\n");
    printf("========================================\r\n");
    printf("   Basic FOTA Example\r\n");
    printf("========================================\r\n");
    printf("\r\n");

    /* 1. FOTA initialization */
    printf("[Step 1] Initializing FOTA...\r\n");
    memset(&config, 0, sizeof(config));
    config.mode = XY_FOTA_MODE_DUAL_BANK;
    config.flash_base_addr = 0x08000000;
    config.slot_size = 192 * 1024;  /* 192KB per slot */
    config.slot_count = 2;
    config.enable_rollback = true;
    config.min_version = 1;  /* Prevent downgrade below v1 */

    ret = xy_fota_init(&fota, &config);
    if (ret != XY_FOTA_OK) {
        printf("[Error] FOTA init failed: %d\r\n", ret);
        return -1;
    }

    /* Set flash operations */
    ret = xy_fota_set_flash_ops(&fota, &g_flash_ops);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Set flash ops failed: %d\r\n", ret);
        return -1;
    }

    /* Set progress callback */
    ret = xy_fota_set_progress_callback(&fota, progress_callback, NULL);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Set progress callback failed: %d\r\n", ret);
        return -1;
    }

    printf("[OK] FOTA initialized in dual-bank mode\r\n");
    printf("     Flash base: 0x%08X\r\n", config.flash_base_addr);
    printf("     Slot size: %u bytes\r\n", config.slot_size);
    printf("\r\n");

    /* 2. Check for updates */
    printf("[Step 2] Checking for updates...\r\n");
    uint32_t current_version = xy_fota_get_current_version(&fota);
    printf("     Current version: %u\r\n", current_version);

    /* Simulate: server has version 2 available */
    uint32_t available_version = 2;
    printf("     Available version: %u\r\n", available_version);

    if (available_version > current_version) {
        printf("     [Update available]\r\n");
    } else {
        printf("     [No update needed]\r\n");
    }
    printf("\r\n");

    /* 3. Download image */
    printf("[Step 3] Downloading firmware image...\r\n");

    /* Simulated firmware data */
    uint8_t dummy_fw[1024];
    for (int i = 0; i < (int)sizeof(dummy_fw); i++) {
        dummy_fw[i] = (uint8_t)(i & 0xFF);
    }

    /* Start download */
    uint32_t fw_size = sizeof(dummy_fw);
    ret = xy_fota_start_download(&fota, available_version, fw_size, false);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Start download failed: %d\r\n", ret);
        return -1;
    }
    printf("[OK] Download started: version %u, size %u bytes\r\n", available_version, fw_size);

    /* Download in chunks */
    uint32_t chunk_size = 256;
    uint32_t offset = 0;
    while (offset < fw_size) {
        uint32_t remaining = fw_size - offset;
        uint32_t to_send = (remaining < chunk_size) ? remaining : chunk_size;

        ret = xy_fota_download_chunk(&fota, &dummy_fw[offset], to_send);
        if (ret != XY_FOTA_OK) {
            printf("[Error] Download chunk failed at offset %u: %d\r\n", offset, ret);
            return -1;
        }
        offset += to_send;
    }

    /* Finish download */
    ret = xy_fota_finish_download(&fota);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Finish download failed: %d\r\n", ret);
        return -1;
    }
    printf("[OK] Download completed\r\n");
    printf("\r\n");

    /* 4. Verify (basic CRC check) */
    printf("[Step 4] Verifying firmware...\r\n");
    xy_fota_state_t state = xy_fota_get_state(&fota);
    printf("     State: %d (expected COMPLETE=9)\r\n", state);

    if (state == XY_FOTA_STATE_COMPLETE) {
        printf("[OK] Firmware verification passed\r\n");
    } else {
        printf("[Error] Firmware verification failed, state=%d\r\n", state);
        return -1;
    }
    printf("\r\n");

    /* 5. Apply update */
    printf("[Step 5] Applying update...\r\n");
    ret = xy_fota_start_update(&fota);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Apply update failed: %d\r\n", ret);
        return -1;
    }
    printf("[OK] Update applied, system will reboot to new firmware\r\n");
    printf("\r\n");

    /* 6. Rollback handling */
    printf("[Step 6] Testing rollback...\r\n");

    /* Check if rollback is needed */
    if (xy_fota_needs_rollback(&fota)) {
        printf("[Info] Rollback needed, restoring previous version...\r\n");
        ret = xy_fota_rollback(&fota);
        if (ret != XY_FOTA_OK) {
            printf("[Error] Rollback failed: %d\r\n", ret);
            return -1;
        }
        printf("[OK] Rollback completed\r\n");
    } else {
        printf("[Info] No rollback needed, firmware is healthy\r\n");
    }
    printf("\r\n");

    /* Cleanup */
    printf("[Step 7] Cleanup...\r\n");
    xy_fota_deinit(&fota);
    printf("[OK] FOTA deinitialized\r\n");
    printf("\r\n");

    printf("========================================\r\n");
    printf("   Basic FOTA Example Completed\r\n");
    printf("========================================\r\n");

    return 0;
}
