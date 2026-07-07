#include "xy_fota.h"
#include "unity.h"
#include "fff.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

#define FLASH_BASE 0x08010000u
#define BACKUP_BASE 0x08100000u
#define SLOT_SIZE 1024u

static uint8_t g_flash[4096];
static uint32_t g_last_write_addr;
static uint32_t g_last_erase_addr;
static uint32_t g_last_erase_size;
static uint32_t g_progress_current;
static uint32_t g_progress_total;
static int g_progress_user;

FAKE_VALUE_FUNC(int, mock_flash_init)
FAKE_VALUE_FUNC(int, mock_flash_deinit)
FAKE_VALUE_FUNC(int, mock_flash_write, uint32_t, const uint8_t *, uint32_t)
FAKE_VALUE_FUNC(int, mock_flash_read, uint32_t, uint8_t *, uint32_t)
FAKE_VALUE_FUNC(int, mock_flash_erase, uint32_t, uint32_t)

static int mock_flash_write_impl(uint32_t addr, const uint8_t *data, uint32_t size);
static int mock_flash_read_impl(uint32_t addr, uint8_t *data, uint32_t size);
static int mock_flash_erase_impl(uint32_t addr, uint32_t size);

void setUp(void)
{
}

void tearDown(void)
{
}

static void mock_flash_reset_fakes(void)
{
    RESET_FAKE(mock_flash_init);
    RESET_FAKE(mock_flash_deinit);
    RESET_FAKE(mock_flash_write);
    RESET_FAKE(mock_flash_read);
    RESET_FAKE(mock_flash_erase);
    FFF_RESET_HISTORY();

    mock_flash_init_fake.return_val = XY_FOTA_OK;
    mock_flash_deinit_fake.return_val = XY_FOTA_OK;
    mock_flash_write_fake.custom_fake = mock_flash_write_impl;
    mock_flash_read_fake.custom_fake = mock_flash_read_impl;
    mock_flash_erase_fake.custom_fake = mock_flash_erase_impl;
}

static void reset_fixture(void)
{
    memset(g_flash, 0xFF, sizeof(g_flash));
    g_last_write_addr = 0;
    g_last_erase_addr = 0;
    g_last_erase_size = 0;
    g_progress_current = 0;
    g_progress_total = 0;
    g_progress_user = 0;
    mock_flash_reset_fakes();
}

static uint32_t flash_offset(uint32_t addr)
{
    if (addr >= BACKUP_BASE) {
        return (addr - BACKUP_BASE) + 2048u;
    }
    return addr - FLASH_BASE;
}

static int mock_flash_write_impl(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t off = flash_offset(addr);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(sizeof(g_flash), off + size);
    memcpy(&g_flash[off], data, size);
    g_last_write_addr = addr;
    return XY_FOTA_OK;
}

static int mock_flash_read_impl(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t off = flash_offset(addr);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(sizeof(g_flash), off + size);
    memcpy(data, &g_flash[off], size);
    return XY_FOTA_OK;
}

static int mock_flash_erase_impl(uint32_t addr, uint32_t size)
{
    uint32_t off = flash_offset(addr);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(sizeof(g_flash), off + size);
    memset(&g_flash[off], 0xFF, size);
    g_last_erase_addr = addr;
    g_last_erase_size = size;
    return XY_FOTA_OK;
}

static const xy_fota_flash_ops_t mock_ops = {
    .init = mock_flash_init,
    .write = mock_flash_write,
    .read = mock_flash_read,
    .erase = mock_flash_erase,
    .deinit = mock_flash_deinit,
};

static xy_fota_config_t default_config(void)
{
    xy_fota_config_t config = {0};
    config.mode = XY_FOTA_MODE_DUAL_BANK;
    config.flash_base_addr = FLASH_BASE;
    config.slot_size = SLOT_SIZE;
    config.slot_count = 2;
    config.backup_addr = BACKUP_BASE;
    config.backup_size = SLOT_SIZE;
    config.enable_rollback = true;
    config.min_version = 2;
    return config;
}

static void progress_cb(uint32_t current, uint32_t total, void *user_data)
{
    g_progress_current = current;
    g_progress_total = total;
    g_progress_user = *(int *)user_data;
}

static void test_init_validation_and_state_helpers(void)
{
    xy_fota_t fota;
    xy_fota_config_t config = default_config();

    reset_fixture();
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, xy_fota_init(NULL, &config));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, xy_fota_init(&fota, NULL));

    config.slot_count = 0;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, xy_fota_init(&fota, &config));
    config = default_config();
    config.slot_size = 0;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, xy_fota_init(&fota, &config));
    config = default_config();
    config.mode = XY_FOTA_MODE_SINGLE_SLOT;
    config.backup_addr = 0;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, xy_fota_init(&fota, &config));

    config = default_config();
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_init(&fota, &config));
    TEST_ASSERT_TRUE(fota.initialized);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_STATE_IDLE, xy_fota_get_state(&fota));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_STATE_ERROR, xy_fota_get_state(NULL));
    TEST_ASSERT_EQUAL_UINT8(0, xy_fota_get_progress(NULL));
    TEST_ASSERT_EQUAL_UINT32(0, xy_fota_get_current_version(NULL));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_deinit(&fota));
    TEST_ASSERT_FALSE(fota.initialized);
    TEST_ASSERT_EQUAL_UINT(0, mock_flash_init_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0, mock_flash_deinit_fake.call_count);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, xy_fota_deinit(NULL));
}

static void test_header_crc_and_version_guards(void)
{
    xy_fota_t fota;
    xy_fota_config_t config = default_config();
    uint8_t payload[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    xy_fota_header_t header = {0};

    reset_fixture();
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, xy_fota_calc_crc32(payload, sizeof(payload)));
    TEST_ASSERT_FALSE(xy_fota_validate_header(NULL));
    TEST_ASSERT_FALSE(xy_fota_validate_header(&header));
    header.magic = 0x464F5441u;
    header.image_size = 32;
    TEST_ASSERT_TRUE(xy_fota_validate_header(&header));
    header.image_size = 0;
    TEST_ASSERT_FALSE(xy_fota_validate_header(&header));

    TEST_ASSERT_FALSE(xy_fota_validate_version(NULL, 3));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_init(&fota, &config));
    TEST_ASSERT_FALSE(xy_fota_validate_version(&fota, 1));
    TEST_ASSERT_TRUE(xy_fota_validate_version(&fota, 2));
    fota.header.version = 5;
    TEST_ASSERT_FALSE(xy_fota_validate_version(&fota, 4));
    TEST_ASSERT_TRUE(xy_fota_validate_version(&fota, 5));
}

static void test_download_writes_progress_and_control(void)
{
    xy_fota_t fota;
    xy_fota_config_t config = default_config();
    uint8_t chunk1[] = {1, 2, 3, 4};
    uint8_t chunk2[] = {5, 6};
    int user = 77;

    reset_fixture();
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_init(&fota, &config));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, xy_fota_set_flash_ops(NULL, &mock_ops));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, xy_fota_set_flash_ops(&fota, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_set_flash_ops(&fota, &mock_ops));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_set_backup_flash_ops(&fota, &mock_ops));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_set_progress_callback(&fota, progress_cb, &user));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, xy_fota_set_progress_callback(NULL, progress_cb, &user));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, xy_fota_start_download(NULL, 3, sizeof(chunk1), false));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_INVALID_PARAM, xy_fota_start_download(&fota, 3, 0, false));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK,
                          xy_fota_start_download(&fota, 3, sizeof(chunk1) + sizeof(chunk2), false));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_STATE_DOWNLOADING, fota.state);
    TEST_ASSERT_EQUAL_UINT8(0, xy_fota_get_progress(&fota));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_download_chunk(&fota, chunk1, sizeof(chunk1)));
    TEST_ASSERT_EQUAL_UINT(1, mock_flash_write_fake.call_count);
    TEST_ASSERT_EQUAL_HEX32(FLASH_BASE, mock_flash_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(chunk1, mock_flash_write_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(sizeof(chunk1), mock_flash_write_fake.arg2_val);
    TEST_ASSERT_EQUAL_HEX32(FLASH_BASE, g_last_write_addr);
    TEST_ASSERT_EQUAL_UINT32(sizeof(chunk1), g_progress_current);
    TEST_ASSERT_EQUAL_UINT32(sizeof(chunk1) + sizeof(chunk2), g_progress_total);
    TEST_ASSERT_EQUAL_INT(user, g_progress_user);
    TEST_ASSERT_EQUAL_UINT8(66, xy_fota_get_progress(&fota));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_download_chunk(&fota, chunk2, sizeof(chunk2)));
    TEST_ASSERT_EQUAL_UINT(2, mock_flash_write_fake.call_count);
    TEST_ASSERT_EQUAL_HEX32(FLASH_BASE + sizeof(chunk1), mock_flash_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(chunk2, mock_flash_write_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(sizeof(chunk2), mock_flash_write_fake.arg2_val);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_STATE_VALIDATING, fota.state);
    TEST_ASSERT_EQUAL_UINT8(100, xy_fota_get_progress(&fota));
    TEST_ASSERT_EQUAL_MEMORY(chunk1, g_flash, sizeof(chunk1));
    TEST_ASSERT_EQUAL_MEMORY(chunk2, &g_flash[sizeof(chunk1)], sizeof(chunk2));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_IN_PROGRESS, xy_fota_download_chunk(&fota, chunk2, sizeof(chunk2)));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_finish_download(&fota));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_STATE_COMPLETE, fota.state);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_start_update(&fota));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_STATE_COMPLETE, fota.state);
    TEST_ASSERT_FALSE(xy_fota_needs_rollback(&fota));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_cancel(&fota));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_STATE_IDLE, fota.state);
    TEST_ASSERT_EQUAL_UINT32(0, fota.downloaded_bytes);
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_reset(&fota));
    TEST_ASSERT_EQUAL_HEX32(0, fota.header.magic);
}

static void test_single_slot_backup_download_path(void)
{
    xy_fota_t fota;
    xy_fota_config_t config = default_config();
    uint8_t chunk[] = {9, 8, 7, 6};

    reset_fixture();
    config.mode = XY_FOTA_MODE_SINGLE_SLOT;
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_init(&fota, &config));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_set_flash_ops(&fota, &mock_ops));

    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_start_download(&fota, 3, sizeof(chunk), false));
    TEST_ASSERT_EQUAL_INT(XY_FOTA_OK, xy_fota_download_chunk(&fota, chunk, sizeof(chunk)));
    TEST_ASSERT_EQUAL_UINT(1, mock_flash_write_fake.call_count);
    TEST_ASSERT_EQUAL_HEX32(BACKUP_BASE, mock_flash_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(chunk, mock_flash_write_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(sizeof(chunk), mock_flash_write_fake.arg2_val);
    TEST_ASSERT_EQUAL_HEX32(BACKUP_BASE, g_last_write_addr);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_validation_and_state_helpers);
    RUN_TEST(test_header_crc_and_version_guards);
    RUN_TEST(test_download_writes_progress_and_control);
    RUN_TEST(test_single_slot_backup_download_path);
    return UNITY_END();
}
