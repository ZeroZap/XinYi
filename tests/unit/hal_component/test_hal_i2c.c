/**
 * @file test_hal_i2c.c
 * @brief HAL I2C Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-03-01
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* HAL I2C header */
#include "xy_hal_i2c.h"

/* ==================== Test Fixtures ==================== */

static xy_hal_i2c_config_t test_config;

void setUp(void)
{
    test_config.speed_mode = XY_HAL_I2C_SPEED_STANDARD;
    test_config.speed = 100000;
    test_config.duty_cycle = XY_HAL_I2C_DUTY_2;
}

void tearDown(void)
{
    /* Cleanup */
}

/* ==================== I2C Config Tests ==================== */

void test_i2c_config_structure_size(void)
{
    TEST_ASSERT_TRUE(sizeof(xy_hal_i2c_config_t) >= 8);
}

void test_i2c_speed_mode_values(void)
{
    TEST_ASSERT_EQUAL(0, XY_HAL_I2C_SPEED_STANDARD);
    TEST_ASSERT_EQUAL(1, XY_HAL_I2C_SPEED_FAST);
}

void test_i2c_duty_cycle_values(void)
{
    TEST_ASSERT_EQUAL(0, XY_HAL_I2C_DUTY_2);
    TEST_ASSERT_EQUAL(1, XY_HAL_I2C_DUTY_16_9);
}

/* ==================== I2C Handle Tests ==================== */

void test_i2c_handle_structure_size(void)
{
    TEST_ASSERT_TRUE(sizeof(xy_hal_i2c_handle_t) >= (sizeof(void*) + 1));
}

void test_i2c_handle_initialization(void)
{
    xy_hal_i2c_handle_t handle;
    memset(&handle, 0, sizeof(handle));

    TEST_ASSERT_EQUAL_PTR(NULL, handle.instance);
    TEST_ASSERT_EQUAL(0, handle.initialized);
}

/* ==================== I2C Function Tests ==================== */

void test_i2c_init_null_param(void)
{
    xy_hal_error_t ret = xy_hal_i2c_init(NULL, &test_config);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

void test_i2c_deinit_null_param(void)
{
    xy_hal_error_t ret = xy_hal_i2c_deinit(NULL);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

void test_i2c_write_null_param(void)
{
    xy_hal_error_t ret = xy_hal_i2c_write(NULL, 0, NULL, 0);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

void test_i2c_read_null_param(void)
{
    xy_hal_error_t ret = xy_hal_i2c_read(NULL, 0, NULL, 0);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* I2C Config Tests */
    RUN_TEST(test_i2c_config_structure_size);
    RUN_TEST(test_i2c_speed_mode_values);
    RUN_TEST(test_i2c_duty_cycle_values);

    /* I2C Handle Tests */
    RUN_TEST(test_i2c_handle_structure_size);
    RUN_TEST(test_i2c_handle_initialization);

    /* I2C Function Tests */
    RUN_TEST(test_i2c_init_null_param);
    RUN_TEST(test_i2c_deinit_null_param);
    RUN_TEST(test_i2c_write_null_param);
    RUN_TEST(test_i2c_read_null_param);

    return UNITY_END();
}
