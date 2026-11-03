#include "unity.h"
#include "xy_cmm905.h"

#include <string.h>

typedef struct {
    uint8_t reg;
    uint8_t data[6];
    size_t len;
    xy_error_t ret;
    uint8_t write;
} op_t;

static op_t g_ops[12];
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
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
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
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    TEST_ASSERT_TRUE(op->write);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    TEST_ASSERT_EQUAL_UINT8(op->data[0], data[0]);
    return op->ret;
}

uint32_t xy_hal_sys_get_tick_count(void)
{
    return g_tick;
}

void setUp(void)
{
    memset(g_ops, 0, sizeof(g_ops));
    g_count = 0U;
    g_index = 0U;
    g_tick = 500U;
    g_init_ret = XY_DEVICE_OK;
    g_init_establish_transport = 1;
}

void tearDown(void) {}

static void queue_valid_init(void)
{
    /* the MM905 needs no register traffic to start; init only establishes the transport */
    (void)0;
}

static void test_cmm905_identity_init_read_and_deinit(void)
{
    xy_cmm905_t dev;
    xy_cmm905_sample_t sample;
    const uint8_t raw[6] = {0x01U, 0x00U, 0xFEU, 0xFFU, 0x10U, 0x00U};
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_cmm905_init(&dev, &bus));
    TEST_ASSERT_EQUAL_UINT16(XY_CMM905_ADDR, dev.i2c_dev.dev_addr);
    TEST_ASSERT_TRUE(dev.initialized);

    for (size_t i = 0; i < 6U; i++) {
        queue_read((uint8_t)(XY_CMM905_REG_DATA + i), &raw[i], 1U, XY_DEVICE_OK);
    }
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_cmm905_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT16(1, sample.raw_x);
    TEST_ASSERT_EQUAL_INT16(-2, sample.raw_y);
    TEST_ASSERT_EQUAL_INT16(16, sample.raw_z);
    TEST_ASSERT_EQUAL_UINT32(500U, sample.timestamp);

    queue_write(XY_CMM905_REG_STATUS, 0x00U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_cmm905_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(g_count, g_index);
}

static void test_cmm905_read_uses_six_single_byte_registers(void)
{
    xy_cmm905_t dev;
    xy_cmm905_sample_t sample;
    const uint8_t raw[6] = {0x34U, 0x12U, 0xCDU, 0xABU, 0x00U, 0x80U};
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_cmm905_init(&dev, &bus));
    for (size_t i = 0; i < 6U; i++) {
        queue_read((uint8_t)(XY_CMM905_REG_DATA + i), &raw[i], 1U, XY_DEVICE_OK);
    }
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_cmm905_read(&dev, &sample));
    /* little-endian pairing: 0x1234, 0xABCD, 0x8000 */
    TEST_ASSERT_EQUAL_INT16(0x1234, sample.raw_x);
    TEST_ASSERT_EQUAL_INT16((int16_t)0xABCD, sample.raw_y);
    TEST_ASSERT_EQUAL_INT16((int16_t)0x8000, sample.raw_z);
    TEST_ASSERT_EQUAL_UINT(6U, g_index);
}

static void test_cmm905_read_failure_stops_and_preserves_output(void)
{
    xy_cmm905_t dev;
    xy_cmm905_sample_t sample = {.raw_x = 11, .raw_y = 22, .raw_z = 33, .timestamp = 44U};
    const xy_cmm905_sample_t snapshot = sample;
    const uint8_t raw0 = 0x7FU;
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_cmm905_init(&dev, &bus));
    dev.sample = snapshot;

    queue_read(XY_CMM905_REG_DATA, &raw0, 1U, XY_DEVICE_OK);
    queue_read((uint8_t)(XY_CMM905_REG_DATA + 1U), NULL, 1U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_cmm905_read(&dev, &sample));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &sample, sizeof(sample));
    TEST_ASSERT_EQUAL_INT16(11, dev.sample.raw_x);
    TEST_ASSERT_EQUAL_UINT(2U, g_index);
}

static void test_cmm905_init_rejects_incomplete_nested_transport(void)
{
    xy_cmm905_t dev;
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    g_init_establish_transport = 0;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_cmm905_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, g_index);
}

static void test_cmm905_failed_reinit_preserves_live_owner(void)
{
    xy_cmm905_t dev;
    xy_cmm905_t snapshot;
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_cmm905_init(&dev, &bus));
    dev.sample = (xy_cmm905_sample_t){11, 22, 33, 44U};
    snapshot = dev;

    setUp();
    g_init_ret = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_cmm905_init(&dev, &bus));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev, sizeof(dev));
}

static void test_cmm905_deinit_propagates_transport_error(void)
{
    xy_cmm905_t dev;
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_cmm905_init(&dev, &bus));
    queue_write(XY_CMM905_REG_STATUS, 0x00U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_cmm905_deinit(&dev));
    /* a failed deinit keeps the owner live */
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT(1U, g_index);
}

static void test_cmm905_missing_handle_fails_closed(void)
{
    xy_cmm905_t dev;
    xy_cmm905_sample_t output = {.raw_x = 11, .raw_y = 22, .raw_z = 33, .timestamp = 44U};
    const xy_cmm905_sample_t snapshot = output;
    unsigned before;
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_cmm905_init(&dev, &bus));
    dev.sample = snapshot;
    before = g_index;
    dev.i2c_dev.i2c_handle = NULL;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_cmm905_read(&dev, &output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_cmm905_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT(before, g_index);
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev.sample, sizeof(dev.sample));
    TEST_ASSERT_TRUE(dev.initialized);
}

static void test_cmm905_null_arguments_are_rejected(void)
{
    xy_cmm905_t dev;
    xy_cmm905_sample_t sample;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_cmm905_init(NULL, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_cmm905_init(&dev, NULL));

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_cmm905_init(&dev, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_cmm905_read(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_cmm905_read(NULL, &sample));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_cmm905_identity_init_read_and_deinit);
    RUN_TEST(test_cmm905_read_uses_six_single_byte_registers);
    RUN_TEST(test_cmm905_read_failure_stops_and_preserves_output);
    RUN_TEST(test_cmm905_init_rejects_incomplete_nested_transport);
    RUN_TEST(test_cmm905_failed_reinit_preserves_live_owner);
    RUN_TEST(test_cmm905_deinit_propagates_transport_error);
    RUN_TEST(test_cmm905_missing_handle_fails_closed);
    RUN_TEST(test_cmm905_null_arguments_are_rejected);
    return UNITY_END();
}
