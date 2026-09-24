#include "unity.h"
#include "xy_vcnl4040.h"

#include <string.h>

static uint8_t g_bytes[2];
static xy_error_t g_read_ret;
static unsigned g_reads;
static xy_error_t g_init_ret;
static int g_init_establish_transport;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t addr, uint32_t timeout)
{
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = g_init_establish_transport;
    dev->i2c_handle = g_init_establish_transport ? handle : NULL;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    return g_init_ret;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    TEST_ASSERT_EQUAL_UINT8(XY_VCNL4040_REG_PS_DATA_L, reg);
    TEST_ASSERT_EQUAL_UINT(2U, len);
    g_reads++;
    if (g_read_ret == XY_DEVICE_OK) {
        memcpy(data, g_bytes, 2U);
    }
    return g_read_ret;
}

uint32_t xy_hal_sys_get_tick_count(void) { return 222333U; }

void setUp(void)
{
    g_bytes[0] = 0U;
    g_bytes[1] = 0U;
    g_read_ret = XY_DEVICE_OK;
    g_reads = 0U;
    g_init_ret = XY_DEVICE_OK;
    g_init_establish_transport = 1;
}
void tearDown(void) {}

static void test_vcnl4040_reads_little_endian_proximity_and_lifecycle(void)
{
    xy_vcnl4040_t dev;
    xy_vcnl4040_sample_t sample = {.proximity_raw = 1U, .timestamp = 2U};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_vcnl4040_init(&dev, &bus));
    TEST_ASSERT_EQUAL_UINT16(XY_VCNL4040_ADDR, dev.i2c_dev.dev_addr);
    g_bytes[0] = 0x34U;
    g_bytes[1] = 0x12U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_vcnl4040_read(&dev, &sample));
    TEST_ASSERT_EQUAL_UINT16(0x1234U, sample.proximity_raw);
    TEST_ASSERT_EQUAL_UINT32(222333U, sample.timestamp);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_vcnl4040_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
}

static void test_vcnl4040_init_rejects_incomplete_nested_transport(void)
{
    xy_vcnl4040_t dev;
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    g_init_establish_transport = 0;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_vcnl4040_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, g_reads);
}

static void test_vcnl4040_failure_and_nested_lifecycle_are_atomic(void)
{
    xy_vcnl4040_t dev;
    xy_vcnl4040_sample_t sample = {.proximity_raw = 0xAAAAU, .timestamp = 7U};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_vcnl4040_init(&dev, &bus));
    dev.sample = sample;
    g_read_ret = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_vcnl4040_read(&dev, &sample));
    TEST_ASSERT_EQUAL_UINT16(0xAAAAU, sample.proximity_raw);
    TEST_ASSERT_EQUAL_UINT16(0xAAAAU, dev.sample.proximity_raw);
    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_vcnl4040_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_vcnl4040_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT(1U, g_reads);
}

static void test_vcnl4040_missing_handle_fails_closed(void)
{
    xy_vcnl4040_t dev;
    xy_vcnl4040_sample_t sample = {.proximity_raw = 0xAAAAU, .timestamp = 7U};
    xy_vcnl4040_sample_t snapshot = sample;
    unsigned before;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_vcnl4040_init(&dev, &bus));
    dev.sample = snapshot;
    before = g_reads;
    dev.i2c_dev.i2c_handle = NULL;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_vcnl4040_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_vcnl4040_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT(before, g_reads);
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &sample, sizeof(sample));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev.sample, sizeof(dev.sample));
    TEST_ASSERT_TRUE(dev.initialized);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_vcnl4040_reads_little_endian_proximity_and_lifecycle);
    RUN_TEST(test_vcnl4040_init_rejects_incomplete_nested_transport);
    RUN_TEST(test_vcnl4040_failure_and_nested_lifecycle_are_atomic);
    RUN_TEST(test_vcnl4040_missing_handle_fails_closed);
    return UNITY_END();
}
