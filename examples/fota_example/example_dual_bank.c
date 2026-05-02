/**
 * @file example_dual_bank.c
 * @brief Dual Bank Swap Mechanism Example
 *
 * This example demonstrates dual-bank FOTA:
 * - Dual bank flash layout
 * - Bank swapping mechanism
 * - Active/alternate slot management
 * - Safe update with rollback support
 *
 * Reference: SECURE_FOTA.md - Flash Layout section
 */

#include "xy_fota.h"
#include <stdio.h>
#include <string.h>

/* ==================== Flash Layout (512KB total) ==================== */
/*
 * +------------------+ 0x00000 - Bootloader (32KB)
 * |    Bootloader    |
 * +------------------+ 0x08000 - Config (32KB)
 * |     Config       |
 * +------------------+ 0x10000 - Slot 0 (192KB) - Active firmware
 * |     Slot 0       |
 * +------------------+ 0x40000 - Slot 1 (192KB) - Update firmware
 * |     Slot 1       |
 * +------------------+ 0x70000 - Parameters (64KB)
 * |    Parameters    |
 * +------------------+ 0x80000 - End
 */

/* Flash layout constants */
#define FLASH_BASE_ADDR     0x08000000
#define BOOTLOADER_SIZE     (32 * 1024)   /* 32KB */
#define CONFIG_SIZE         (32 * 1024)    /* 32KB */
#define SLOT_SIZE           (192 * 1024)   /* 192KB */
#define PARAM_SIZE          (64 * 1024)    /* 64KB */
#define SLOT0_ADDR          (FLASH_BASE_ADDR + BOOTLOADER_SIZE + CONFIG_SIZE)
#define SLOT1_ADDR          (SLOT0_ADDR + SLOT_SIZE)
#define PARAM_ADDR          (FLASH_BASE_ADDR + 0x70000)

/* ==================== Bank Status ==================== */

typedef enum {
    BANK_INVALID = 0,
    BANK_VALID,
    BANK_UPDATE_AVAILABLE,
    BANK_corrupted,
} bank_status_t;

/* Bank information structure */
typedef struct {
    uint32_t address;
    uint32_t size;
    bank_status_t status;
    uint32_t version;
    uint32_t crc32;
} bank_info_t;

/* ==================== Mock Flash Operations ==================== */

static uint8_t mock_flash[512 * 1024] = {0};

static int mock_flash_init(void)
{
    printf("[Flash] Dual-bank flash initialized\r\n");
    return XY_FOTA_OK;
}

static int mock_flash_write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    if (addr + size > sizeof(mock_flash)) {
        return XY_FOTA_ERROR;
    }
    memcpy(&mock_flash[addr], data, size);
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
    printf("[Flash] Dual-bank flash deinitialized\r\n");
    return XY_FOTA_OK;
}

static const xy_fota_flash_ops_t g_dual_bank_flash_ops = {
    .init   = mock_flash_init,
    .write  = mock_flash_write,
    .read   = mock_flash_read,
    .erase  = mock_flash_erase,
    .deinit = mock_flash_deinit,
};

/* ==================== Dual Bank Functions ==================== */

/**
 * @brief Get bank information
 */
static void dual_bank_get_info(bank_info_t *slot0, bank_info_t *slot1)
{
    /* Slot 0 - Active bank */
    slot0->address = SLOT0_ADDR;
    slot0->size = SLOT_SIZE;
    slot0->status = BANK_VALID;
    slot0->version = 1;  /* Current running version */
    slot0->crc32 = 0x12345678;

    /* Slot 1 - Update bank */
    slot1->address = SLOT1_ADDR;
    slot1->size = SLOT_SIZE;
    slot1->status = BANK_INVALID;
    slot1->version = 0;
    slot1->crc32 = 0;
}

/**
 * @brief Print flash layout
 */
static void print_flash_layout(void)
{
    printf("\r\n");
    printf("    +---------------------+ 0x00000\r\n");
    printf("    |     Bootloader     | 32KB\r\n");
    printf("    +---------------------+ 0x08000\r\n");
    printf("    |       Config       | 32KB\r\n");
    printf("    +---------------------+ 0x10000\r\n");
    printf("    |       Slot 0       | 192KB (Active Firmware v1)\r\n");
    printf("    +---------------------+ 0x40000\r\n");
    printf("    |       Slot 1       | 192KB (Update Region)\r\n");
    printf("    +---------------------+ 0x70000\r\n");
    printf("    |     Parameters     | 64KB\r\n");
    printf("    +---------------------+ 0x80000\r\n");
    printf("\r\n");
}

/**
 * @brief Print bank status
 */
static void print_bank_status(const bank_info_t *slot0, const bank_info_t *slot1, uint32_t active_slot)
{
    printf("\r\n");
    printf("    Bank Status:\r\n");
    printf("    +----------+---------+--------+---------+----------+\r\n");
    printf("    |   Slot   | Address | Status | Version |   CRC    |\r\n");
    printf("    +----------+---------+--------+---------+----------+\r\n");

    printf("    | Slot %u   | 0x%05X | %-6s | v%u      | 0x%08X |\r\n",
           0, (unsigned int)slot0->address,
           slot0->status == BANK_VALID ? "VALID" : "INVALID",
           (unsigned int)slot0->version,
           (unsigned int)slot0->crc32);

    printf("    | Slot %u   | 0x%05X | %-6s | v%u      | 0x%08X |\r\n",
           1, (unsigned int)slot1->address,
           slot1->status == BANK_VALID ? "VALID" :
           slot1->status == BANK_UPDATE_AVAILABLE ? "UPDATE" : "INVALID",
           (unsigned int)slot1->version,
           (unsigned int)slot1->crc32);

    printf("    +----------+---------+--------+---------+----------+\r\n");
    printf("\r\n");
    printf("    Active Slot: %u\r\n", active_slot);
    printf("\r\n");
}

/**
 * @brief Swap active slot (after successful update)
 */
static uint32_t dual_bank_swap(uint32_t current_slot)
{
    uint32_t new_slot = 1 - current_slot;
    printf("[DualBank] Swapping from Slot %u to Slot %u\r\n", current_slot, new_slot);
    printf("[DualBank] Update complete - next boot will run new firmware\r\n");
    return new_slot;
}

/**
 * @brief Validate firmware in bank
 */
static int dual_bank_validate(uint32_t slot_addr, uint32_t size)
{
    printf("[DualBank] Validating firmware at 0x%08X (size: %u bytes)\r\n",
           slot_addr, size);

    /* Simulate validation */
    /* In production: verify CRC, signature, magic number, etc. */

    return XY_FOTA_OK;
}

/* ==================== Progress Callback ==================== */

static void dual_bank_progress_callback(uint32_t current, uint32_t total, void *user_data)
{
    uint8_t progress = (uint8_t)((current * 100) / total);
    printf("[DualBank FOTA] Progress: %u%% (%u/%u bytes)\r\n", progress, current, total);
    (void)user_data;
}

/* ==================== Main Example ==================== */

int main(void)
{
    int ret;
    xy_fota_t fota;
    xy_fota_config_t config;
    bank_info_t slot0, slot1;
    uint32_t active_slot = 0;  /* Currently running from slot 0 */
    uint8_t dummy_fw[1024];

    printf("\r\n");
    printf("========================================\r\n");
    printf("   Dual Bank FOTA Example\r\n");
    printf("========================================\r\n");
    printf("\r\n");

    /* 1. Show flash layout */
    printf("[Step 1] Flash Layout Overview\r\n");
    print_flash_layout();

    /* 2. Get bank information */
    printf("[Step 2] Getting bank information...\r\n");
    dual_bank_get_info(&slot0, &slot1);
    print_bank_status(&slot0, &slot1, active_slot);

    /* 3. Initialize FOTA in dual-bank mode */
    printf("[Step 3] Initializing Dual Bank FOTA...\r\n");
    memset(&config, 0, sizeof(config));
    config.mode = XY_FOTA_MODE_DUAL_BANK;
    config.flash_base_addr = SLOT0_ADDR;
    config.slot_size = SLOT_SIZE;
    config.slot_count = 2;
    config.enable_rollback = true;
    config.min_version = 1;

    ret = xy_fota_init(&fota, &config);
    if (ret != XY_FOTA_OK) {
        printf("[Error] FOTA init failed: %d\r\n", ret);
        return -1;
    }

    ret = xy_fota_set_flash_ops(&fota, &g_dual_bank_flash_ops);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Set flash ops failed: %d\r\n", ret);
        return -1;
    }

    ret = xy_fota_set_progress_callback(&fota, dual_bank_progress_callback, NULL);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Set progress callback failed: %d\r\n", ret);
        return -1;
    }

    printf("[OK] Dual Bank FOTA initialized\r\n");
    printf("     Active slot: %u\r\n", active_slot);
    printf("\r\n");

    /* 4. Simulate downloading new firmware to inactive slot */
    printf("[Step 4] Downloading firmware to inactive slot (Slot 1)...\r\n");

    /* Prepare dummy firmware */
    for (int i = 0; i < (int)sizeof(dummy_fw); i++) {
        dummy_fw[i] = (uint8_t)(i & 0xFF);
    }

    /* Start download to slot 1 (inactive bank) */
    printf("     Target: Slot 1 (0x%08X)\r\n", SLOT1_ADDR);

    ret = xy_fota_start_download(&fota, 2, sizeof(dummy_fw), false);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Start download failed: %d\r\n", ret);
        return -1;
    }
    printf("[OK] Download started: version 2 -> Slot 1\r\n");

    /* Download in chunks */
    uint32_t chunk_size = 256;
    uint32_t offset = 0;
    while (offset < sizeof(dummy_fw)) {
        uint32_t remaining = sizeof(dummy_fw) - offset;
        uint32_t to_send = (remaining < chunk_size) ? remaining : chunk_size;

        ret = xy_fota_download_chunk(&fota, &dummy_fw[offset], to_send);
        if (ret != XY_FOTA_OK) {
            printf("[Error] Download chunk failed at offset %u: %d\r\n", offset, ret);
            return -1;
        }
        offset += to_send;
    }

    ret = xy_fota_finish_download(&fota);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Finish download failed: %d\r\n", ret);
        return -1;
    }
    printf("[OK] Firmware downloaded to Slot 1\r\n");
    printf("\r\n");

    /* 5. Update bank status to show update available */
    printf("[Step 5] Updating bank status...\r\n");
    slot1.status = BANK_UPDATE_AVAILABLE;
    slot1.version = 2;
    slot1.crc32 = xy_fota_calc_crc32(dummy_fw, sizeof(dummy_fw));
    print_bank_status(&slot0, &slot1, active_slot);

    /* 6. Validate new firmware before swapping */
    printf("[Step 6] Validating new firmware in Slot 1...\r\n");
    ret = dual_bank_validate(SLOT1_ADDR, sizeof(dummy_fw));
    if (ret != XY_FOTA_OK) {
        printf("[Error] Validation failed: %d\r\n", ret);
        return -1;
    }
    slot1.status = BANK_VALID;
    printf("[OK] New firmware validated\r\n");
    printf("\r\n");

    /* 7. Apply update (swap banks) */
    printf("[Step 7] Applying update - swapping banks...\r\n");
    ret = xy_fota_start_update(&fota);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Apply update failed: %d\r\n", ret);
        return -1;
    }

    /* Perform the actual swap */
    active_slot = dual_bank_swap(active_slot);

    /* Update bank info after swap */
    slot0.version = 2;
    slot0.crc32 = slot1.crc32;
    slot1.version = 1;  /* Old firmware remains as backup */
    slot1.status = BANK_VALID;

    printf("[OK] Banks swapped successfully\r\n");
    print_bank_status(&slot0, &slot1, active_slot);
    printf("\r\n");

    /* 8. Rollback test */
    printf("[Step 8] Testing rollback capability...\r\n");
    printf("     Current: Slot %u (v%u)\r\n", active_slot, slot0.version);
    printf("     Backup:   Slot %u (v%u)\r\n", 1 - active_slot, slot1.version);

    if (xy_fota_needs_rollback(&fota)) {
        printf("[Info] Rollback needed, swapping back...\r\n");
        active_slot = dual_bank_swap(active_slot);
        printf("[OK] Rollback completed - now running Slot %u\r\n", active_slot);
    } else {
        printf("[Info] No rollback needed - firmware is healthy\r\n");
    }
    printf("\r\n");

    /* 9. Cleanup */
    printf("[Step 9] Cleanup...\r\n");
    xy_fota_deinit(&fota);
    printf("[OK] FOTA deinitialized\r\n");
    printf("\r\n");

    printf("========================================\r\n");
    printf("   Dual Bank FOTA Example Completed\r\n");
    printf("========================================\r\n");

    return 0;
}
