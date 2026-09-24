#include "unity.h"
#include "xy_max44009.h"

#include <string.h>

static uint8_t g_bytes[2];
static xy_error_t g_read_ret[2];
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
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    TEST_ASSERT_EQUAL_UINT(1U, len);
    TEST_ASSERT_TRUE(reg == XY_MAX44009_REG_LUX_HIGH || reg == XY_MAX44009_REG_LUX_LOW);
    if (reg == XY_MAX44009_REG_LUX_HIGH) {
        g_reads = 0U;
    }
    if (g_read_ret[g_reads] != XY_DEVICE_OK) {
        g_reads++;
        return g_read_ret[g_reads - 1U];
    }
    *data = reg == XY_MAX44009_REG_LUX_HIGH ? g_bytes[0] : g_bytes[1];
    g_reads++;
    return XY_DEVICE_OK;
}

uint32_t xy_hal_sys_get_tick_count(void) { return 222333U; }

void setUp(void)
{
    g_bytes[0] = 0U;
    g_bytes[1] = 0U;
    g_read_ret[0] = XY_DEVICE_OK;
    g_read_ret[1] = XY_DEVICE_OK;
    g_init_establish_transport = 1;
    g_reads = 0U;
}
void tearDown(void) {}

static void test_max44009_read_decodes_lux_and_lifecycle(void)
{
    xy_max44009_t dev;
    xy_max44009_sample_t sample = {.illuminance_mlux = 1U, .timestamp = 2U};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_max44009_init(&dev, &bus, 0x49U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_max44009_init(&dev, &bus, XY_MAX44009_ADDR_LOW));
    g_bytes[0] = 0x23U;
    g_bytes[1] = 0x05U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_max44009_read(&dev, &sample));
    TEST_ASSERT_EQUAL_UINT32(9540U, sample.illuminance_mlux);
    TEST_ASSERT_EQUAL_UINT32(222333U, sample.timestamp);
    TEST_ASSERT_EQUAL_UINT(2U, g_reads);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_max44009_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
}

static void test_max44009_failure_and_nested_lifecycle_are_atomic(void)
{
    xy_max44009_t dev;
    xy_max44009_sample_t sample = {.illuminance_mlux = 0xAAAAU, .timestamp = 7U};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_max44009_init(&dev, &bus, XY_MAX44009_ADDR_HIGH));
    dev.sample = sample;
    g_read_ret[1] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_max44009_read(&dev, &sample));
    TEST_ASSERT_EQUAL_UINT32(0xAAAAU, sample.illuminance_mlux);
    TEST_ASSERT_EQUAL_UINT32(0xAAAAU, dev.sample.illuminance_mlux);
    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_max44009_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_max44009_deinit(&dev));
}

static void test_max44009_init_rejects_incomplete_nested_transport(void)
{
    xy_max44009_t dev;
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    g_init_establish_transport = 0;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_max44009_init(&dev, &bus, XY_MAX44009_ADDR_LOW));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, g_reads);
}

static void test_max44009_missing_handle_fails_closed(void)
{
    xy_max44009_t dev;
    xy_max44009_sample_t output = {.illuminance_mlux = 0xAAAAU, .timestamp = 7U};
    xy_max44009_sample_t snapshot = output;
    unsigned before;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_max44009_init(&dev, &bus, XY_MAX44009_ADDR_LOW));
    dev.sample = snapshot;
    before = g_reads;
    dev.i2c_dev.i2c_handle = NULL;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_max44009_read(&dev, &output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_max44009_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT(before, g_reads);
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev.sample, sizeof(dev.sample));
    TEST_ASSERT_TRUE(dev.initialized);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_max44009_read_decodes_lux_and_lifecycle);
    RUN_TEST(test_max44009_failure_and_nested_lifecycle_are_atomic);
    RUN_TEST(test_max44009_init_rejects_incomplete_nested_transport);
    RUN_TEST(test_max44009_missing_handle_fails_closed);
    return UNITY_END();
}
