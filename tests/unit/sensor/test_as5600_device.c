#include "unity.h"
#include "xy_as5600.h"

#include <string.h>

static uint8_t g_bytes[2];
static xy_error_t g_read_ret;
static int g_init_establish_transport;
static unsigned g_reads;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t addr, uint32_t timeout)
{
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = g_init_establish_transport;
    dev->i2c_handle = g_init_establish_transport ? handle : NULL;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_EQUAL_UINT8(XY_AS5600_REG_ANGLE_H, reg);
    TEST_ASSERT_EQUAL_UINT(2U, len);
    g_reads++;
    if (g_read_ret == XY_DEVICE_OK) {
        memcpy(data, g_bytes, 2U);
    }
    return g_read_ret;
}

uint32_t xy_hal_sys_get_tick_count(void)
{
    return 222333U;
}

void setUp(void)
{
    g_bytes[0] = 0U;
    g_bytes[1] = 0U;
    g_read_ret = XY_DEVICE_OK;
    g_init_establish_transport = 1;
    g_reads = 0U;
}

void tearDown(void) {}

static void test_as5600_read_masks_reserved_bits_and_lifecycle(void)
{
    xy_as5600_t dev;
    xy_as5600_sample_t sample = {.angle_raw = 0xAAAAU, .timestamp = 7U};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_as5600_init(&dev, &bus));
    g_bytes[0] = 0xF8U;
    g_bytes[1] = 0x00U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_as5600_read(&dev, &sample));
    TEST_ASSERT_EQUAL_UINT16(0x0800U, sample.angle_raw);
    TEST_ASSERT_EQUAL_UINT32(222333U, sample.timestamp);
    TEST_ASSERT_EQUAL_UINT(1U, g_reads);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_as5600_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
}

static void test_as5600_failure_and_nested_lifecycle_are_atomic(void)
{
    xy_as5600_t dev;
    xy_as5600_sample_t sample = {.angle_raw = 0xAAAAU, .timestamp = 7U};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_as5600_init(&dev, &bus));
    dev.sample = sample;
    g_read_ret = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_as5600_read(&dev, &sample));
    TEST_ASSERT_EQUAL_UINT16(0xAAAAU, sample.angle_raw);
    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_as5600_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_as5600_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT(1U, g_reads);
}

static void test_as5600_init_rejects_incomplete_nested_transport(void)
{
    xy_as5600_t dev;
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    g_init_establish_transport = 0;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_as5600_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, g_reads);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_as5600_read_masks_reserved_bits_and_lifecycle);
    RUN_TEST(test_as5600_failure_and_nested_lifecycle_are_atomic);
    RUN_TEST(test_as5600_init_rejects_incomplete_nested_transport);
    return UNITY_END();
}
