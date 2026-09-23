#include "unity.h"
#include "xy_lis2dw12.h"

#include <string.h>

static xy_error_t init_error;
static xy_error_t io_error;
static unsigned operation_count;
static unsigned delay_count;
static uint8_t raw[6] = {0x00, 0x10, 0x00, 0xF0, 0x00, 0x08};

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout)
{
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1U;
    dev->i2c_handle = handle;
    dev->dev_addr = address;
    dev->timeout = timeout;
    return init_error;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data,
                                  size_t length)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    operation_count++;
    if (io_error != XY_DEVICE_OK) {
        return io_error;
    }
    if (reg == XY_LIS2DW12_REG_WHO_AM_I) {
        *data = XY_LIS2DW12_WHO_AM_I;
    } else {
        memcpy(data, raw, length);
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
    return io_error;
}

void xy_hal_delay_ms(uint32_t milliseconds)
{
    TEST_ASSERT_EQUAL_UINT32(10U, milliseconds);
    delay_count++;
}

uint32_t xy_hal_sys_get_tick_count(void)
{
    return 77U;
}

void setUp(void)
{
    init_error = XY_DEVICE_OK;
    io_error = XY_DEVICE_OK;
    operation_count = 0U;
    delay_count = 0U;
}

void tearDown(void)
{
}

static void test_lis2dw12_init_read_and_deinit(void)
{
    xy_lis2dw12_t dev;
    xy_lis2dw12_sample_t sample;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lis2dw12_init(&dev, &bus));
    TEST_ASSERT_EQUAL_UINT16(XY_LIS2DW12_ADDR, dev.i2c_dev.dev_addr);
    TEST_ASSERT_EQUAL_UINT(1U, delay_count);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lis2dw12_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT16(256, sample.raw_x);
    TEST_ASSERT_EQUAL_INT16(-256, sample.raw_y);
    TEST_ASSERT_EQUAL_INT32(499, sample.x_mg);
    TEST_ASSERT_EQUAL_INT32(-499, sample.y_mg);
    TEST_ASSERT_EQUAL_UINT32(77U, sample.timestamp);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lis2dw12_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
}

static void test_lis2dw12_init_failure_clears_partial_transport(void)
{
    xy_lis2dw12_t dev;
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    init_error = XY_DEVICE_ERROR;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_lis2dw12_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, operation_count);
    TEST_ASSERT_EQUAL_UINT(0U, delay_count);
}

static void test_lis2dw12_fail_closed_on_invalid_nested_transport(void)
{
    xy_lis2dw12_t dev;
    xy_lis2dw12_sample_t output = {1, 2, 3, 4, 5, 6, 7};
    xy_lis2dw12_sample_t snapshot = output;
    unsigned before;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lis2dw12_init(&dev, &bus));
    dev.sample = snapshot;
    before = operation_count;
    dev.i2c_dev.i2c_handle = NULL;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_lis2dw12_read(&dev, &output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_lis2dw12_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT(before, operation_count);
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev.sample, sizeof(dev.sample));
    TEST_ASSERT_TRUE(dev.initialized);
}

static void test_lis2dw12_transport_failures_preserve_state(void)
{
    xy_lis2dw12_t dev;
    xy_lis2dw12_sample_t output = {1, 2, 3, 4, 5, 6, 7};
    xy_lis2dw12_sample_t snapshot = output;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_lis2dw12_init(&dev, &bus));
    dev.sample = snapshot;
    io_error = XY_DEVICE_TIMEOUT;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_lis2dw12_read(&dev, &output));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev.sample, sizeof(dev.sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_lis2dw12_deinit(&dev));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_TRUE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NOT_NULL(dev.i2c_dev.i2c_handle);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lis2dw12_init_read_and_deinit);
    RUN_TEST(test_lis2dw12_init_failure_clears_partial_transport);
    RUN_TEST(test_lis2dw12_fail_closed_on_invalid_nested_transport);
    RUN_TEST(test_lis2dw12_transport_failures_preserve_state);
    return UNITY_END();
}
