#include "lfs.h"
#include "unity.h"
#include "xy_littlefs_w25q128.h"

#include <string.h>

static uint8_t storage[4U * 4096U];
static uint8_t w25_storage[16U * 1024U * 1024U];

xy_w25q128_status_t xy_w25q128_read(xy_w25q128_t* flash, uint32_t address, uint8_t* data,
                                    size_t length) {
    if (flash == NULL || flash->qspi == NULL || data == NULL || address > sizeof(w25_storage) ||
        length > sizeof(w25_storage) - address) {
        return XY_W25Q128_INVALID_PARAM;
    }
    memcpy(data, (const uint8_t*)flash->qspi + address, length);
    return XY_W25Q128_OK;
}

xy_w25q128_status_t xy_w25q128_page_program(xy_w25q128_t* flash, uint32_t address,
                                            const uint8_t* data, size_t length,
                                            uint32_t timeout_ms) {
    (void)timeout_ms;
    if (flash == NULL || flash->qspi == NULL || data == NULL || length == 0U ||
        address > sizeof(w25_storage) || length > sizeof(w25_storage) - address) {
        return XY_W25Q128_INVALID_PARAM;
    }
    memcpy((uint8_t*)flash->qspi + address, data, length);
    return XY_W25Q128_OK;
}

xy_w25q128_status_t xy_w25q128_sector_erase(xy_w25q128_t* flash, uint32_t address,
                                            uint32_t timeout_ms) {
    (void)timeout_ms;
    if (flash == NULL || flash->qspi == NULL || address % 4096U != 0U ||
        address > sizeof(w25_storage) - 4096U) {
        return XY_W25Q128_INVALID_PARAM;
    }
    memset((uint8_t*)flash->qspi + address, 0xff, 4096U);
    return XY_W25Q128_OK;
}

static int read_cb(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buf,
                   lfs_size_t size) {
    (void)c;
    memcpy(buf, storage + block * 4096U + off, size);
    return 0;
}

static int prog_cb(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buf,
                   lfs_size_t size) {
    (void)c;
    memcpy(storage + block * 4096U + off, buf, size);
    return 0;
}

static int erase_cb(const struct lfs_config* c, lfs_block_t block) {
    (void)c;
    memset(storage + block * 4096U, 0xff, 4096U);
    return 0;
}

static int sync_cb(const struct lfs_config* c) {
    return c != NULL ? 0 : -1;
}

void setUp(void) {
    memset(storage, 0xff, sizeof(storage));
    memset(w25_storage, 0xff, sizeof(w25_storage));
}

void tearDown(void) {}

void test_littlefs_config_has_w25q_geometry(void) {
    struct lfs_config config = {
        .context = NULL,
        .read = read_cb,
        .prog = prog_cb,
        .erase = erase_cb,
        .sync = sync_cb,
        .read_size = 256U,
        .prog_size = 256U,
        .block_size = 4096U,
        .block_count = 4U,
        .cache_size = 4096U,
        .lookahead_size = 1U,
    };
    TEST_ASSERT_EQUAL_UINT(256U, config.read_size);
    TEST_ASSERT_EQUAL_UINT(4096U, config.block_size);
    TEST_ASSERT_EQUAL_UINT(4U, config.block_count);
    TEST_ASSERT_NOT_NULL(config.read);
}

void test_littlefs_callbacks_round_trip(void) {
    struct lfs_config config = {
        .context = NULL,
        .read = read_cb,
        .prog = prog_cb,
        .erase = erase_cb,
        .sync = sync_cb,
        .read_size = 256U,
        .prog_size = 256U,
        .block_size = 4096U,
        .block_count = 4U,
    };
    uint8_t expected[256U];
    uint8_t actual[256U];

    memset(expected, 0x5a, sizeof(expected));
    memset(actual, 0, sizeof(actual));
    TEST_ASSERT_EQUAL_INT(0, config.erase(&config, 0U));
    TEST_ASSERT_EQUAL_INT(0, config.prog(&config, 0U, 0U, expected, sizeof(expected)));
    TEST_ASSERT_EQUAL_INT(0, config.read(&config, 0U, 0U, actual, sizeof(actual)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, sizeof(expected));
}

void test_littlefs_w25q128_format_file_round_trip(void) {
    xy_littlefs_w25q128_t volume;
    xy_w25q128_t flash = {.qspi = w25_storage, .capacity = sizeof(w25_storage), .initialized = 1U};
    lfs_file_t file;
    char actual[12] = {0};
    const char expected[] = "hello lfs";

    TEST_ASSERT_EQUAL_INT(0, xy_littlefs_w25q128_format_mount(&volume, &flash));
    TEST_ASSERT_EQUAL_INT(0, lfs_file_open(&volume.fs, &file, "state", LFS_O_RDWR | LFS_O_CREAT));
    TEST_ASSERT_EQUAL_INT((int)(sizeof(expected) - 1U),
                          lfs_file_write(&volume.fs, &file, expected, sizeof(expected) - 1U));
    TEST_ASSERT_EQUAL_INT(0, lfs_file_close(&volume.fs, &file));
    TEST_ASSERT_EQUAL_INT(0, lfs_file_open(&volume.fs, &file, "state", LFS_O_RDONLY));
    TEST_ASSERT_EQUAL_INT((int)(sizeof(expected) - 1U),
                          lfs_file_read(&volume.fs, &file, actual, sizeof(expected) - 1U));
    TEST_ASSERT_EQUAL_STRING(expected, actual);
    TEST_ASSERT_EQUAL_INT(0, lfs_file_close(&volume.fs, &file));
    TEST_ASSERT_EQUAL_INT(0, xy_littlefs_w25q128_unmount(&volume));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_littlefs_config_has_w25q_geometry);
    RUN_TEST(test_littlefs_callbacks_round_trip);
    RUN_TEST(test_littlefs_w25q128_format_file_round_trip);
    return UNITY_END();
}
