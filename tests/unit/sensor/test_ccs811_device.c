#include "unity.h"
#include "xy_ccs811.h"

#include <string.h>

static xy_error_t g_init_error;
static xy_error_t g_read_error;
static xy_error_t g_write_error;
static uint8_t g_init_sets_transport;
static uint32_t g_read_count;
static uint32_t g_write_count;
static uint8_t g_result[8] = {0x01, 0x90, 0x00, 0x2a, 0, 0, 0, 0};

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout)
{
    memset(dev, 0, sizeof(*dev));
    if (g_init_sets_transport) {
        dev->base.initialized = 1U;
        dev->i2c_handle = handle;
    }
    dev->dev_addr = address;
    dev->timeout = timeout;
    return g_init_error;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *buffer,
                                  size_t length)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    g_read_count++;
    if (g_read_error != XY_DEVICE_OK) {
        return g_read_error;
    }
    if (reg == XY_CCS811_REG_HW_ID) {
        *buffer = XY_CCS811_HW_ID_VALUE;
    } else if (reg == XY_CCS811_REG_STATUS) {
        *buffer = XY_CCS811_STATUS_DATA_READY;
    } else {
        memcpy(buffer, g_result, length);
    }
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *buffer, size_t length)
{
    (void)buffer;
    (void)length;
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    g_write_count++;
    return g_write_error;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *buffer,
                                   size_t length)
{
    (void)reg;
    (void)buffer;
    (void)length;
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    g_write_count++;
    return g_write_error;
}

uint32_t xy_hal_sys_get_tick_count(void)
{
    return 77U;
}

void setUp(void)
{
    g_init_error = XY_DEVICE_OK;
    g_read_error = XY_DEVICE_OK;
    g_write_error = XY_DEVICE_OK;
    g_init_sets_transport = 1U;
    g_read_count = 0U;
    g_write_count = 0U;
}

void tearDown(void)
{
}

static void test_ccs811_init_read_and_deinit(void)
{
    xy_ccs811_t dev;
    xy_ccs811_sample_t sample;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ccs811_init(&dev, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ccs811_read(&dev, &sample));
    TEST_ASSERT_EQUAL_UINT16(400U, sample.eco2_ppm);
    TEST_ASSERT_EQUAL_UINT16(42U, sample.tvoc_ppb);
    TEST_ASSERT_EQUAL_UINT32(77U, sample.timestamp);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ccs811_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
}

static void test_ccs811_init_failure_clears_partial_transport(void)
{
    xy_ccs811_t dev;
    int bus;

    memset(&dev, 0xa5, sizeof(dev));
    g_init_error = XY_DEVICE_TIMEOUT;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_ccs811_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT32(0U, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(0U, g_write_count);
}

static void test_ccs811_rejects_incomplete_successful_helper_init(void)
{
    xy_ccs811_t dev;
    int bus;

    memset(&dev, 0xa5, sizeof(dev));
    g_init_sets_transport = 0U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ccs811_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT32(0U, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(0U, g_write_count);
}

static void test_ccs811_rejects_lost_nested_transport_without_io(void)
{
    xy_ccs811_t dev;
    xy_ccs811_sample_t sample = {1U, 2U, 3U};
    uint32_t reads_before;
    uint32_t writes_before;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ccs811_init(&dev, &bus));
    dev.sample = sample;
    reads_before = g_read_count;
    writes_before = g_write_count;

    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ccs811_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ccs811_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT32(reads_before, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(writes_before, g_write_count);
    TEST_ASSERT_EQUAL_UINT16(1U, sample.eco2_ppm);
    TEST_ASSERT_EQUAL_UINT16(1U, dev.sample.eco2_ppm);

    dev.i2c_dev.base.initialized = 1U;
    dev.i2c_dev.i2c_handle = NULL;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ccs811_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ccs811_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT32(reads_before, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(writes_before, g_write_count);
}

static void test_ccs811_transport_failures_preserve_state(void)
{
    xy_ccs811_t dev;
    xy_ccs811_sample_t sample = {1U, 2U, 3U};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ccs811_init(&dev, &bus));
    dev.sample = sample;
    g_read_error = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_ccs811_read(&dev, &sample));
    TEST_ASSERT_EQUAL_UINT16(1U, sample.eco2_ppm);
    TEST_ASSERT_EQUAL_UINT16(1U, dev.sample.eco2_ppm);

    g_read_error = XY_DEVICE_OK;
    g_write_error = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_ccs811_deinit(&dev));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_TRUE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NOT_NULL(dev.i2c_dev.i2c_handle);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ccs811_init_read_and_deinit);
    RUN_TEST(test_ccs811_init_failure_clears_partial_transport);
    RUN_TEST(test_ccs811_rejects_incomplete_successful_helper_init);
    RUN_TEST(test_ccs811_rejects_lost_nested_transport_without_io);
    RUN_TEST(test_ccs811_transport_failures_preserve_state);
    return UNITY_END();
}
