/**
 * @file test_dm_nvm.c
 * @brief DM NVM (Non-Volatile Memory) Unit Tests
 * @version 1.0.0
 * @date 2026-03-01
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* DM NVM header */
#include "xy_nvm.h"

/* ==================== Test Fixtures ==================== */

void setUp(void)
{
}

void tearDown(void)
{
}

/* ==================== NVM Structure Tests ==================== */

void test_nvm_config_structure_size(void)
{
    TEST_ASSERT_TRUE(sizeof(xy_nvm_config_t) >= 12);
}

void test_nvm_config_initialization(void)
{
    xy_nvm_config_t config;
    memset(&config, 0, sizeof(config));

    TEST_ASSERT_EQUAL(0, config.base_addr);
    TEST_ASSERT_EQUAL(0, config.size);
    TEST_ASSERT_EQUAL(0, config.sector_size);
}

/* ==================== NVM Function Tests ==================== */

void test_nvm_init_null_param(void)
{
    int ret = xy_nvm_init(NULL);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

void test_nvm_read_null_param(void)
{
    int ret = xy_nvm_read(NULL, 0, NULL, 0);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

void test_nvm_write_null_param(void)
{
    int ret = xy_nvm_write(NULL, 0, NULL, 0);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

void test_nvm_erase_null_param(void)
{
    int ret = xy_nvm_erase(NULL, 0, 0);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

/* ==================== NVM Operation Tests ==================== */

void test_nvm_valid_config(void)
{
    xy_nvm_config_t config;
    config.base_addr = 0x08010000;
    config.size = 0x10000;
    config.sector_size = 0x1000;

    TEST_ASSERT_EQUAL(0x08010000, config.base_addr);
    TEST_ASSERT_EQUAL(0x10000, config.size);
    TEST_ASSERT_EQUAL(0x1000, config.sector_size);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Structure Tests */
    RUN_TEST(test_nvm_config_structure_size);
    RUN_TEST(test_nvm_config_initialization);

    /* Function Tests */
    RUN_TEST(test_nvm_init_null_param);
    RUN_TEST(test_nvm_read_null_param);
    RUN_TEST(test_nvm_write_null_param);
    RUN_TEST(test_nvm_erase_null_param);

    /* Operation Tests */
    RUN_TEST(test_nvm_valid_config);

    return UNITY_END();
}
