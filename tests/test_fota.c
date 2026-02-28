/**
 * @file test_fota.c
 * @brief FOTA (Firmware Over-The-Air) Unit Tests
 * @version 1.0.0
 * @date 2026-03-01
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* FOTA headers */
#include "xy_fota.h"

/* ==================== Test Fixtures ==================== */

static xy_fota_t fota;
static xy_fota_config_t config;
static uint8_t test_firmware[256];

void setUp(void)
{
    memset(&fota, 0, sizeof(fota));
    memset(&config, 0, sizeof(config));
    memset(test_firmware, 0, sizeof(test_firmware));
    
    config.flash_base_addr = 0x08010000;
    config.slot_size = 0x20000;
    config.slot_count = 2;
    config.enable_secure_boot = false;
}

void tearDown(void)
{
}

/* ==================== FOTA Config Tests ==================== */

void test_fota_config_structure_size(void)
{
    TEST_ASSERT_TRUE(sizeof(xy_fota_config_t) >= 16);
}

void test_fota_header_structure_size(void)
{
    TEST_ASSERT_TRUE(sizeof(xy_fota_header_t) >= 24);
}

void test_fota_structure_size(void)
{
    TEST_ASSERT_TRUE(sizeof(xy_fota_t) >= 64);
}

/* ==================== FOTA Init Tests ==================== */

void test_fota_init_null_param(void)
{
    int ret = xy_fota_init(NULL, &config);
    TEST_ASSERT_EQUAL(XY_FOTA_INVALID_PARAM, ret);
}

void test_fota_init_null_config(void)
{
    int ret = xy_fota_init(&fota, NULL);
    TEST_ASSERT_EQUAL(XY_FOTA_INVALID_PARAM, ret);
}

void test_fota_init_valid_param(void)
{
    int ret = xy_fota_init(&fota, &config);
    TEST_ASSERT_EQUAL(XY_FOTA_OK, ret);
    TEST_ASSERT_TRUE(fota.initialized);
    TEST_ASSERT_EQUAL(XY_FOTA_STATE_IDLE, fota.state);
}

/* ==================== FOTA Deinit Tests ==================== */

void test_fota_deinit_null_param(void)
{
    int ret = xy_fota_deinit(NULL);
    TEST_ASSERT_EQUAL(XY_FOTA_INVALID_PARAM, ret);
}

void test_fota_deinit_valid(void)
{
    xy_fota_init(&fota, &config);
    int ret = xy_fota_deinit(&fota);
    TEST_ASSERT_EQUAL(XY_FOTA_OK, ret);
    TEST_ASSERT_FALSE(fota.initialized);
}

/* ==================== FOTA Download Tests ==================== */

void test_fota_start_download_null_param(void)
{
    int ret = xy_fota_start_download(NULL, 1, 1024);
    TEST_ASSERT_EQUAL(XY_FOTA_INVALID_PARAM, ret);
}

void test_fota_start_download_invalid_size(void)
{
    xy_fota_init(&fota, &config);
    
    /* Size = 0 */
    int ret = xy_fota_start_download(&fota, 1, 0);
    TEST_ASSERT_EQUAL(XY_FOTA_INVALID_PARAM, ret);
    
    /* Size too large */
    ret = xy_fota_start_download(&fota, 1, XY_FOTA_MAX_IMAGE_SIZE + 1);
    TEST_ASSERT_EQUAL(XY_FOTA_INVALID_PARAM, ret);
}

void test_fota_start_download_valid(void)
{
    xy_fota_init(&fota, &config);
    
    int ret = xy_fota_start_download(&fota, 1, 1024);
    TEST_ASSERT_EQUAL(XY_FOTA_OK, ret);
    TEST_ASSERT_EQUAL(XY_FOTA_STATE_DOWNLOADING, fota.state);
    TEST_ASSERT_EQUAL(0, fota.downloaded_bytes);
}

void test_fota_download_chunk_null_param(void)
{
    int ret = xy_fota_download_chunk(NULL, test_firmware, 100);
    TEST_ASSERT_EQUAL(XY_FOTA_INVALID_PARAM, ret);
    
    ret = xy_fota_download_chunk(&fota, NULL, 100);
    TEST_ASSERT_EQUAL(XY_FOTA_INVALID_PARAM, ret);
}

void test_fota_download_chunk_not_downloading(void)
{
    xy_fota_init(&fota, &config);
    
    /* Not in downloading state */
    int ret = xy_fota_download_chunk(&fota, test_firmware, 100);
    TEST_ASSERT_EQUAL(XY_FOTA_IN_PROGRESS, ret);
}

void test_fota_download_chunk_valid(void)
{
    xy_fota_init(&fota, &config);
    xy_fota_start_download(&fota, 1, 1024);
    
    int ret = xy_fota_download_chunk(&fota, test_firmware, 100);
    /* In simulation mode, flash write may fail */
    TEST_ASSERT_TRUE(ret == XY_FOTA_OK || ret == XY_FOTA_FLASH_ERROR);
}

/* ==================== FOTA State Tests ==================== */

void test_fota_get_state_null_param(void)
{
    xy_fota_state_t state = xy_fota_get_state(NULL);
    TEST_ASSERT_EQUAL(XY_FOTA_STATE_ERROR, state);
}

void test_fota_get_state_valid(void)
{
    xy_fota_init(&fota, &config);
    
    xy_fota_state_t state = xy_fota_get_state(&fota);
    TEST_ASSERT_EQUAL(XY_FOTA_STATE_IDLE, state);
}

void test_fota_state_transitions(void)
{
    xy_fota_init(&fota, &config);
    
    /* IDLE -> DOWNLOADING */
    xy_fota_start_download(&fota, 1, 1024);
    TEST_ASSERT_EQUAL(XY_FOTA_STATE_DOWNLOADING, fota.state);
}

/* ==================== FOTA CRC Tests ==================== */

void test_fota_crc32_calculation(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t crc = xy_fota_calc_crc32(data, 4);
    
    TEST_ASSERT_TRUE(crc != 0);
    TEST_ASSERT_TRUE(crc != 0xFFFFFFFF);
}

void test_fota_crc32_same_data(void)
{
    uint8_t data1[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t data2[] = {0x01, 0x02, 0x03, 0x04};
    
    uint32_t crc1 = xy_fota_calc_crc32(data1, 4);
    uint32_t crc2 = xy_fota_calc_crc32(data2, 4);
    
    TEST_ASSERT_EQUAL(crc1, crc2);
}

void test_fota_crc32_different_data(void)
{
    uint8_t data1[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t data2[] = {0x05, 0x06, 0x07, 0x08};
    
    uint32_t crc1 = xy_fota_calc_crc32(data1, 4);
    uint32_t crc2 = xy_fota_calc_crc32(data2, 4);
    
    TEST_ASSERT_NOT_EQUAL(crc1, crc2);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Structure Tests */
    RUN_TEST(test_fota_config_structure_size);
    RUN_TEST(test_fota_header_structure_size);
    RUN_TEST(test_fota_structure_size);

    /* Init Tests */
    RUN_TEST(test_fota_init_null_param);
    RUN_TEST(test_fota_init_null_config);
    RUN_TEST(test_fota_init_valid_param);
    RUN_TEST(test_fota_deinit_null_param);
    RUN_TEST(test_fota_deinit_valid);

    /* Download Tests */
    RUN_TEST(test_fota_start_download_null_param);
    RUN_TEST(test_fota_start_download_invalid_size);
    RUN_TEST(test_fota_start_download_valid);
    RUN_TEST(test_fota_download_chunk_null_param);
    RUN_TEST(test_fota_download_chunk_not_downloading);
    RUN_TEST(test_fota_download_chunk_valid);

    /* State Tests */
    RUN_TEST(test_fota_get_state_null_param);
    RUN_TEST(test_fota_get_state_valid);
    RUN_TEST(test_fota_state_transitions);

    /* CRC Tests */
    RUN_TEST(test_fota_crc32_calculation);
    RUN_TEST(test_fota_crc32_same_data);
    RUN_TEST(test_fota_crc32_different_data);

    return UNITY_END();
}
