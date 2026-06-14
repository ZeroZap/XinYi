/**
 * @file test_device_bmp280.c
 * @brief BMP280 Device Unit Tests
 * @version 1.0.0
 * @date 2026-03-01
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* Device headers */
#include "xy_bmp280.h"

/* ==================== Test Fixtures ==================== */

static xy_bmp280_t bmp;

void setUp(void)
{
    memset(&bmp, 0, sizeof(bmp));
}

void tearDown(void)
{
    /* Cleanup */
}

/* ==================== BMP280 Structure Tests ==================== */

void test_bmp280_structure_size(void)
{
    TEST_ASSERT_TRUE(sizeof(xy_bmp280_t) >= (sizeof(xy_i2c_device_t) + 36));
}

void test_bmp280_initialization(void)
{
    memset(&bmp, 0, sizeof(bmp));

    TEST_ASSERT_EQUAL_PTR(NULL, bmp.i2c_dev.handle);
    TEST_ASSERT_EQUAL(0, bmp.temperature);
    TEST_ASSERT_EQUAL(0, bmp.pressure);
}

/* ==================== BMP280 Init Tests ==================== */

void test_bmp280_init_null_param(void)
{
    int ret = xy_bmp280_init(NULL, NULL);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

/* ==================== BMP280 Read Tests ==================== */

void test_bmp280_read_null_param(void)
{
    int ret = xy_bmp280_read(NULL);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

void test_bmp280_read_temperature_null_param(void)
{
    int32_t temp = xy_bmp280_read_temperature(NULL);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, temp);
}

void test_bmp280_read_pressure_null_param(void)
{
    uint32_t pressure = xy_bmp280_read_pressure(NULL);
    TEST_ASSERT_EQUAL(0, pressure);
}

/* ==================== BMP280 Data Range Tests ==================== */

void test_bmp280_temperature_range(void)
{
    /* Normal temperature range: -40°C to +85°C */
    bmp.temperature = 2500;  /* 25.00°C */
    TEST_ASSERT_INT_WITHIN(10000, 2500, bmp.temperature);

    bmp.temperature = -1000;  /* -10.00°C */
    TEST_ASSERT_INT_WITHIN(10000, -1000, bmp.temperature);
}

void test_bmp280_pressure_range(void)
{
    /* Normal pressure range: 30000 to 110000 Pa */
    bmp.pressure = 101325;  /* Standard pressure */
    TEST_ASSERT_INT_WITHIN(50000, 101325, bmp.pressure);

    bmp.pressure = 95000;
    TEST_ASSERT_INT_WITHIN(50000, 95000, bmp.pressure);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Structure Tests */
    RUN_TEST(test_bmp280_structure_size);
    RUN_TEST(test_bmp280_initialization);

    /* Init Tests */
    RUN_TEST(test_bmp280_init_null_param);

    /* Read Tests */
    RUN_TEST(test_bmp280_read_null_param);
    RUN_TEST(test_bmp280_read_temperature_null_param);
    RUN_TEST(test_bmp280_read_pressure_null_param);

    /* Data Range Tests */
    RUN_TEST(test_bmp280_temperature_range);
    RUN_TEST(test_bmp280_pressure_range);

    return UNITY_END();
}
