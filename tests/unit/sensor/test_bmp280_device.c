#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_bmp280.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef enum {
    OP_READ_REG,
    OP_WRITE_REG,
} op_kind_t;

typedef struct {
    op_kind_t kind;
    uint8_t reg;
    uint8_t data[24];
    size_t len;
    xy_error_t ret;
} i2c_op_t;

static i2c_op_t g_ops[16];
static size_t g_op_count;
static size_t g_op_index;
static uint16_t g_last_addr;
static uint32_t g_last_timeout;

static const uint8_t g_calibration[24] = {
    0x70, 0x6B, 0x43, 0x67, 0x18, 0xFC, 0x7D, 0x8E, 0x43, 0xD6, 0xD0, 0x0B,
    0x27, 0x0B, 0x8C, 0x00, 0xF9, 0xFF, 0x8C, 0x3C, 0xF8, 0xC6, 0x70, 0x17,
};

static void queue_read(uint8_t reg, const uint8_t *data, size_t len, xy_error_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    g_ops[g_op_count].kind = OP_READ_REG;
    g_ops[g_op_count].reg = reg;
    g_ops[g_op_count].len = len;
    g_ops[g_op_count].ret = ret;
    if (data != NULL) {
        memcpy(g_ops[g_op_count].data, data, len);
    }
    g_op_count++;
}

static void queue_read8(uint8_t reg, uint8_t value, xy_error_t ret)
{
    queue_read(reg, &value, 1U, ret);
}

static void queue_write8(uint8_t reg, uint8_t value, xy_error_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    g_ops[g_op_count].kind = OP_WRITE_REG;
    g_ops[g_op_count].reg = reg;
    g_ops[g_op_count].data[0] = value;
    g_ops[g_op_count].len = 1U;
    g_ops[g_op_count].ret = ret;
    g_op_count++;
}

static i2c_op_t *next_op(op_kind_t kind)
{
    TEST_ASSERT_LESS_THAN_UINT(g_op_count, g_op_index);
    TEST_ASSERT_EQUAL_INT(kind, g_ops[g_op_index].kind);
    return &g_ops[g_op_index++];
}

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle, uint16_t addr,
                              uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(i2c_handle);
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1U;
    dev->i2c_handle = i2c_handle;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    g_last_addr = addr;
    g_last_timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    i2c_op_t *op;

    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    op = next_op(OP_READ_REG);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    if (op->ret == XY_DEVICE_OK) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data,
                                   size_t len)
{
    i2c_op_t *op;

    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    op = next_op(OP_WRITE_REG);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    return op->ret;
}

void setUp(void)
{
    memset(g_ops, 0, sizeof(g_ops));
    g_op_count = 0U;
    g_op_index = 0U;
    g_last_addr = 0U;
    g_last_timeout = 0U;
}

void tearDown(void)
{
}

static void queue_init_success(void)
{
    queue_read8(BMP280_REG_ID, BMP280_ID_VALUE, XY_DEVICE_OK);
    queue_write8(BMP280_REG_RESET, 0xB6U, XY_DEVICE_OK);
    queue_read(BMP280_REG_CALIB, g_calibration, sizeof(g_calibration), XY_DEVICE_OK);
    queue_write8(BMP280_REG_CONFIG, 0x00U, XY_DEVICE_OK);
    queue_write8(BMP280_REG_CTRL_MEAS, 0x27U, XY_DEVICE_OK);
}

static void init_success(xy_bmp280_t *bmp, int *bus, uint8_t addr)
{
    queue_init_success();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmp280_init_addr(bmp, bus, addr));
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);
}

static void test_bmp280_init_supports_both_addresses_and_parses_calibration(void)
{
    xy_bmp280_t bmp;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_bmp280_init_addr(NULL, &bus, BMP280_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_bmp280_init_addr(&bmp, NULL, BMP280_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bmp280_init_addr(&bmp, &bus, 0x75U));

    init_success(&bmp, &bus, BMP280_ADDR_ALT);
    TEST_ASSERT_EQUAL_UINT8(BMP280_ADDR_ALT, bmp.addr);
    TEST_ASSERT_EQUAL_UINT16(BMP280_ADDR_ALT, g_last_addr);
    TEST_ASSERT_EQUAL_UINT32(1000U, g_last_timeout);
    TEST_ASSERT_TRUE(bmp.initialized);
    TEST_ASSERT_EQUAL_UINT16(27504U, bmp.calibration.dig_t1);
    TEST_ASSERT_EQUAL_INT16(26435, bmp.calibration.dig_t2);
    TEST_ASSERT_EQUAL_INT16(-1000, bmp.calibration.dig_t3);
    TEST_ASSERT_EQUAL_UINT16(36477U, bmp.calibration.dig_p1);
}

static void test_bmp280_init_propagates_each_io_failure(void)
{
    xy_bmp280_t bmp;
    int bus;

    queue_read8(BMP280_REG_ID, BMP280_ID_VALUE, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_bmp280_init_addr(&bmp, &bus, BMP280_ADDR_DEFAULT));
    TEST_ASSERT_FALSE(bmp.initialized);

    queue_read8(BMP280_REG_ID, 0x00U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND,
                          xy_bmp280_init_addr(&bmp, &bus, BMP280_ADDR_DEFAULT));

    queue_read8(BMP280_REG_ID, BMP280_ID_VALUE, XY_DEVICE_OK);
    queue_write8(BMP280_REG_RESET, 0xB6U, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_bmp280_init_addr(&bmp, &bus, BMP280_ADDR_DEFAULT));

    queue_read8(BMP280_REG_ID, BMP280_ID_VALUE, XY_DEVICE_OK);
    queue_write8(BMP280_REG_RESET, 0xB6U, XY_DEVICE_OK);
    queue_read(BMP280_REG_CALIB, NULL, sizeof(g_calibration), XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_bmp280_init_addr(&bmp, &bus, BMP280_ADDR_DEFAULT));

    queue_read8(BMP280_REG_ID, BMP280_ID_VALUE, XY_DEVICE_OK);
    queue_write8(BMP280_REG_RESET, 0xB6U, XY_DEVICE_OK);
    queue_read(BMP280_REG_CALIB, g_calibration, sizeof(g_calibration), XY_DEVICE_OK);
    queue_write8(BMP280_REG_CONFIG, 0x00U, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_bmp280_init_addr(&bmp, &bus, BMP280_ADDR_DEFAULT));

    queue_read8(BMP280_REG_ID, BMP280_ID_VALUE, XY_DEVICE_OK);
    queue_write8(BMP280_REG_RESET, 0xB6U, XY_DEVICE_OK);
    queue_read(BMP280_REG_CALIB, g_calibration, sizeof(g_calibration), XY_DEVICE_OK);
    queue_write8(BMP280_REG_CONFIG, 0x00U, XY_DEVICE_OK);
    queue_write8(BMP280_REG_CTRL_MEAS, 0x27U, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_bmp280_init_addr(&bmp, &bus, BMP280_ADDR_DEFAULT));
    TEST_ASSERT_FALSE(bmp.initialized);
}

static void test_bmp280_compensates_bosch_sample_and_preserves_cache_on_failure(void)
{
    const uint8_t raw[6] = {0x65, 0x5A, 0xC0, 0x7E, 0xED, 0x00};
    xy_bmp280_t bmp;
    int32_t temperature = -1;
    uint32_t pressure = 0U;
    int bus;

    init_success(&bmp, &bus, BMP280_ADDR_DEFAULT);
    queue_read(BMP280_REG_PRESS_DATA, raw, sizeof(raw), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmp280_read(&bmp));
    TEST_ASSERT_EQUAL_INT32(2508, bmp.temperature);
    TEST_ASSERT_EQUAL_UINT32(100653U, bmp.pressure);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmp280_get_temperature(&bmp, &temperature));
    TEST_ASSERT_EQUAL_INT32(2508, temperature);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmp280_get_pressure(&bmp, &pressure));
    TEST_ASSERT_EQUAL_UINT32(100653U, pressure);

    queue_read(BMP280_REG_PRESS_DATA, NULL, sizeof(raw), XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_bmp280_read(&bmp));
    TEST_ASSERT_EQUAL_INT32(2508, bmp.temperature);
    TEST_ASSERT_EQUAL_UINT32(100653U, bmp.pressure);
}

static void test_bmp280_deinit_failure_preserves_state(void)
{
    xy_bmp280_t bmp;
    int bus;

    init_success(&bmp, &bus, BMP280_ADDR_DEFAULT);
    queue_write8(BMP280_REG_CTRL_MEAS, 0x00U, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_bmp280_deinit(&bmp));
    TEST_ASSERT_TRUE(bmp.initialized);

    queue_write8(BMP280_REG_CTRL_MEAS, 0x00U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmp280_deinit(&bmp));
    TEST_ASSERT_FALSE(bmp.initialized);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_bmp280_read(&bmp));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bmp280_init_supports_both_addresses_and_parses_calibration);
    RUN_TEST(test_bmp280_init_propagates_each_io_failure);
    RUN_TEST(test_bmp280_compensates_bosch_sample_and_preserves_cache_on_failure);
    RUN_TEST(test_bmp280_deinit_failure_preserves_state);
    return UNITY_END();
}
