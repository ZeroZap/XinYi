#include "unity.h"
#include "xy_apds9960.h"

#include <string.h>

static xy_error_t g_init_error;
static xy_error_t g_read_error;
static xy_error_t g_write_error;
static uint32_t g_read_count;
static uint32_t g_write_count;
static uint8_t g_rgb[8] = {0x78, 0x56, 0x34, 0x12, 0xcd, 0xab, 0xef, 0xbe};

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout)
{
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1U;
    dev->i2c_handle = handle;
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
    if (reg == XY_APDS9960_REG_ID) {
        *buffer = 0xabU;
    } else if (reg == XY_APDS9960_REG_CDATAL) {
        memcpy(buffer, g_rgb, length);
    } else if (reg == XY_APDS9960_REG_PDATA) {
        *buffer = 42U;
    } else if (reg == XY_APDS9960_REG_GSTATUS) {
        *buffer = 0U;
    }
    return XY_DEVICE_OK;
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
    g_read_count = 0U;
    g_write_count = 0U;
}

void tearDown(void)
{
}

static void test_apds9960_init_read_and_deinit(void)
{
    xy_apds9960_t dev;
    xy_apds9960_rgb_t rgb;
    xy_apds9960_proximity_t proximity;
    xy_apds9960_gesture_fifo_t gesture;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_read_rgb(&dev, &rgb));
    TEST_ASSERT_EQUAL_UINT16(0x5678U, rgb.clear);
    TEST_ASSERT_EQUAL_UINT16(0x1234U, rgb.red);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_read_proximity(&dev, &proximity));
    TEST_ASSERT_EQUAL_UINT8(42U, proximity.proximity);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_read_gesture_fifo(&dev, &gesture));
    TEST_ASSERT_EQUAL_UINT32(77U, gesture.timestamp);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
}

static void test_apds9960_init_failure_clears_partial_transport(void)
{
    xy_apds9960_t dev;
    int bus;

    memset(&dev, 0xa5, sizeof(dev));
    g_init_error = XY_DEVICE_TIMEOUT;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_apds9960_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT32(0U, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(0U, g_write_count);
}

static void test_apds9960_rejects_lost_nested_transport_without_io(void)
{
    xy_apds9960_t dev;
    xy_apds9960_rgb_t rgb = {1U, 2U, 3U, 4U, 5U};
    xy_apds9960_proximity_t proximity = {6U, 7U};
    xy_apds9960_gesture_fifo_t gesture;
    uint32_t reads_before;
    uint32_t writes_before;
    int bus;

    memset(&gesture, 0x5a, sizeof(gesture));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));
    dev.rgb = rgb;
    dev.proximity = proximity;
    dev.gesture = gesture;
    reads_before = g_read_count;
    writes_before = g_write_count;

    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_read_rgb(&dev, &rgb));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_apds9960_read_proximity(&dev, &proximity));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_apds9960_read_gesture_fifo(&dev, &gesture));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT32(reads_before, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(writes_before, g_write_count);
    TEST_ASSERT_EQUAL_UINT16(1U, rgb.clear);
    TEST_ASSERT_EQUAL_UINT8(6U, proximity.proximity);
    TEST_ASSERT_EQUAL_UINT8(0x5aU, gesture.level);

    dev.i2c_dev.base.initialized = 1U;
    dev.i2c_dev.i2c_handle = NULL;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_read_rgb(&dev, &rgb));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT32(reads_before, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(writes_before, g_write_count);
}

static void test_apds9960_transport_failures_preserve_state(void)
{
    xy_apds9960_t dev;
    xy_apds9960_rgb_t rgb = {1U, 2U, 3U, 4U, 5U};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));
    dev.rgb = rgb;
    g_read_error = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_apds9960_read_rgb(&dev, &rgb));
    TEST_ASSERT_EQUAL_UINT16(1U, rgb.clear);
    TEST_ASSERT_EQUAL_UINT16(1U, dev.rgb.clear);

    g_read_error = XY_DEVICE_OK;
    g_write_error = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_apds9960_deinit(&dev));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_TRUE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NOT_NULL(dev.i2c_dev.i2c_handle);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_apds9960_init_read_and_deinit);
    RUN_TEST(test_apds9960_init_failure_clears_partial_transport);
    RUN_TEST(test_apds9960_rejects_lost_nested_transport_without_io);
    RUN_TEST(test_apds9960_transport_failures_preserve_state);
    return UNITY_END();
}
