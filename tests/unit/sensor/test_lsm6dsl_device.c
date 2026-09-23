#include "unity.h"
#include "xy_lsm6dsl.h"

#include <string.h>

static xy_error_t error;
static unsigned operation_count;
static uint8_t accel[6] = {1, 0, 2, 0, 3, 0};
static uint8_t gyro[6] = {4, 0, 5, 0, 6, 0};

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout)
{
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1U;
    dev->i2c_handle = handle;
    dev->dev_addr = address;
    dev->timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data,
                                  size_t length)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    operation_count++;
    if (error != XY_DEVICE_OK) {
        return error;
    }
    if (reg == XY_LSM6DSL_REG_WHOAMI) {
        *data = XY_LSM6DSL_WHOAMI;
    } else {
        memcpy(data, reg == XY_LSM6DSL_REG_OUTX_L_XL ? accel : gyro, length);
    }
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data,
                                   size_t length)
{
    (void)reg;
    (void)data;
    (void)length;
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    operation_count++;
    return error;
}

void xy_hal_delay_ms(uint32_t delay_ms)
{
    (void)delay_ms;
}

uint32_t xy_hal_sys_get_tick_count(void)
{
    return 77U;
}

void setUp(void)
{
    error = XY_DEVICE_OK;
    operation_count = 0U;
}

void tearDown(void)
{
}

static void test_lsm6dsl_init_read_and_deinit(void)
{
    xy_lsm6dsl_t dev;
    xy_lsm6dsl_sample_t sample;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lsm6dsl_init(&dev, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lsm6dsl_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT16(1, sample.accel_x);
    TEST_ASSERT_EQUAL_INT16(4, sample.gyro_x);
    TEST_ASSERT_EQUAL_UINT32(77U, sample.timestamp);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lsm6dsl_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
}

static void test_lsm6dsl_fail_closed_on_invalid_nested_transport(void)
{
    xy_lsm6dsl_t dev;
    xy_lsm6dsl_sample_t output = {11, 22, 33, 44, 55, 66, 77};
    xy_lsm6dsl_sample_t snapshot = output;
    unsigned before;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lsm6dsl_init(&dev, &bus));
    dev.sample = snapshot;
    before = operation_count;
    dev.i2c_dev.i2c_handle = NULL;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_lsm6dsl_read(&dev, &output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_lsm6dsl_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT(before, operation_count);
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev.sample, sizeof(dev.sample));
    TEST_ASSERT_TRUE(dev.initialized);
}

static void test_lsm6dsl_transport_failure_preserves_state(void)
{
    xy_lsm6dsl_t dev;
    xy_lsm6dsl_sample_t output = {11, 22, 33, 44, 55, 66, 77};
    xy_lsm6dsl_sample_t snapshot = output;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lsm6dsl_init(&dev, &bus));
    dev.sample = snapshot;
    error = XY_DEVICE_TIMEOUT;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_lsm6dsl_read(&dev, &output));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev.sample, sizeof(dev.sample));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lsm6dsl_init_read_and_deinit);
    RUN_TEST(test_lsm6dsl_fail_closed_on_invalid_nested_transport);
    RUN_TEST(test_lsm6dsl_transport_failure_preserves_state);
    return UNITY_END();
}
