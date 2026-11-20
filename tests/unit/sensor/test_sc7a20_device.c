#include "unity.h"
#include "xy_sc7a20.h"

#include <string.h>

typedef struct {
    uint8_t reg;
    uint8_t data[8];
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
    g_tick = 900U;
    g_init_ret = XY_DEVICE_OK;
    g_init_establish_transport = 1;
}

void tearDown(void) {}

static void queue_valid_init(void)
{
    const uint8_t whoami = XY_SC7A20_WHOAMI_VALUE;
    queue_read(XY_SC7A20_REG_WHOAMI, &whoami, 1U, XY_DEVICE_OK);
    queue_write(XY_SC7A20_REG_CTRL_REG1, (uint8_t)(XY_SC7A20_ODR_100HZ | XY_SC7A20_AXIS_ENABLE_MASK),
                XY_DEVICE_OK);
    queue_write(XY_SC7A20_REG_CTRL_REG4, (uint8_t)(XY_SC7A20_RANGE_2G | XY_SC7A20_BDU_MASK),
                XY_DEVICE_OK);
}

static void test_sc7a20_identity_init_read_and_deinit(void)
{
    xy_sc7a20_t dev;
    xy_sc7a20_data_t data;
    /* 12-bit left-aligned: 0x1E0 -> raw 0x01E = 30, 0xFFF0 -> -1 after the sign-aware shift */
    const uint8_t raw[6] = {0xE0U, 0x01U, 0x00U, 0x00U, 0xF0U, 0xFFU};
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_init(&dev, &bus));
    TEST_ASSERT_EQUAL_UINT16(XY_SC7A20_ADDR, dev.i2c_dev.dev_addr);
    TEST_ASSERT_EQUAL_UINT8(2U, dev.range_g);
    TEST_ASSERT_TRUE(dev.initialized);

    queue_read(XY_SC7A20_REG_OUT_X_L, raw, 6U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_read(&dev, &data));
    TEST_ASSERT_EQUAL_INT16(30, data.x);
    TEST_ASSERT_EQUAL_INT16(0, data.y);
    TEST_ASSERT_EQUAL_INT16(-1, data.z);
    TEST_ASSERT_EQUAL_UINT32(900U, data.timestamp);

    queue_write(XY_SC7A20_REG_CTRL_REG1, XY_SC7A20_ODR_POWER_DOWN, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(g_count, g_index);
}

static void test_sc7a20_read_accel_converts_to_milli_g(void)
{
    xy_sc7a20_t dev;
    xy_sc7a20_accel_t accel;
    /* 0x0800 -> raw 0x080 = 128 counts; at +/-2 g that is 128 * 2000 / 2048 = 125 mg */
    const uint8_t raw[6] = {0x00U, 0x08U, 0x00U, 0x08U, 0x00U, 0x08U};
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_init(&dev, &bus));
    queue_read(XY_SC7A20_REG_OUT_X_L, raw, 6U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_read_accel(&dev, &accel));
    TEST_ASSERT_EQUAL_INT32(125, accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(125, accel.y_mg);
    TEST_ASSERT_EQUAL_INT32(125, accel.z_mg);
}

static void test_sc7a20_range_change_changes_the_scale(void)
{
    xy_sc7a20_t dev;
    xy_sc7a20_accel_t accel;
    const uint8_t raw[6] = {0x00U, 0x08U, 0x00U, 0x08U, 0x00U, 0x08U};
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_init(&dev, &bus));
    queue_write(XY_SC7A20_REG_CTRL_REG4, (uint8_t)(XY_SC7A20_RANGE_16G | XY_SC7A20_BDU_MASK),
                XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_set_range(&dev, 16U));
    TEST_ASSERT_EQUAL_UINT8(16U, dev.range_g);

    queue_read(XY_SC7A20_REG_OUT_X_L, raw, 6U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_read_accel(&dev, &accel));
    /* 128 counts at +/-16 g: 128 * 16000 / 2048 = 1000 mg */
    TEST_ASSERT_EQUAL_INT32(1000, accel.x_mg);
}

static void test_sc7a20_set_range_rejects_unsupported_values(void)
{
    xy_sc7a20_t dev;
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_init(&dev, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_set_range(&dev, 3U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_set_range(&dev, 0U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_set_range(&dev, 32U));
    TEST_ASSERT_EQUAL_UINT8(2U, dev.range_g);
    /* no register traffic for a rejected range */
    TEST_ASSERT_EQUAL_UINT(3U, g_index);
}

static void test_sc7a20_set_odr_accepts_supported_rates_and_rejects_others(void)
{
    xy_sc7a20_t dev;
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_init(&dev, &bus));
    queue_write(XY_SC7A20_REG_CTRL_REG1, (uint8_t)(XY_SC7A20_ODR_400HZ | XY_SC7A20_AXIS_ENABLE_MASK),
                XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_set_odr(&dev, XY_SC7A20_ODR_400HZ));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(XY_SC7A20_ODR_400HZ | XY_SC7A20_AXIS_ENABLE_MASK), dev.odr_reg);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_set_odr(&dev, 0x01U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_set_odr(&dev, 0x80U));
    TEST_ASSERT_EQUAL_UINT(4U, g_index);
}

static void test_sc7a20_wrong_whoami_is_rejected(void)
{
    xy_sc7a20_t dev;
    const uint8_t bad = 0x00U;
    int bus;

    queue_read(XY_SC7A20_REG_WHOAMI, &bad, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND, xy_sc7a20_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_UINT(1U, g_index);
}

static void test_sc7a20_config_write_failure_propagates_and_leaves_owner_uninitialised(void)
{
    xy_sc7a20_t dev;
    const uint8_t whoami = XY_SC7A20_WHOAMI_VALUE;
    int bus;

    queue_read(XY_SC7A20_REG_WHOAMI, &whoami, 1U, XY_DEVICE_OK);
    queue_write(XY_SC7A20_REG_CTRL_REG1, (uint8_t)(XY_SC7A20_ODR_100HZ | XY_SC7A20_AXIS_ENABLE_MASK),
                XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_sc7a20_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT(2U, g_index);
}

static void test_sc7a20_read_failure_preserves_output(void)
{
    xy_sc7a20_t dev;
    xy_sc7a20_data_t data = {.x = 11, .y = 22, .z = 33, .timestamp = 44U};
    const xy_sc7a20_data_t snapshot = data;
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_init(&dev, &bus));
    queue_read(XY_SC7A20_REG_OUT_X_L, NULL, 6U, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_sc7a20_read(&dev, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
}

static void test_sc7a20_init_rejects_incomplete_nested_transport(void)
{
    xy_sc7a20_t dev;
    int bus;

    memset(&dev, 0xA5, sizeof(dev));
    g_init_establish_transport = 0;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, g_index);
}

static void test_sc7a20_missing_handle_fails_closed(void)
{
    xy_sc7a20_t dev;
    xy_sc7a20_data_t output = {.x = 11, .y = 22, .z = 33, .timestamp = 44U};
    const xy_sc7a20_data_t snapshot = output;
    unsigned before;
    int bus;

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_init(&dev, &bus));
    dev.data = snapshot;
    before = g_index;
    dev.i2c_dev.i2c_handle = NULL;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_read(&dev, &output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_read_accel(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT(before, g_index);
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev.data, sizeof(dev.data));
    TEST_ASSERT_TRUE(dev.initialized);
}

static void test_sc7a20_null_arguments_are_rejected(void)
{
    xy_sc7a20_t dev;
    xy_sc7a20_data_t data;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_init(NULL, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_init(&dev, NULL));

    queue_valid_init();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sc7a20_init(&dev, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_read(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sc7a20_read_accel(&dev, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sc7a20_identity_init_read_and_deinit);
    RUN_TEST(test_sc7a20_read_accel_converts_to_milli_g);
    RUN_TEST(test_sc7a20_range_change_changes_the_scale);
    RUN_TEST(test_sc7a20_set_range_rejects_unsupported_values);
    RUN_TEST(test_sc7a20_set_odr_accepts_supported_rates_and_rejects_others);
    RUN_TEST(test_sc7a20_wrong_whoami_is_rejected);
    RUN_TEST(test_sc7a20_config_write_failure_propagates_and_leaves_owner_uninitialised);
    RUN_TEST(test_sc7a20_read_failure_preserves_output);
    RUN_TEST(test_sc7a20_init_rejects_incomplete_nested_transport);
    RUN_TEST(test_sc7a20_missing_handle_fails_closed);
    RUN_TEST(test_sc7a20_null_arguments_are_rejected);
    return UNITY_END();
}
