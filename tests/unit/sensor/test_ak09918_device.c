#include "unity.h"
#include "xy_ak09918.h"

#include <string.h>

typedef struct {
    uint8_t reg;
    uint8_t data[6];
    size_t len;
    xy_error_t ret;
    uint8_t write;
} op_t;

static op_t g_ops[8];
static unsigned g_count;
static unsigned g_index;
static uint32_t g_tick;
static xy_error_t g_init_ret;
static int g_init_establish_transport;

static void queue_read(uint8_t reg, const uint8_t *data, size_t len, xy_error_t ret)
{
    op_t *op = &g_ops[g_count++];
    op->reg = reg;
    op->len = len;
    op->ret = ret;
    op->write = 0U;
    if (data != NULL) {
        memcpy(op->data, data, len);
    }
}

static void queue_write(uint8_t reg, uint8_t value, xy_error_t ret)
{
    op_t *op = &g_ops[g_count++];
    op->reg = reg;
    op->data[0] = value;
    op->len = 1U;
    op->ret = ret;
    op->write = 1U;
}

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
    op_t *op = &g_ops[g_index++];
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_FALSE(op->write);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    if (op->ret == XY_DEVICE_OK) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data, size_t len)
{
    op_t *op = &g_ops[g_index++];
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_TRUE(op->write);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    TEST_ASSERT_EQUAL_UINT8(op->data[0], data[0]);
    return op->ret;
}

void xy_hal_delay_ms(uint32_t ms) { g_tick += ms; }
uint32_t xy_hal_sys_get_tick_count(void) { return g_tick; }

void setUp(void)
{
    memset(g_ops, 0, sizeof(g_ops));
    g_count = 0U;
    g_index = 0U;
    g_tick = 100U;
    g_init_ret = XY_DEVICE_OK;
    g_init_establish_transport = 1;
}
void tearDown(void) {}

static void queue_valid_init(void)
{
    const uint8_t id[2] = {XY_AK09918_WIA1, XY_AK09918_WIA2};
    queue_read(XY_AK09918_REG_WIA1, id, sizeof(id), XY_DEVICE_OK);
    queue_write(XY_AK09918_REG_CNTL3, XY_AK09918_RESET, XY_DEVICE_OK);
    queue_write(XY_AK09918_REG_CNTL2, XY_AK09918_MODE_CONTINUOUS_100HZ, XY_DEVICE_OK);
}

static void test_ak09918_identity_init_read_and_deinit(void)
{
    xy_ak09918_t dev;
    xy_ak09918_sample_t sample;
    const uint8_t ready = 1U;
    const uint8_t raw[6] = {0xE8U, 0x03U, 0x18U, 0xFCU, 0xD0U, 0x07U};
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ak09918_init(&dev, &bus));
    TEST_ASSERT_EQUAL_UINT16(XY_AK09918_ADDR, dev.i2c_dev.dev_addr);
    TEST_ASSERT_EQUAL_UINT32(101U, g_tick);
    queue_read(XY_AK09918_REG_ST1, &ready, 1U, XY_DEVICE_OK);
    queue_read(XY_AK09918_REG_HXL, raw, sizeof(raw), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ak09918_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT16(1000, sample.raw_x);
    TEST_ASSERT_EQUAL_INT16(-1000, sample.raw_y);
    TEST_ASSERT_EQUAL_INT16(2000, sample.raw_z);
    TEST_ASSERT_EQUAL_UINT32(101U, sample.timestamp);
    queue_write(XY_AK09918_REG_CNTL2, XY_AK09918_MODE_POWER_DOWN, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ak09918_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT(g_count, g_index);
}

static void test_ak09918_failures_preserve_state_and_stop(void)
{
    xy_ak09918_t dev;
    xy_ak09918_sample_t sample = {.raw_x = 11, .raw_y = 22, .raw_z = 33, .timestamp = 44U};
    const uint8_t id[2] = {XY_AK09918_WIA1, XY_AK09918_WIA2};
    const uint8_t ready = 1U;
    int bus;

    queue_read(XY_AK09918_REG_WIA1, id, sizeof(id), XY_DEVICE_OK);
    queue_write(XY_AK09918_REG_CNTL3, XY_AK09918_RESET, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_ak09918_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ak09918_init(&dev, &bus));
    dev.sample = sample;
    queue_read(XY_AK09918_REG_ST1, &ready, 1U, XY_DEVICE_OK);
    queue_read(XY_AK09918_REG_HXL, NULL, 6U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_ak09918_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT16(11, sample.raw_x);
    TEST_ASSERT_EQUAL_INT16(11, dev.sample.raw_x);
    queue_write(XY_AK09918_REG_CNTL2, XY_AK09918_MODE_POWER_DOWN, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_ak09918_deinit(&dev));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT(g_count, g_index);
}

static void test_ak09918_init_rejects_incomplete_nested_transport(void)
{
    xy_ak09918_t dev;
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    g_init_establish_transport = 0;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ak09918_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, g_index);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ak09918_identity_init_read_and_deinit);
    RUN_TEST(test_ak09918_failures_preserve_state_and_stop);
    RUN_TEST(test_ak09918_init_rejects_incomplete_nested_transport);
    return UNITY_END();
}
