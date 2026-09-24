#include "unity.h"
#include "xy_vl53l0x.h"

#include <string.h>

static uint8_t g_model;
static uint8_t g_range[12];
static xy_error_t g_init_ret;
static int g_init_establish_transport;
static xy_error_t g_read_ret;
static xy_error_t g_write_ret;
static unsigned g_reads;
static unsigned g_writes;

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
    TEST_ASSERT_TRUE(reg == XY_VL53L0X_REG_MODEL_ID || reg == XY_VL53L0X_REG_RANGE_STATUS);
    g_reads++;
    if (g_read_ret != XY_DEVICE_OK) {
        return g_read_ret;
    }
    if (reg == XY_VL53L0X_REG_MODEL_ID) {
        TEST_ASSERT_EQUAL_UINT(1U, len);
        data[0] = g_model;
    } else {
        TEST_ASSERT_EQUAL_UINT(sizeof(g_range), len);
        memcpy(data, g_range, len);
    }
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data, size_t len)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    TEST_ASSERT_EQUAL_UINT8(XY_VL53L0X_REG_SYSRANGE_START, reg);
    TEST_ASSERT_EQUAL_UINT(1U, len);
    TEST_ASSERT_EQUAL_UINT8(1U, data[0]);
    g_writes++;
    return g_write_ret;
}

void xy_hal_delay_ms(uint32_t ms) { TEST_ASSERT_EQUAL_UINT32(50U, ms); }
uint32_t xy_hal_sys_get_tick_count(void) { return 98765U; }

void setUp(void)
{
    memset(g_range, 0, sizeof(g_range));
    g_model = XY_VL53L0X_MODEL_ID;
    g_init_ret = XY_DEVICE_OK;
    g_init_establish_transport = 1;
    g_read_ret = XY_DEVICE_OK;
    g_write_ret = XY_DEVICE_OK;
    g_reads = 0U;
    g_writes = 0U;
}
void tearDown(void) {}

static void test_vl53l0x_identity_read_and_lifecycle(void)
{
    xy_vl53l0x_t dev;
    xy_vl53l0x_sample_t sample = {.distance_mm = 7U, .timestamp = 8U};
    int bus;

    g_range[10] = 0x01U;
    g_range[11] = 0xF4U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_vl53l0x_init(&dev, &bus));
    TEST_ASSERT_EQUAL_UINT16(XY_VL53L0X_ADDR, dev.i2c_dev.dev_addr);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_vl53l0x_read(&dev, &sample));
    TEST_ASSERT_EQUAL_UINT16(500U, sample.distance_mm);
    TEST_ASSERT_EQUAL_UINT32(98765U, sample.timestamp);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_vl53l0x_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(2U, g_reads);
    TEST_ASSERT_EQUAL_UINT(1U, g_writes);
}

static void test_vl53l0x_failures_preserve_state_and_reject_invalid_context(void)
{
    xy_vl53l0x_t dev;
    xy_vl53l0x_sample_t sample = {.distance_mm = 0xAAAAU, .timestamp = 42U};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_vl53l0x_init(&dev, &bus));
    g_write_ret = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_vl53l0x_read(&dev, &sample));
    TEST_ASSERT_EQUAL_UINT16(0xAAAAU, sample.distance_mm);
    TEST_ASSERT_EQUAL_UINT32(42U, sample.timestamp);
    TEST_ASSERT_EQUAL_UINT(1U, g_reads);
    TEST_ASSERT_EQUAL_UINT(1U, g_writes);
    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_vl53l0x_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_vl53l0x_deinit(&dev));
}

static void test_vl53l0x_init_rejects_identity_and_transport_failures(void)
{
    xy_vl53l0x_t dev;
    int bus;

    g_model = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND, xy_vl53l0x_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    g_model = XY_VL53L0X_MODEL_ID;
    g_read_ret = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_vl53l0x_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
}

static void test_vl53l0x_init_rejects_incomplete_nested_transport(void)
{
    xy_vl53l0x_t dev;
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    g_init_establish_transport = 0;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_vl53l0x_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, g_reads);
    TEST_ASSERT_EQUAL_UINT(0U, g_writes);
}

static void test_vl53l0x_rejects_missing_nested_transport_without_side_effects(void)
{
    xy_vl53l0x_t dev;
    xy_vl53l0x_sample_t sample = {.distance_mm = 0x1357U, .timestamp = 0x2468U};
    xy_vl53l0x_sample_t snapshot = sample;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_vl53l0x_init(&dev, &bus));
    dev.sample = snapshot;
    dev.i2c_dev.i2c_handle = NULL;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_vl53l0x_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_vl53l0x_deinit(&dev));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &sample, sizeof(sample));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev.sample, sizeof(dev.sample));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT(1U, g_reads);
    TEST_ASSERT_EQUAL_UINT(0U, g_writes);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_vl53l0x_identity_read_and_lifecycle);
    RUN_TEST(test_vl53l0x_failures_preserve_state_and_reject_invalid_context);
    RUN_TEST(test_vl53l0x_init_rejects_identity_and_transport_failures);
    RUN_TEST(test_vl53l0x_init_rejects_incomplete_nested_transport);
    RUN_TEST(test_vl53l0x_rejects_missing_nested_transport_without_side_effects);
    return UNITY_END();
}
