#include "lfs.h"
#include "unity.h"

#include <string.h>

static uint8_t storage[4U * 4096U];

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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_littlefs_config_has_w25q_geometry);
    RUN_TEST(test_littlefs_callbacks_round_trip);
    return UNITY_END();
}
