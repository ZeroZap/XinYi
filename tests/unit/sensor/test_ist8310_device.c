#include "unity.h"
#include "xy_ist8310.h"

#include <string.h>

static unsigned g_io_count;
static uint8_t g_id;
static uint8_t g_raw[6];
static xy_error_t g_io_error;
static xy_error_t g_init_error;
static int g_init_establish_transport;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout)
{
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = g_init_establish_transport;
    dev->i2c_handle = g_init_establish_transport ? handle : NULL;
    dev->dev_addr = address;
    dev->timeout = timeout;
    return g_init_error;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    g_io_count++;
    if (g_io_error != XY_DEVICE_OK) {
        return g_io_error;
    }
    if (reg == XY_IST8310_REG_WHOAMI) {
        *data = g_id;
    } else {
        memcpy(data, g_raw, len);
    }
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data,
                                   size_t len)
{
    (void)reg;
    (void)data;
    (void)len;
    TEST_ASSERT_TRUE(dev->base.initialized);
    g_io_count++;
    return g_io_error;
}

uint32_t xy_hal_sys_get_tick_count(void)
{
    return 222333U;
}

void setUp(void)
{
    g_io_count = 0U;
    g_id = XY_IST8310_WHOAMI;
    g_raw[0] = 1U;
    g_raw[1] = 0U;
    g_raw[2] = 0xFEU;
    g_raw[3] = 0xFFU;
    g_raw[4] = 0x10U;
    g_raw[5] = 0U;
    g_io_error = XY_DEVICE_OK;
    g_init_error = XY_DEVICE_OK;
    g_init_establish_transport = 1;
}

void tearDown(void) {}

static void test_ist8310_init_read_and_deinit(void)
{
    xy_ist8310_t dev;
    xy_ist8310_sample_t sample;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ist8310_init(&dev, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ist8310_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT16(1, sample.raw_x);
    TEST_ASSERT_EQUAL_INT16(-2, sample.raw_y);
    TEST_ASSERT_EQUAL_UINT32(222333U, sample.timestamp);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ist8310_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
}

static void test_ist8310_read_failure_preserves_output(void)
{
    xy_ist8310_t dev;
    xy_ist8310_sample_t sample = {11, 22, 33, 44};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ist8310_init(&dev, &bus));
    dev.sample = sample;
    g_io_error = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_ist8310_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT16(11, sample.raw_x);
    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ist8310_read(&dev, &sample));
}

static void test_ist8310_init_rejects_incomplete_nested_transport(void)
{
    xy_ist8310_t dev;
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    g_init_establish_transport = 0;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ist8310_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, g_io_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ist8310_init_read_and_deinit);
    RUN_TEST(test_ist8310_read_failure_preserves_output);
    RUN_TEST(test_ist8310_init_rejects_incomplete_nested_transport);
    return UNITY_END();
}
