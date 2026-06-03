#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "xy_fota.h"

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
static int g_init_count;
static int g_deinit_count;

static void reset_fixture(void)
{
    memset(g_flash, 0xFF, sizeof(g_flash));
    g_last_write_addr = 0;
    g_last_erase_addr = 0;
    g_last_erase_size = 0;
    g_progress_current = 0;
    g_progress_total = 0;
    g_progress_user = 0;
    g_init_count = 0;
    g_deinit_count = 0;
}

static uint32_t flash_offset(uint32_t addr)
{
    if (addr >= BACKUP_BASE) {
        return (addr - BACKUP_BASE) + 2048u;
    }
    return addr - FLASH_BASE;
}

static int mock_flash_init(void)
{
    g_init_count++;
    return XY_FOTA_OK;
}

static int mock_flash_deinit(void)
{
    g_deinit_count++;
    return XY_FOTA_OK;
}

static int mock_flash_write(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t off = flash_offset(addr);
    assert(data != NULL);
    assert(off + size <= sizeof(g_flash));
    memcpy(&g_flash[off], data, size);
    g_last_write_addr = addr;
    return XY_FOTA_OK;
}

static int mock_flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t off = flash_offset(addr);
    assert(data != NULL);
    assert(off + size <= sizeof(g_flash));
    memcpy(data, &g_flash[off], size);
    return XY_FOTA_OK;
}

static int mock_flash_erase(uint32_t addr, uint32_t size)
{
    uint32_t off = flash_offset(addr);
    assert(off + size <= sizeof(g_flash));
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
    assert(xy_fota_init(NULL, &config) == XY_FOTA_INVALID_PARAM);
    assert(xy_fota_init(&fota, NULL) == XY_FOTA_INVALID_PARAM);

    config.slot_count = 0;
    assert(xy_fota_init(&fota, &config) == XY_FOTA_INVALID_PARAM);
    config = default_config();
    config.slot_size = 0;
    assert(xy_fota_init(&fota, &config) == XY_FOTA_INVALID_PARAM);
    config = default_config();
    config.mode = XY_FOTA_MODE_SINGLE_SLOT;
    config.backup_addr = 0;
    assert(xy_fota_init(&fota, &config) == XY_FOTA_INVALID_PARAM);

    config = default_config();
    assert(xy_fota_init(&fota, &config) == XY_FOTA_OK);
    assert(fota.initialized);
    assert(xy_fota_get_state(&fota) == XY_FOTA_STATE_IDLE);
    assert(xy_fota_get_state(NULL) == XY_FOTA_STATE_ERROR);
    assert(xy_fota_get_progress(NULL) == 0);
    assert(xy_fota_get_current_version(NULL) == 0);

    assert(xy_fota_deinit(&fota) == XY_FOTA_OK);
    assert(!fota.initialized);
    assert(xy_fota_deinit(NULL) == XY_FOTA_INVALID_PARAM);
}

static void test_header_crc_and_version_guards(void)
{
    xy_fota_t fota;
    xy_fota_config_t config = default_config();
    uint8_t payload[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    xy_fota_header_t header = {0};

    reset_fixture();
    assert(xy_fota_calc_crc32(payload, sizeof(payload)) == 0xCBF43926u);
    assert(!xy_fota_validate_header(NULL));
    assert(!xy_fota_validate_header(&header));
    header.magic = 0x464F5441u;
    header.image_size = 32;
    assert(xy_fota_validate_header(&header));
    header.image_size = 0;
    assert(!xy_fota_validate_header(&header));

    assert(!xy_fota_validate_version(NULL, 3));
    assert(xy_fota_init(&fota, &config) == XY_FOTA_OK);
    assert(!xy_fota_validate_version(&fota, 1));
    assert(xy_fota_validate_version(&fota, 2));
    fota.header.version = 5;
    assert(!xy_fota_validate_version(&fota, 4));
    assert(xy_fota_validate_version(&fota, 5));
}

static void test_download_writes_progress_and_control(void)
{
    xy_fota_t fota;
    xy_fota_config_t config = default_config();
    uint8_t chunk1[] = {1, 2, 3, 4};
    uint8_t chunk2[] = {5, 6};
    int user = 77;

    reset_fixture();
    assert(xy_fota_init(&fota, &config) == XY_FOTA_OK);
    assert(xy_fota_set_flash_ops(NULL, &mock_ops) == XY_FOTA_INVALID_PARAM);
    assert(xy_fota_set_flash_ops(&fota, NULL) == XY_FOTA_INVALID_PARAM);
    assert(xy_fota_set_flash_ops(&fota, &mock_ops) == XY_FOTA_OK);
    assert(xy_fota_set_backup_flash_ops(&fota, &mock_ops) == XY_FOTA_OK);
    assert(xy_fota_set_progress_callback(&fota, progress_cb, &user) == XY_FOTA_OK);
    assert(xy_fota_set_progress_callback(NULL, progress_cb, &user) == XY_FOTA_INVALID_PARAM);

    assert(xy_fota_start_download(NULL, 3, sizeof(chunk1), false) == XY_FOTA_INVALID_PARAM);
    assert(xy_fota_start_download(&fota, 3, 0, false) == XY_FOTA_INVALID_PARAM);
    assert(xy_fota_start_download(&fota, 3, sizeof(chunk1) + sizeof(chunk2), false) == XY_FOTA_OK);
    assert(fota.state == XY_FOTA_STATE_DOWNLOADING);
    assert(xy_fota_get_progress(&fota) == 0);

    assert(xy_fota_download_chunk(&fota, chunk1, sizeof(chunk1)) == XY_FOTA_OK);
    assert(g_last_write_addr == FLASH_BASE);
    assert(g_progress_current == sizeof(chunk1));
    assert(g_progress_total == sizeof(chunk1) + sizeof(chunk2));
    assert(g_progress_user == user);
    assert(xy_fota_get_progress(&fota) == 66);

    assert(xy_fota_download_chunk(&fota, chunk2, sizeof(chunk2)) == XY_FOTA_OK);
    assert(fota.state == XY_FOTA_STATE_VALIDATING);
    assert(xy_fota_get_progress(&fota) == 100);
    assert(memcmp(g_flash, chunk1, sizeof(chunk1)) == 0);
    assert(memcmp(&g_flash[sizeof(chunk1)], chunk2, sizeof(chunk2)) == 0);

    assert(xy_fota_download_chunk(&fota, chunk2, sizeof(chunk2)) == XY_FOTA_IN_PROGRESS);
    assert(xy_fota_finish_download(&fota) == XY_FOTA_OK);
    assert(fota.state == XY_FOTA_STATE_COMPLETE);
    assert(xy_fota_start_update(&fota) == XY_FOTA_OK);
    assert(fota.state == XY_FOTA_STATE_COMPLETE);
    assert(xy_fota_needs_rollback(&fota) == false);

    assert(xy_fota_cancel(&fota) == XY_FOTA_OK);
    assert(fota.state == XY_FOTA_STATE_IDLE);
    assert(fota.downloaded_bytes == 0);
    assert(xy_fota_reset(&fota) == XY_FOTA_OK);
    assert(fota.header.magic == 0);
}

static void test_single_slot_backup_download_path(void)
{
    xy_fota_t fota;
    xy_fota_config_t config = default_config();
    uint8_t chunk[] = {9, 8, 7, 6};

    reset_fixture();
    config.mode = XY_FOTA_MODE_SINGLE_SLOT;
    assert(xy_fota_init(&fota, &config) == XY_FOTA_OK);
    assert(xy_fota_set_flash_ops(&fota, &mock_ops) == XY_FOTA_OK);

    assert(xy_fota_start_download(&fota, 3, sizeof(chunk), false) == XY_FOTA_OK);
    assert(xy_fota_download_chunk(&fota, chunk, sizeof(chunk)) == XY_FOTA_OK);
    assert(g_last_write_addr == BACKUP_BASE);
}

int main(void)
{
    test_init_validation_and_state_helpers();
    test_header_crc_and_version_guards();
    test_download_writes_progress_and_control();
    test_single_slot_backup_download_path();
    return 0;
}
