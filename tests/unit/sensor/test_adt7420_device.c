#include "unity.h"
#include "xy_adt7420.h"

#include <string.h>

typedef struct {
    uint8_t reg;
    uint8_t data[2];
    size_t len;
    xy_error_t ret;
    uint8_t write;
} op_t;

static op_t g_ops[8];
static unsigned g_count;
static unsigned g_index;
static xy_error_t g_init_ret;
static int g_init_establish_transport;
static uint16_t g_init_addr;

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
    g_init_addr = addr;
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

void setUp(void)
{
    memset(g_ops, 0, sizeof(g_ops));
    g_count = 0U;
    g_index = 0U;
    g_init_ret = XY_DEVICE_OK;
    g_init_establish_transport = 1;
    g_init_addr = 0U;
}
void tearDown(void) {}

static void queue_valid_init(uint8_t address)
{
    const uint8_t id = XY_ADT7420_ID_VALUE;
    queue_read(XY_ADT7420_REG_ID, &id, 1U, XY_DEVICE_OK);
    queue_write(XY_ADT7420_REG_CONFIG, XY_ADT7420_CONFIG_RES_16BIT, XY_DEVICE_OK);
}

static void test_identity_init_read_and_deinit(void)
{
    const uint8_t raw[2] = {0x20U, 0x00U}; /* 0x2000 = 8192 -> 64.00 C */
    xy_adt7420_t dev;
    xy_adt7420_data_t sample;
    int bus;

    queue_valid_init(XY_ADT7420_ADDR_DEFAULT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_adt7420_init(&dev, &bus, XY_ADT7420_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_UINT16(XY_ADT7420_ADDR_DEFAULT, dev.i2c_dev.dev_addr);
    TEST_ASSERT_EQUAL_UINT32(1000U, dev.i2c_dev.timeout);
    queue_read(XY_ADT7420_REG_TEMP_MSB, raw, 2U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_adt7420_read(&dev, &sample));
    TEST_ASSERT_EQUAL_INT32(6400, sample.temperature_centi_c);
    TEST_ASSERT_EQUAL_INT32(6400, dev.data.temperature_centi_c);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_adt7420_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(g_count, g_index);
}

static void test_default_and_alternate_addresses(void)
{
    xy_adt7420_t dev;
    int bus;

    queue_valid_init(XY_ADT7420_ADDR_DEFAULT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_adt7420_init(&dev, &bus, 0U));
    TEST_ASSERT_EQUAL_UINT16(XY_ADT7420_ADDR_DEFAULT, dev.i2c_dev.dev_addr);
    TEST_ASSERT_EQUAL_UINT(g_count, g_index);

    setUp();
    queue_valid_init(XY_ADT7420_ADDR_ALT3);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_adt7420_init(&dev, &bus, XY_ADT7420_ADDR_ALT3));
    TEST_ASSERT_EQUAL_UINT16(XY_ADT7420_ADDR_ALT3, dev.i2c_dev.dev_addr);
    TEST_ASSERT_EQUAL_UINT(g_count, g_index);

    setUp();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_adt7420_init(&dev, &bus, 0x47U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_adt7420_init(&dev, &bus, 0x4CU));
    TEST_ASSERT_EQUAL_UINT(0U, g_index);
}

static void test_identity_mismatch_and_init_failures(void)
{
    const uint8_t bad_id = 0x00U;
    xy_adt7420_t dev;
    int bus;

    /* wrong ID value */
    queue_read(XY_ADT7420_REG_ID, &bad_id, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND, xy_adt7420_init(&dev, &bus, 0U));
    TEST_ASSERT_FALSE(dev.initialized);

    /* ID read transport failure */
    setUp();
    queue_read(XY_ADT7420_REG_ID, NULL, 1U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_adt7420_init(&dev, &bus, 0U));
    TEST_ASSERT_FALSE(dev.initialized);

    /* config write failure */
    setUp();
    {
        const uint8_t id = XY_ADT7420_ID_VALUE;
        queue_read(XY_ADT7420_REG_ID, &id, 1U, XY_DEVICE_OK);
    }
    queue_write(XY_ADT7420_REG_CONFIG, XY_ADT7420_CONFIG_RES_16BIT, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_adt7420_init(&dev, &bus, 0U));
    TEST_ASSERT_FALSE(dev.initialized);
}

static void test_init_rejects_null_and_incomplete_transport(void)
{
    xy_adt7420_t dev;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_adt7420_init(NULL, &bus, 0U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_adt7420_init(&dev, NULL, 0U));

    memset(&dev, 0xA5, sizeof(dev));
    g_init_establish_transport = 0;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_adt7420_init(&dev, &bus, 0U));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, g_index);
}

static void test_read_failure_preserves_output_and_cache(void)
{
    const uint8_t raw[2] = {0x01U, 0x46U}; /* 0x0146 = 326 -> 254 centi-C (2.54 C) */
    xy_adt7420_t dev;
    xy_adt7420_data_t out = {.temperature_centi_c = 12345};
    xy_adt7420_data_t before = out;
    int bus;

    queue_valid_init(XY_ADT7420_ADDR_DEFAULT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_adt7420_init(&dev, &bus, 0U));
    dev.data.temperature_centi_c = 777;

    queue_read(XY_ADT7420_REG_TEMP_MSB, NULL, 2U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_adt7420_read(&dev, &out));
    TEST_ASSERT_EQUAL_MEMORY(&before, &out, sizeof(out));
    TEST_ASSERT_EQUAL_INT32(777, dev.data.temperature_centi_c);

    queue_read(XY_ADT7420_REG_TEMP_MSB, raw, 2U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_adt7420_read(&dev, &out));
    TEST_ASSERT_EQUAL_INT32(254, out.temperature_centi_c);
    TEST_ASSERT_EQUAL_INT32(254, dev.data.temperature_centi_c);
}

static void test_missing_handle_fails_closed_without_io(void)
{
    xy_adt7420_t dev;
    xy_adt7420_data_t out = {.temperature_centi_c = 321};
    xy_adt7420_data_t snapshot = out;
    unsigned before;
    int bus;

    queue_valid_init(XY_ADT7420_ADDR_DEFAULT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_adt7420_init(&dev, &bus, 0U));
    dev.data = snapshot;
    before = g_index;
    dev.i2c_dev.i2c_handle = NULL;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_adt7420_read(&dev, &out));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_adt7420_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT(before, g_index);
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &out, sizeof(out));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev.data, sizeof(dev.data));
    TEST_ASSERT_TRUE(dev.initialized);
}

static void test_failed_reinit_preserves_live_owner(void)
{
    xy_adt7420_t dev;
    xy_adt7420_t snapshot;
    int bus;

    queue_valid_init(XY_ADT7420_ADDR_DEFAULT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_adt7420_init(&dev, &bus, 0U));
    dev.data.temperature_centi_c = -4567;
    snapshot = dev;

    setUp();
    g_init_ret = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_adt7420_init(&dev, &bus, 0U));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev, sizeof(dev));

    setUp();
    {
        const uint8_t bad_id = 0x00U;
        queue_read(XY_ADT7420_REG_ID, &bad_id, 1U, XY_DEVICE_OK);
    }
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND, xy_adt7420_init(&dev, &bus, 0U));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev, sizeof(dev));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_identity_init_read_and_deinit);
    RUN_TEST(test_default_and_alternate_addresses);
    RUN_TEST(test_identity_mismatch_and_init_failures);
    RUN_TEST(test_init_rejects_null_and_incomplete_transport);
    RUN_TEST(test_read_failure_preserves_output_and_cache);
    RUN_TEST(test_missing_handle_fails_closed_without_io);
    RUN_TEST(test_failed_reinit_preserves_live_owner);
    return UNITY_END();
}
