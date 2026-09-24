#include "unity.h"
#include "xy_lsm9ds1.h"
#include <string.h>

static xy_error_t g_error;
static unsigned g_init_calls;
static unsigned g_init_fail_call;
static unsigned g_operation_count;
static uint8_t g_accel[6] = {1, 0, 2, 0, 3, 0};
static uint8_t g_gyro[6] = {4, 0, 5, 0, 6, 0};
static uint8_t g_mag[6] = {7, 0, 8, 0, 9, 0};

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t addr, uint32_t timeout)
{
    g_init_calls++;
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1;
    dev->i2c_handle = handle;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    if (g_init_calls == g_init_fail_call) {
        return XY_DEVICE_TIMEOUT;
    }
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    g_operation_count++;
    if (g_error != XY_DEVICE_OK) {
        return g_error;
    }
    if (reg == XY_LSM9DS1_REG_WHOAMI_IMU) {
        *data = dev->dev_addr == XY_LSM9DS1_IMU_ADDR ? XY_LSM9DS1_IMU_WHOAMI : XY_LSM9DS1_MAG_WHOAMI;
    } else if (dev->dev_addr == XY_LSM9DS1_MAG_ADDR) {
        memcpy(data, g_mag, len);
    } else if (reg == XY_LSM9DS1_REG_OUTX_L_XL) {
        memcpy(data, g_accel, len);
    } else {
        memcpy(data, g_gyro, len);
    }
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data, size_t len)
{
    (void)reg;
    (void)data;
    (void)len;
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    g_operation_count++;
    return g_error;
}

void xy_hal_delay_ms(uint32_t delay_ms)
{
    (void)delay_ms;
}

uint32_t xy_hal_sys_get_tick_count(void)
{
    return 77;
}

void setUp(void)
{
    g_error = XY_DEVICE_OK;
    g_init_calls = 0U;
    g_init_fail_call = 0U;
    g_operation_count = 0U;
}

void tearDown(void) {}

static void test_init_read_deinit(void)
{
    xy_lsm9ds1_t dev;
    xy_lsm9ds1_sample_t sample;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lsm9ds1_init(&dev, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lsm9ds1_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT16(1, sample.accel_x);
    TEST_ASSERT_EQUAL_INT16(4, sample.gyro_x);
    TEST_ASSERT_EQUAL_INT16(7, sample.mag_x);
    TEST_ASSERT_EQUAL_UINT32(77, sample.timestamp);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lsm9ds1_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.imu.base.initialized);
    TEST_ASSERT_NULL(dev.imu.i2c_handle);
    TEST_ASSERT_FALSE(dev.mag.base.initialized);
    TEST_ASSERT_NULL(dev.mag.i2c_handle);
}

static void test_missing_nested_transport_fails_closed(void)
{
    xy_lsm9ds1_t dev;
    xy_lsm9ds1_sample_t output = {11, 22, 33, 44, 55, 66, 77, 88, 99, 100};
    xy_lsm9ds1_sample_t snapshot = output;
    unsigned before;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lsm9ds1_init(&dev, &bus));
    dev.sample = snapshot;
    before = g_operation_count;
    dev.mag.i2c_handle = NULL;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_lsm9ds1_read(&dev, &output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_lsm9ds1_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT(before, g_operation_count);
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev.sample, sizeof(dev.sample));
    TEST_ASSERT_TRUE(dev.initialized);
}

static void test_read_failure_preserves_output_and_rejects_unready_device(void)
{
    xy_lsm9ds1_t dev;
    xy_lsm9ds1_sample_t sample = {11, 22, 33, 44, 55, 66, 77, 88, 99, 100};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lsm9ds1_init(&dev, &bus));
    g_error = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_lsm9ds1_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT16(11, sample.accel_x);
    dev.mag.base.initialized = 0;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_lsm9ds1_read(&dev, &sample));
}

static void test_invalid_arguments_are_rejected(void)
{
    xy_lsm9ds1_t dev;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_lsm9ds1_init(NULL, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_lsm9ds1_init(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_lsm9ds1_deinit(NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_lsm9ds1_read(NULL, NULL));
}

static void test_device_helper_init_failures_clear_both_transports(void)
{
    xy_lsm9ds1_t dev;
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    g_init_fail_call = 1U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_lsm9ds1_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.imu.base.initialized);
    TEST_ASSERT_NULL(dev.imu.i2c_handle);
    TEST_ASSERT_FALSE(dev.mag.base.initialized);
    TEST_ASSERT_NULL(dev.mag.i2c_handle);

    memset(&dev, 0xA5, sizeof(dev));
    g_init_fail_call = 2U;
    g_init_calls = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_lsm9ds1_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.imu.base.initialized);
    TEST_ASSERT_NULL(dev.imu.i2c_handle);
    TEST_ASSERT_FALSE(dev.mag.base.initialized);
    TEST_ASSERT_NULL(dev.mag.i2c_handle);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_read_deinit);
    RUN_TEST(test_read_failure_preserves_output_and_rejects_unready_device);
    RUN_TEST(test_invalid_arguments_are_rejected);
    RUN_TEST(test_device_helper_init_failures_clear_both_transports);
    RUN_TEST(test_missing_nested_transport_fails_closed);
    return UNITY_END();
}
