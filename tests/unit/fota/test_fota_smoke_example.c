/**
 * @file test_fota_smoke_example.c
 * @brief Build-guarded host-safe smoke example for the public FOTA update flow.
 *
 * This example intentionally uses fake Flash callbacks. Board Flash geometry,
 * bootloader handoff, and external NOR hardware logs belong in board/project
 * validation records, not in this platform-independent component smoke.
 */

#include "unity.h"
#include "xy_fota.h"

#include <stdint.h>
#include <string.h>

#define SMOKE_FLASH_BASE  0x08010000u
#define SMOKE_BACKUP_BASE 0x08100000u
#define SMOKE_SLOT_SIZE   1024u

static uint8_t g_flash[4096];
static unsigned g_flash_write_calls;
static unsigned g_flash_read_calls;
static unsigned g_flash_erase_calls;
static uint32_t g_progress_current;
static uint32_t g_progress_total;

static uint32_t smoke_flash_offset(uint32_t addr)
{
    if (addr >= SMOKE_BACKUP_BASE) {
        return (addr - SMOKE_BACKUP_BASE) + 2048u;
    }
    return addr - SMOKE_FLASH_BASE;
}

static int smoke_flash_init(void)
{
    return XY_FOTA_OK;
}

static int smoke_flash_write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t off = smoke_flash_offset(addr);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(sizeof(g_flash), off + size);
    memcpy(&g_flash[off], data, size);
    g_flash_write_calls++;
    return XY_FOTA_OK;
}

static int smoke_flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t off = smoke_flash_offset(addr);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(sizeof(g_flash), off + size);
    memcpy(data, &g_flash[off], size);
    g_flash_read_calls++;
    return XY_FOTA_OK;
}

static int smoke_flash_erase(uint32_t addr, uint32_t size)
{
    uint32_t off = smoke_flash_offset(addr);

    TEST_ASSERT_LESS_OR_EQUAL_UINT32(sizeof(g_flash), off + size);
    memset(&g_flash[off], 0xFF, size);
    g_flash_erase_calls++;
    return XY_FOTA_OK;
}

static int smoke_flash_deinit(void)
{
    return XY_FOTA_OK;
}

static const xy_fota_flash_ops_t smoke_flash_ops = {
    .init = smoke_flash_init,
    .write = smoke_flash_write,
    .read = smoke_flash_read,
    .erase = smoke_flash_erase,
    .deinit = smoke_flash_deinit,
};

static xy_fota_config_t smoke_dual_bank_config(void)
{
    xy_fota_config_t config = {0};

    config.mode = XY_FOTA_MODE_DUAL_BANK;
    config.flash_base_addr = SMOKE_FLASH_BASE;
    config.slot_size = SMOKE_SLOT_SIZE;
    config.slot_count = 2;
    config.backup_addr = SMOKE_BACKUP_BASE;
    config.backup_size = SMOKE_SLOT_SIZE;
    config.enable_rollback = true;
    config.min_version = 1;
    return config;
}

static void smoke_progress_cb(uint32_t current, uint32_t total, void *user_data)
{
    TEST_ASSERT_NULL(user_data);
    g_progress_current = current;
    g_progress_total = total;
}

void setUp(void)
{
    memset(g_flash, 0xFF, sizeof(g_flash));
    g_flash_write_calls = 0U;
    g_flash_read_calls = 0U;
    g_flash_erase_calls = 0U;
    g_progress_current = 0U;
    g_progress_total = 0U;
}

void tearDown(void)
{
}

static void test_public_dual_bank_flow_uses_fake_flash_callbacks(void)
{
    xy_fota_t fota;
    xy_fota_config_t config = smoke_dual_bank_config();
    const uint8_t image[] = {0x46, 0x4F, 0x54, 0x41, 0x01, 0x02, 0x03, 0x04};
    uint8_t stored[sizeof(image)] = {0};

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_init(&fota, &config));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_set_flash_ops(&fota, &smoke_flash_ops));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_set_backup_flash_ops(&fota, &smoke_flash_ops));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_set_progress_callback(&fota, smoke_progress_cb, NULL));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_start_download(&fota, 2U, sizeof(image), false));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_download_chunk(&fota, image, sizeof(image)));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_STATE_VALIDATING, xy_fota_get_state(&fota));
    TEST_ASSERT_EQUAL_UINT32(sizeof(image), g_progress_current);
    TEST_ASSERT_EQUAL_UINT32(sizeof(image), g_progress_total);
    TEST_ASSERT_EQUAL_UINT8(100U, xy_fota_get_progress(&fota));
    TEST_ASSERT_EQUAL_UINT(1U, g_flash_write_calls);

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          smoke_flash_read(SMOKE_FLASH_BASE, stored, sizeof(stored)));
    TEST_ASSERT_EQUAL_MEMORY(image, stored, sizeof(image));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_finish_download(&fota));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_STATE_COMPLETE, xy_fota_get_state(&fota));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_NOT_SUPPORTED, xy_fota_start_update(&fota));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_STATE_COMPLETE, xy_fota_get_state(&fota));
    TEST_ASSERT_EQUAL_UINT32(2U, xy_fota_get_current_version(&fota));

    TEST_ASSERT_EQUAL_UINT(1U, g_flash_read_calls);
    TEST_ASSERT_EQUAL_UINT(0U, g_flash_erase_calls);
}

static void test_external_backup_policy_stays_callback_based_and_host_safe(void)
{
    xy_fota_t fota;
    xy_fota_config_t config = smoke_dual_bank_config();
    const uint8_t image[] = {0xAA, 0x55, 0x10, 0x20};

    config.mode = XY_FOTA_MODE_SINGLE_SLOT;

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_init(&fota, &config));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_set_flash_ops(&fota, &smoke_flash_ops));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_set_backup_flash_ops(&fota, &smoke_flash_ops));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_start_download(&fota, 2U, sizeof(image), false));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_download_chunk(&fota, image, sizeof(image)));
    TEST_ASSERT_EQUAL_MEMORY(image, &g_flash[2048], sizeof(image));
    TEST_ASSERT_EQUAL_UINT(1U, g_flash_write_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_public_dual_bank_flow_uses_fake_flash_callbacks);
    RUN_TEST(test_external_backup_policy_stays_callback_based_and_host_safe);
    return UNITY_END();
}
