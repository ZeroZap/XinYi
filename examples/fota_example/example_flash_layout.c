/**
 * @file example_flash_layout.c
 * @brief Flash Layout Configuration Example
 *
 * This example demonstrates:
 * - Various flash layout configurations for different MCU platforms
 * - Single bank with backup layout
 * - Dual bank layout (primary + update)
 * - Combined internal + external flash layout
 * - Memory-mapped IOT flash layout
 *
 * Reference: SECURE_FOTA.md - Bootloader Memory Layout section
 */

#include "xy_fota.h"
#include <stdio.h>
#include <string.h>

/* ==================== Platform Definitions ==================== */

/* STM32F4 (512KB Flash, 128KB RAM) */
#define STM32F4_FLASH_BASE     0x08000000
#define STM32F4_FLASH_SIZE     (512 * 1024)
#define STM32F4_SRAM_BASE      0x20000000
#define STM32F4_SRAM_SIZE      (128 * 1024)

/* STM32U5 (2MB Flash, 256KB RAM) */
#define STM32U5_FLASH_BASE     0x08000000
#define STM32U5_FLASH_SIZE     (2 * 1024 * 1024)
#define STM32U5_SRAM_BASE      0x20000000
#define STM32U5_SRAM_SIZE      (256 * 1024)

/* WCH (WinChiphead RISC-V) */
#define WCH_FLASH_BASE         0x08000000
#define WCH_FLASH_SIZE         (256 * 1024)

/* ==================== Flash Layout Structures ==================== */

/**
 * @brief Flash region definition
 */
typedef struct {
    const char *name;
    uint32_t start;
    uint32_t size;
    const char *description;
} flash_region_t;

/**
 * @brief Complete flash layout for a platform
 */
typedef struct {
    const char *platform;
    uint32_t total_flash;
    flash_region_t regions[8];
    uint8_t region_count;
} flash_layout_t;

/* ==================== Mock Flash Operations ==================== */

static uint8_t mock_flash[2 * 1024 * 1024] = {0};  /* 2MB mock flash */

static int mock_flash_init(void)
{
    printf("[Flash] Flash initialized\r\n");
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
    return XY_FOTA_OK;
}

static int mock_flash_deinit(void)
{
    printf("[Flash] Flash deinitialized\r\n");
    return XY_FOTA_OK;
}

static const xy_fota_flash_ops_t g_flash_ops = {
    .init   = mock_flash_init,
    .write  = mock_flash_write,
    .read   = mock_flash_read,
    .erase  = mock_flash_erase,
    .deinit = mock_flash_deinit,
};

/* ==================== Layout Definitions ==================== */

/* STM32F4 Dual Bank Layout (512KB) */
static const flash_layout_t layout_stm32f4_dual_bank = {
    .platform = "STM32F4 (Dual Bank, 512KB)",
    .total_flash = 512 * 1024,
    .regions = {
        {"Bootloader",  0x08000000,  32 * 1024, "Bootloader + Secure Boot"},
        {"Config",     0x08008000,  32 * 1024, "NV config, keys, params"},
        {"Slot 0",     0x08010000, 192 * 1024, "Active firmware"},
        {"Slot 1",     0x08040000, 192 * 1024, "Update firmware"},
        {"Parameters", 0x08070000,  64 * 1024, "FOTA state, rollback info"},
    },
    .region_count = 5,
};

/* STM32U5 Large Flash Layout (2MB) */
static const flash_layout_t layout_stm32u5_large = {
    .platform = "STM32U5 (Large, 2MB)",
    .total_flash = 2 * 1024 * 1024,
    .regions = {
        {"Bootloader",  0x08000000,  64 * 1024, "Bootloader + Secure Boot"},
        {"Config",     0x08010000,  64 * 1024, "NV config, keys, params"},
        {"Slot 0",     0x08020000, 640 * 1024, "Active firmware"},
        {"Slot 1",     0x080C0000, 640 * 1024, "Update firmware"},
        {"Reserved",   0x08140000, 256 * 1024, "Reserved for future"},
        {"Parameters", 0x08180000, 384 * 1024, "FOTA state, rollback, logs"},
    },
    .region_count = 6,
};

/* Single Bank with Backup (for smaller MCUs) */
static const flash_layout_t layout_single_bank_backup = {
    .platform = "Single Bank with Backup (256KB)",
    .total_flash = 256 * 1024,
    .regions = {
        {"Bootloader",  0x08000000,  24 * 1024, "Bootloader"},
        {"Config",      0x08006000,  16 * 1024, "NV config"},
        {"App",         0x0800C000, 128 * 1024, "Application"},
        {"Backup",      0x0801C000,  64 * 1024, "Backup/Swap region"},
        {"Params",      0x0802C000,  24 * 1024, "FOTA parameters"},
    },
    .region_count = 5,
};

/* External SPI Flash Layout (combined) */
static const flash_layout_t layout_external_flash = {
    .platform = "Internal + External SPI Flash",
    .total_flash = (256 + 4 * 1024) * 1024,  /* 256KB internal + 4MB external */
    .regions = {
        /* Internal Flash */
        {"Bootloader",  0x08000000,  32 * 1024, "Bootloader (internal)"},
        {"Config",      0x08008000,  32 * 1024, "Config (internal)"},
        {"IntApp",      0x08010000, 192 * 1024, "Internal app region"},
        /* External SPI Flash */
        {"ExtSlot0",    0x90000000,   1 * 1024 * 1024, "External slot 0 (1MB)"},
        {"ExtSlot1",    0x90100000,   1 * 1024 * 1024, "External slot 1 (1MB)"},
        {"ExtBackup",   0x90200000,   1 * 1024 * 1024, "External backup (1MB)"},
        {"ExtData",     0x90300000,   1 * 1024 * 1024, "External data (1MB)"},
    },
    .region_count = 7,
};

/* ==================== Layout Display Functions ==================== */

/**
 * @brief Print flash layout visually
 */
static void print_layout(const flash_layout_t *layout)
{
    printf("\r\n");
    printf("    +========================================+\r\n");
    printf("    |   %-35s   |\r\n", layout->platform);
    printf("    +========================================+\r\n");

    for (int i = 0; i < (int)layout->region_count; i++) {
        const flash_region_t *region = &layout->regions[i];
        uint32_t size_kb = region->size / 1024;

        printf("    | 0x%08X  %-20s  %5u KB  |\r\n",
               (unsigned int)region->start,
               region->name,
               (unsigned int)size_kb);
        printf("    |              %-24s  |\r\n", region->description);

        if (i < layout->region_count - 1) {
            printf("    +----------------------------------------+\r\n");
        }
    }

    printf("    +----------------------------------------+\r\n");
    printf("    Total Flash: %u KB\r\n", (unsigned int)(layout->total_flash / 1024));
    printf("\r\n");
}

/**
 * @brief Configure FOTA based on flash layout
 */
static int configure_fota_for_layout(xy_fota_t *fota, const flash_layout_t *layout)
{
    xy_fota_config_t config;
    int ret;

    printf("[Config] Configuring FOTA for %s\r\n", layout->platform);

    /* Find App and Update regions */
    const flash_region_t *app_region = NULL;
    const flash_region_t *update_region = NULL;
    const flash_region_t *param_region = NULL;

    for (int i = 0; i < (int)layout->region_count; i++) {
        if (strcmp(layout->regions[i].name, "Slot 0") == 0 ||
            strcmp(layout->regions[i].name, "App") == 0) {
            app_region = &layout->regions[i];
        }
        if (strcmp(layout->regions[i].name, "Slot 1") == 0) {
            update_region = &layout->regions[i];
        }
        if (strcmp(layout->regions[i].name, "Parameters") == 0 ||
            strcmp(layout->regions[i].name, "Params") == 0) {
            param_region = &layout->regions[i];
        }
    }

    if (!app_region) {
        printf("[Error] Cannot find application region\r\n");
        return XY_FOTA_INVALID_PARAM;
    }

    /* Configure based on available regions */
    memset(&config, 0, sizeof(config));

    if (update_region) {
        /* Dual bank mode */
        config.mode = XY_FOTA_MODE_DUAL_BANK;
        config.flash_base_addr = app_region->start;
        config.slot_size = app_region->size;
        config.slot_count = 2;
        printf("[Config] Mode: Dual Bank\r\n");
        printf("[Config] Slot size: %u KB\r\n", (unsigned int)(config.slot_size / 1024));
    } else {
        /* Single bank with backup */
        config.mode = XY_FOTA_MODE_SINGLE_SLOT;
        config.flash_base_addr = app_region->start;
        config.slot_size = app_region->size;
        config.slot_count = 1;

        /* Find backup region */
        for (int i = 0; i < (int)layout->region_count; i++) {
            if (strcmp(layout->regions[i].name, "Backup") == 0) {
                config.backup_addr = layout->regions[i].start;
                config.backup_size = layout->regions[i].size;
                break;
            }
        }
        printf("[Config] Mode: Single Bank with Backup\r\n");
    }

    if (param_region) {
        printf("[Config] Parameters region: 0x%08X\r\n", (unsigned int)param_region->start);
    }

    config.enable_rollback = true;
    config.enable_secure_boot = true;
    config.min_version = 1;

    ret = xy_fota_init(fota, &config);
    if (ret != XY_FOTA_OK) {
        printf("[Error] FOTA init failed: %d\r\n", ret);
        return ret;
    }

    ret = xy_fota_set_flash_ops(fota, &g_flash_ops);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Set flash ops failed: %d\r\n", ret);
        return ret;
    }

    printf("[OK] FOTA configured successfully\r\n");

    return XY_FOTA_OK;
}

/**
 * @brief Calculate and display layout efficiency
 */
static void analyze_layout_efficiency(const flash_layout_t *layout)
{
    uint32_t app_size = 0;
    uint32_t total_overhead = 0;

    for (int i = 0; i < (int)layout->region_count; i++) {
        if (strcmp(layout->regions[i].name, "Slot 0") == 0 ||
            strcmp(layout->regions[i].name, "App") == 0) {
            app_size = layout->regions[i].size;
        } else if (strcmp(layout->regions[i].name, "Bootloader") != 0) {
            total_overhead += layout->regions[i].size;
        }
    }

    uint32_t usable_ratio = (app_size * 100) / layout->total_flash;

    printf("\r\n");
    printf("    Layout Efficiency Analysis:\r\n");
    printf("    +------------------------------+\r\n");
    printf("    | Total Flash:     %6u KB     |\r\n", (unsigned int)(layout->total_flash / 1024));
    printf("    | Application:     %6u KB     |\r\n", (unsigned int)(app_size / 1024));
    printf("    | Overhead:        %6u KB     |\r\n", (unsigned int)(total_overhead / 1024));
    printf("    | Usable Ratio:    %6u %%     |\r\n", usable_ratio);
    printf("    +------------------------------+\r\n");
    printf("\r\n");
}

/* ==================== Progress Callback ==================== */

static void layout_progress_callback(uint32_t current, uint32_t total, void *user_data)
{
    uint8_t progress = (uint8_t)((current * 100) / total);
    printf("[FlashLayout] Progress: %u%%\r\n", progress);
    (void)user_data;
}

/* ==================== Main Example ==================== */

int main(void)
{
    int ret;
    xy_fota_t fota;

    printf("\r\n");
    printf("========================================\r\n");
    printf("   Flash Layout Configuration Example\r\n");
    printf("========================================\r\n");
    printf("\r\n");

    /* 1. Show STM32F4 Dual Bank Layout */
    printf("[Layout 1] STM32F4 Dual Bank Layout\r\n");
    print_layout(&layout_stm32f4_dual_bank);
    analyze_layout_efficiency(&layout_stm32f4_dual_bank);

    /* 2. Show STM32U5 Large Layout */
    printf("[Layout 2] STM32U5 Large Flash Layout\r\n");
    print_layout(&layout_stm32u5_large);
    analyze_layout_efficiency(&layout_stm32u5_large);

    /* 3. Show Single Bank with Backup */
    printf("[Layout 3] Single Bank with Backup Layout\r\n");
    print_layout(&layout_single_bank_backup);
    analyze_layout_efficiency(&layout_single_bank_backup);

    /* 4. Show External Flash Layout */
    printf("[Layout 4] Internal + External SPI Flash Layout\r\n");
    print_layout(&layout_external_flash);

    /* 5. Demonstrate FOTA configuration with different layouts */
    printf("[Step 5] Configuring FOTA with STM32F4 Dual Bank layout...\r\n");

    ret = configure_fota_for_layout(&fota, &layout_stm32f4_dual_bank);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Failed to configure FOTA\r\n");
        return -1;
    }

    ret = xy_fota_set_progress_callback(&fota, layout_progress_callback, NULL);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Set progress callback failed\r\n");
        return -1;
    }

    /* Simulate a download */
    printf("\r\n");
    printf("[Step 6] Simulating FOTA download with configured layout...\r\n");

    uint8_t dummy_fw[512];
    for (int i = 0; i < (int)sizeof(dummy_fw); i++) {
        dummy_fw[i] = (uint8_t)(i & 0xFF);
    }

    ret = xy_fota_start_download(&fota, 2, sizeof(dummy_fw), false);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Start download failed: %d\r\n", ret);
        return -1;
    }

    ret = xy_fota_download_chunk(&fota, dummy_fw, sizeof(dummy_fw));
    if (ret != XY_FOTA_OK) {
        printf("[Error] Download chunk failed: %d\r\n", ret);
        return -1;
    }

    ret = xy_fota_finish_download(&fota);
    if (ret != XY_FOTA_OK) {
        printf("[Error] Finish download failed: %d\r\n", ret);
        return -1;
    }

    printf("[OK] Download completed successfully\r\n");

    /* 7. Show different chip recommendations */
    printf("\r\n");
    printf("[Step 7] Platform Recommendations:\r\n");
    printf("\r\n");
    printf("    +------------------+----------+-----------+------------------+\r\n");
    printf("    | Platform         | Flash    | FOTA Mode | Recommended Use  |\r\n");
    printf("    +------------------+----------+-----------+------------------+\r\n");
    printf("    | STM32F4xx        | 512KB    | Dual Bank | General purpose  |\r\n");
    printf("    | STM32U5xx        | 2MB      | Dual Bank | High-end IOT     |\r\n");
    printf("    | CH32V103         | 64KB     | Single    | Resource limited |\r\n");
    printf("    | CH32V208         | 128KB    | Single    | BLE + FOTA       |\r\n");
    printf("    | WCH RISC-V       | 256KB    | Dual Bank | Standard IOT     |\r\n");
    printf("    +------------------+----------+-----------+------------------+\r\n");
    printf("\r\n");

    /* Cleanup */
    printf("[Step 8] Cleanup...\r\n");
    xy_fota_deinit(&fota);
    printf("[OK] FOTA deinitialized\r\n");
    printf("\r\n");

    printf("========================================\r\n");
    printf("   Flash Layout Example Completed\r\n");
    printf("========================================\r\n");

    return 0;
}
