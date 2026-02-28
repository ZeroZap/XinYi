/**
 * @file test_device_mpu6050.c
 * @brief MPU6050 Device Unit Tests
 * @version 1.0.0
 * @date 2026-03-01
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* Device headers */
#include "xy_mpu6050.h"

/* ==================== Test Fixtures ==================== */

static xy_mpu6050_t mpu;

void setUp(void)
{
    memset(&mpu, 0, sizeof(mpu));
}

void tearDown(void)
{
    /* Cleanup */
}

/* ==================== MPU6050 Structure Tests ==================== */

void test_mpu6050_structure_size(void)
{
    TEST_ASSERT_TRUE(sizeof(xy_mpu6050_t) >= (sizeof(xy_i2c_device_t) + 12));
}

void test_mpu6050_initialization(void)
{
    memset(&mpu, 0, sizeof(mpu));

    TEST_ASSERT_EQUAL_PTR(NULL, mpu.i2c_dev.handle);
    TEST_ASSERT_EQUAL(0, mpu.accel_x);
    TEST_ASSERT_EQUAL(0, mpu.accel_y);
    TEST_ASSERT_EQUAL(0, mpu.accel_z);
    TEST_ASSERT_EQUAL(0, mpu.gyro_x);
    TEST_ASSERT_EQUAL(0, mpu.gyro_y);
    TEST_ASSERT_EQUAL(0, mpu.gyro_z);
}

/* ==================== MPU6050 Init Tests ==================== */

void test_mpu6050_init_null_param(void)
{
    int ret = xy_mpu6050_init(NULL, NULL);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

/* ==================== MPU6050 Read Tests ==================== */

void test_mpu6050_read_null_param(void)
{
    int ret = xy_mpu6050_read(NULL);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

void test_mpu6050_read_accel_null_param(void)
{
    int ret = xy_mpu6050_read_accel(NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

void test_mpu6050_read_gyro_null_param(void)
{
    int ret = xy_mpu6050_read_gyro(NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

/* ==================== MPU6050 Data Tests ==================== */

void test_mpu6050_data_range(void)
{
    /* Test data is within expected range */
    mpu.accel_x = 16384;  /* 1g at ±2g range */
    mpu.accel_y = 0;
    mpu.accel_z = 16384;

    TEST_ASSERT_INT_WITHIN(20000, 0, mpu.accel_x);
    TEST_ASSERT_INT_WITHIN(20000, 0, mpu.accel_y);
    TEST_ASSERT_INT_WITHIN(20000, 0, mpu.accel_z);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Structure Tests */
    RUN_TEST(test_mpu6050_structure_size);
    RUN_TEST(test_mpu6050_initialization);

    /* Init Tests */
    RUN_TEST(test_mpu6050_init_null_param);

    /* Read Tests */
    RUN_TEST(test_mpu6050_read_null_param);
    RUN_TEST(test_mpu6050_read_accel_null_param);
    RUN_TEST(test_mpu6050_read_gyro_null_param);

    /* Data Tests */
    RUN_TEST(test_mpu6050_data_range);

    return UNITY_END();
}
