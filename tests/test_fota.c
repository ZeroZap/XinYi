/**
 * @file test_fota.c
 * @brief FOTA Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"
#include "xy_fota.h"

/* ==================== Test Fixtures ==================== */

static xy_fota_t test_fota;
static uint8_t progress_callback_called = 0;

void setUp(void)
{
    memset(&test_fota, 0, sizeof(test_fota));
    progress_callback_called = 0;
}

void tearDown(void)
{
    xy_fota_deinit(&test_fota);
}

/* ==================== Helper Functions ==================== */

static void progress_callback(uint8_t progress, uint32_t total, void *user_data)
{
    (void)total;
    (void)user_data;
    progress_callback_called = 1;
    printf("Progress: %d%%\n", progress);
}

/* ==================== FOTA Init Tests ==================== */

void test_fota_init(void)
{
    xy_fota_config_t config = {
        .flash_base_addr = 0x08010000,
        .slot_size = 128 * 1024,
        .slot_count = 2,
    };
    
    int result = xy_fota_init(&test_fota, &config);
    TEST_ASSERT_EQUAL(XY_FOTA_OK, result);
    TEST_ASSERT_TRUE(test_fota.initialized);
    TEST_ASSERT_EQUAL(XY_FOTA_STATE_IDLE, test_fota.state);
}

void test_fota_init_invalid_params(void)
{
    int result;
    
    result = xy_fota_init(NULL, NULL);
    TEST_ASSERT_EQUAL(XY_FOTA_INVALID_PARAM, result);
}

/* ==================== FOTA Download Tests ==================== */

void test_fota_start_download(void)
{
    xy_fota_config_t config = {
        .flash_base_addr = 0x08010000,
        .slot_size = 128 * 1024,
    };
    
    xy_fota_init(&test_fota, &config);
    
    int result = xy_fota_start_download(&test_fota, 0x0100, 1024);
    TEST_ASSERT_EQUAL(XY_FOTA_OK, result);
    TEST_ASSERT_EQUAL(XY_FOTA_STATE_DOWNLOADING, test_fota.state);
    TEST_ASSERT_EQUAL(1024, test_fota.header.image_size);
}

void test_fota_download_chunk(void)
{
    xy_fota_config_t config = {
        .flash_base_addr = 0x08010000,
        .slot_size = 128 * 1024,
    };
    
    xy_fota_init(&test_fota, &config);
    xy_fota_start_download(&test_fota, 0x0100, 1024);
    
    uint8_t data[256] = {0};
    int result = xy_fota_download_chunk(&test_fota, data, 256);
    TEST_ASSERT_EQUAL(XY_FOTA_OK, result);
    TEST_ASSERT_EQUAL(256, test_fota.downloaded_bytes);
}

void test_fota_download_with_progress(void)
{
    xy_fota_config_t config = {
        .flash_base_addr = 0x08010000,
        .slot_size = 128 * 1024,
    };
    
    xy_fota_init(&test_fota, &config);
    xy_fota_set_progress_callback(&test_fota, progress_callback, NULL);
    xy_fota_start_download(&test_fota, 0x0100, 1024);
    
    uint8_t data[256] = {0};
    xy_fota_download_chunk(&test_fota, data, 256);
    
    TEST_ASSERT_EQUAL(1, progress_callback_called);
}

void test_fota_finish_download(void)
{
    xy_fota_config_t config = {
        .flash_base_addr = 0x08010000,
        .slot_size = 128 * 1024,
    };
    
    xy_fota_init(&test_fota, &config);
    xy_fota_start_download(&test_fota, 0x0100, 256);
    
    uint8_t data[256] = {0};
    xy_fota_download_chunk(&test_fota, data, 256);
    
    int result = xy_fota_finish_download(&test_fota);
    TEST_ASSERT_EQUAL(XY_FOTA_OK, result);
    TEST_ASSERT_EQUAL(XY_FOTA_STATE_COMPLETE, test_fota.state);
}

/* ==================== FOTA Update Tests ==================== */

void test_fota_start_update(void)
{
    xy_fota_config_t config = {
        .flash_base_addr = 0x08010000,
        .slot_size = 128 * 1024,
    };
    
    xy_fota_init(&test_fota, &config);
    xy_fota_start_download(&test_fota, 0x0100, 256);
    
    uint8_t data[256] = {0};
    xy_fota_download_chunk(&test_fota, data, 256);
    xy_fota_finish_download(&test_fota);
    
    int result = xy_fota_start_update(&test_fota);
    TEST_ASSERT_EQUAL(XY_FOTA_OK, result);
}

/* ==================== FOTA State Tests ==================== */

void test_fota_get_state(void)
{
    xy_fota_config_t config = {
        .flash_base_addr = 0x08010000,
        .slot_size = 128 * 1024,
    };
    
    xy_fota_init(&test_fota, &config);
    
    xy_fota_state_t state = xy_fota_get_state(&test_fota);
    TEST_ASSERT_EQUAL(XY_FOTA_STATE_IDLE, state);
}

void test_fota_get_progress(void)
{
    xy_fota_config_t config = {
        .flash_base_addr = 0x08010000,
        .slot_size = 128 * 1024,
    };
    
    xy_fota_init(&test_fota, &config);
    xy_fota_start_download(&test_fota, 0x0100, 1024);
    
    uint8_t data[512] = {0};
    xy_fota_download_chunk(&test_fota, data, 512);
    
    uint8_t progress = xy_fota_get_progress(&test_fota);
    TEST_ASSERT_EQUAL(50, progress);
}

/* ==================== FOTA Control Tests ==================== */

void test_fota_cancel(void)
{
    xy_fota_config_t config = {
        .flash_base_addr = 0x08010000,
        .slot_size = 128 * 1024,
    };
    
    xy_fota_init(&test_fota, &config);
    xy_fota_start_download(&test_fota, 0x0100, 1024);
    
    int result = xy_fota_cancel(&test_fota);
    TEST_ASSERT_EQUAL(XY_FOTA_OK, result);
    TEST_ASSERT_EQUAL(XY_FOTA_STATE_IDLE, test_fota.state);
}

void test_fota_reset(void)
{
    xy_fota_config_t config = {
        .flash_base_addr = 0x08010000,
        .slot_size = 128 * 1024,
    };
    
    xy_fota_init(&test_fota, &config);
    xy_fota_start_download(&test_fota, 0x0100, 1024);
    
    int result = xy_fota_reset(&test_fota);
    TEST_ASSERT_EQUAL(XY_FOTA_OK, result);
    TEST_ASSERT_EQUAL(0, test_fota.downloaded_bytes);
}

/* ==================== CRC32 Tests ==================== */

void test_fota_calc_crc32(void)
{
    uint8_t data[] = "123456789";
    uint32_t crc = xy_fota_calc_crc32(data, 9);
    
    /* Standard CRC32 value */
    TEST_ASSERT_EQUAL_UINT32(0xCBF43926, crc);
}

void test_fota_validate_header(void)
{
    xy_fota_header_t header;
    
    /* Valid header */
    header.magic = 0xF0T4A512;
    header.image_size = 1024;
    TEST_ASSERT_TRUE(xy_fota_validate_header(&header));
    
    /* Invalid magic */
    header.magic = 0x00000000;
    TEST_ASSERT_FALSE(xy_fota_validate_header(&header));
    
    /* Invalid size */
    header.magic = 0xF0T4A512;
    header.image_size = 0;
    TEST_ASSERT_FALSE(xy_fota_validate_header(&header));
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Init Tests */
    RUN_TEST(test_fota_init);
    RUN_TEST(test_fota_init_invalid_params);

    /* Download Tests */
    RUN_TEST(test_fota_start_download);
    RUN_TEST(test_fota_download_chunk);
    RUN_TEST(test_fota_download_with_progress);
    RUN_TEST(test_fota_finish_download);

    /* Update Tests */
    RUN_TEST(test_fota_start_update);

    /* State Tests */
    RUN_TEST(test_fota_get_state);
    RUN_TEST(test_fota_get_progress);

    /* Control Tests */
    RUN_TEST(test_fota_cancel);
    RUN_TEST(test_fota_reset);

    /* CRC32 Tests */
    RUN_TEST(test_fota_calc_crc32);
    RUN_TEST(test_fota_validate_header);

    return UNITY_END();
}
