#include "unity.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_bmi270.h"
#include "xy_i2c.h"
#include "xy_spi.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef enum {
    OP_I2C,
    OP_SPI,
} bus_kind_t;

typedef struct {
    bus_kind_t kind;
    uint8_t addr;
    uint8_t reg;
    uint8_t tx[80];
    uint16_t tx_len;
    uint8_t rx[16];
    uint16_t rx_len;
    int ret;
} bus_op_t;

static bus_op_t g_ops[160];
static size_t g_op_count;
static size_t g_op_index;
static int g_log_count;

static void queue_i2c_write(uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    g_ops[g_op_count].kind = OP_I2C;
    g_ops[g_op_count].addr = addr;
    g_ops[g_op_count].reg = reg;
    if (data != NULL && len > 0U) {
        memcpy(g_ops[g_op_count].tx, data, len);
    }
    g_ops[g_op_count].tx_len = len;
    g_ops[g_op_count].rx_len = 0;
    g_ops[g_op_count].ret = ret;
    g_op_count++;
}

static void queue_i2c_read(uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    g_ops[g_op_count].kind = OP_I2C;
    g_ops[g_op_count].addr = addr;
    g_ops[g_op_count].reg = reg;
    g_ops[g_op_count].tx_len = 0;
    if (data != NULL && len > 0U) {
        memcpy(g_ops[g_op_count].rx, data, len);
    }
    g_ops[g_op_count].rx_len = len;
    g_ops[g_op_count].ret = ret;
    g_op_count++;
}

static void queue_i2c_read8(uint8_t addr, uint8_t reg, uint8_t value, int ret)
{
    queue_i2c_read(addr, reg, &value, 1U, ret);
}

static void queue_spi_write(uint8_t cs, uint8_t reg, const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    g_ops[g_op_count].kind = OP_SPI;
    g_ops[g_op_count].addr = cs;
    g_ops[g_op_count].reg = reg;
    g_ops[g_op_count].tx[0] = reg;
    if (data != NULL && len > 0U) {
        memcpy(&g_ops[g_op_count].tx[1], data, len);
    }
    g_ops[g_op_count].tx_len = (uint16_t)(len + 1U);
    g_ops[g_op_count].rx_len = 0;
    g_ops[g_op_count].ret = ret;
    g_op_count++;
}

static void queue_spi_read(uint8_t cs, uint8_t reg, const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    g_ops[g_op_count].kind = OP_SPI;
    g_ops[g_op_count].addr = cs;
    g_ops[g_op_count].reg = (uint8_t)(reg | 0x80U);
    g_ops[g_op_count].tx[0] = (uint8_t)(reg | 0x80U);
    g_ops[g_op_count].tx_len = 1U;
    if (data != NULL && len > 0U) {
        memcpy(g_ops[g_op_count].rx, data, len);
    }
    g_ops[g_op_count].rx_len = len;
    g_ops[g_op_count].ret = ret;
    g_op_count++;
}

static void queue_default_i2c_init(uint8_t addr)
{
    uint8_t reset = 0xB6U;
    uint8_t acc_conf = (uint8_t)((0x0AU << 4) | BMI270_ACC_PERF_MODE);
    uint8_t acc_range = BMI270_ACC_RANGE_4G;
    uint8_t gyr_conf = (uint8_t)(0x0AU << 4);
    uint8_t gyr_range = BMI270_GYR_RANGE_500;
    uint8_t acc_conf_before = 0xA0U;
    uint8_t acc_conf_after = (uint8_t)(acc_conf_before | BMI270_ACC_EN);
    uint8_t gyr_conf_before = 0xA0U;
    uint8_t gyr_conf_after = (uint8_t)(gyr_conf_before | BMI270_GYR_EN);

    queue_i2c_write(addr, 0xB6U, &reset, 1U, XY_DEVICE_OK);
    queue_i2c_read8(addr, BMI270_REG_CHIPID, BMI270_CHIPID_VAL, XY_DEVICE_OK);
    queue_i2c_write(addr, BMI270_REG_ACC_CONF, &acc_conf, 1U, XY_DEVICE_OK);
    queue_i2c_write(addr, BMI270_REG_ACC_RANGE, &acc_range, 1U, XY_DEVICE_OK);
    queue_i2c_write(addr, BMI270_REG_GYR_CONF, &gyr_conf, 1U, XY_DEVICE_OK);
    queue_i2c_write(addr, BMI270_REG_GYR_RANGE, &gyr_range, 1U, XY_DEVICE_OK);
    queue_i2c_read8(addr, BMI270_REG_ACC_CONF, acc_conf_before, XY_DEVICE_OK);
    queue_i2c_write(addr, BMI270_REG_ACC_CONF, &acc_conf_after, 1U, XY_DEVICE_OK);
    queue_i2c_read8(addr, BMI270_REG_GYR_CONF, gyr_conf_before, XY_DEVICE_OK);
    queue_i2c_write(addr, BMI270_REG_GYR_CONF, &gyr_conf_after, 1U, XY_DEVICE_OK);
}

static bus_op_t *next_op(bus_kind_t kind)
{
    TEST_ASSERT_LESS_THAN_UINT(g_op_count, g_op_index);
    TEST_ASSERT_EQUAL_INT(kind, g_ops[g_op_index].kind);
    return &g_ops[g_op_index++];
}

int xy_i2c_master_transmit(xy_i2c_t *i2c, uint8_t addr, const uint8_t *tx, uint16_t tx_len,
                           const void *data, uint16_t data_len, uint32_t timeout_ms)
{
    (void)i2c;
    TEST_ASSERT_EQUAL_UINT32(100U, timeout_ms);
    TEST_ASSERT_NOT_NULL(tx);
    TEST_ASSERT_EQUAL_UINT16(1U, tx_len);
    bus_op_t *op = next_op(OP_I2C);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, tx[0]);
    if (op->tx_len > 0U) {
        TEST_ASSERT_NOT_NULL(data);
        TEST_ASSERT_EQUAL_UINT16(op->tx_len, data_len);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(op->tx, data, data_len);
    } else {
        TEST_ASSERT_NOT_NULL(data);
        TEST_ASSERT_EQUAL_UINT16(op->rx_len, data_len);
        if (op->ret == XY_DEVICE_OK) {
            memcpy((void *)data, op->rx, data_len);
        }
    }
    return op->ret;
}

int xy_spi_transfer(xy_spi_t *spi, uint8_t cs, const uint8_t *tx, uint16_t tx_len,
                    uint8_t *rx, uint16_t rx_len, uint32_t timeout_ms)
{
    (void)spi;
    TEST_ASSERT_EQUAL_UINT32(100U, timeout_ms);
    bus_op_t *op = next_op(OP_SPI);
    TEST_ASSERT_EQUAL_UINT8(op->addr, cs);
    TEST_ASSERT_EQUAL_UINT16(op->tx_len, tx_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->tx, tx, tx_len);
    TEST_ASSERT_EQUAL_UINT16(op->rx_len, rx_len);
    if (rx != NULL && op->ret == XY_DEVICE_OK) {
        memcpy(rx, op->rx, rx_len);
    }
    return op->ret;
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    g_log_count++;
    return 0;
}

void setUp(void)
{
    memset(g_ops, 0, sizeof(g_ops));
    g_op_count = 0;
    g_op_index = 0;
    g_log_count = 0;
}

void tearDown(void)
{
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);
}

static xy_bmi270_t ready_i2c_dev(void)
{
    xy_bmi270_t dev;
    memset(&dev, 0, sizeof(dev));
    dev.bus_handle = (void *)0x1234;
    dev.bus_addr = 0x68U;
    dev.is_spi = false;
    dev.initialized = true;
    dev.range.acc_range = BMI270_ACC_RANGE_4G;
    dev.range.gyr_range = BMI270_GYR_RANGE_500;
    dev.acc_scale = 4.0f * 9.80665f / 32768.0f;
    dev.gyr_scale = 500.0f * 0.017453292519943295f / 32768.0f;
    return dev;
}

static void test_bmi270_invalid_paths_and_init_i2c(void)
{
    xy_bmi270_t dev;
    uint8_t bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bmi270_init(NULL, &bus, 0x68U, false));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bmi270_init(&dev, NULL, 0x68U, false));

    queue_default_i2c_init(0x68U);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmi270_init(&dev, &bus, 0x68U, false));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_PTR(&bus, dev.bus_handle);
    TEST_ASSERT_EQUAL_UINT8(0x68U, dev.bus_addr);
    TEST_ASSERT_FALSE(dev.is_spi);
    TEST_ASSERT_EQUAL_UINT8(BMI270_ACC_RANGE_4G, dev.range.acc_range);
    TEST_ASSERT_EQUAL_UINT8(BMI270_GYR_RANGE_500, dev.range.gyr_range);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 4.0f * 9.80665f / 32768.0f, dev.acc_scale);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 500.0f * 0.017453292519943295f / 32768.0f, dev.gyr_scale);

    uint8_t reset = 0xB6U;
    queue_i2c_write(0x68U, 0xB6U, &reset, 1U, XY_DEVICE_OK);
    queue_i2c_read8(0x68U, BMI270_REG_CHIPID, 0x00U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ENODEV, xy_bmi270_init(&dev, &bus, 0x68U, false));
}

static void test_bmi270_register_access_and_spi_mode(void)
{
    xy_bmi270_t dev;
    uint8_t spi;
    uint8_t val = 0x5AU;
    uint8_t out = 0;
    uint8_t chip = 0;

    memset(&dev, 0, sizeof(dev));
    dev.bus_handle = &spi;
    dev.bus_addr = 3U;
    dev.is_spi = true;
    dev.initialized = true;

    queue_spi_write(3U, BMI270_REG_ACC_RANGE, &val, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmi270_write_regs(&dev, BMI270_REG_ACC_RANGE, &val, 1U));

    queue_spi_read(3U, BMI270_REG_CHIPID, &val, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmi270_read_regs(&dev, BMI270_REG_CHIPID, &out, 1U));
    TEST_ASSERT_EQUAL_UINT8(0x5AU, out);

    queue_spi_read(3U, BMI270_REG_CHIPID, &val, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmi270_get_chip_id(&dev, &chip));
    TEST_ASSERT_EQUAL_UINT8(0x5AU, chip);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bmi270_read_regs(NULL, BMI270_REG_CHIPID, &out, 1U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bmi270_read_regs(&dev, BMI270_REG_CHIPID, NULL, 1U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bmi270_write_regs(&dev, BMI270_REG_CHIPID, &val, 0U));
}

static void test_bmi270_range_enable_and_raw_data(void)
{
    xy_bmi270_t dev = ready_i2c_dev();
    bmi270_range_t range = {
        .acc_range = BMI270_ACC_RANGE_8G,
        .gyr_range = BMI270_GYR_RANGE_250,
        .acc_odr = 0x09U,
        .gyr_odr = 0x08U,
    };
    uint8_t acc_conf = (uint8_t)((range.acc_odr << 4) | BMI270_ACC_PERF_MODE);
    uint8_t gyr_conf = (uint8_t)(range.gyr_odr << 4);
    uint8_t status = (uint8_t)(BMI270_DRDY_ACC | BMI270_DRDY_GYR);
    uint8_t raw_bytes[12] = {
        0x00U, 0x10U, 0x00U, 0xF0U, 0x00U, 0x08U,
        0x00U, 0x04U, 0x00U, 0xFCU, 0x00U, 0x02U,
    };
    uint8_t time_bytes[3] = {0x11U, 0x22U, 0x33U};
    bmi270_raw_data_t raw;
    bmi270_data_t data;

    queue_i2c_write(0x68U, BMI270_REG_ACC_CONF, &acc_conf, 1U, XY_DEVICE_OK);
    queue_i2c_write(0x68U, BMI270_REG_ACC_RANGE, &range.acc_range, 1U, XY_DEVICE_OK);
    queue_i2c_write(0x68U, BMI270_REG_GYR_CONF, &gyr_conf, 1U, XY_DEVICE_OK);
    queue_i2c_write(0x68U, BMI270_REG_GYR_RANGE, &range.gyr_range, 1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmi270_set_range(&dev, &range));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 8.0f * 9.80665f / 32768.0f, dev.acc_scale);
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 250.0f * 0.017453292519943295f / 32768.0f, dev.gyr_scale);

    queue_i2c_read8(0x68U, BMI270_REG_ACC_CONF, 0xA0U, XY_DEVICE_OK);
    { uint8_t expected = 0xA1U; queue_i2c_write(0x68U, BMI270_REG_ACC_CONF, &expected, 1U, XY_DEVICE_OK); }
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmi270_enable_acc(&dev, true));

    queue_i2c_read8(0x68U, BMI270_REG_GYR_CONF, 0xA1U, XY_DEVICE_OK);
    { uint8_t expected = 0xA0U; queue_i2c_write(0x68U, BMI270_REG_GYR_CONF, &expected, 1U, XY_DEVICE_OK); }
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmi270_enable_gyr(&dev, false));

    queue_i2c_read8(0x68U, BMI270_REG_STATUS, status, XY_DEVICE_OK);
    queue_i2c_read(0x68U, BMI270_REG_ACC_X_LSB, raw_bytes, sizeof(raw_bytes), XY_DEVICE_OK);
    queue_i2c_read(0x68U, BMI270_REG_SENSORTIME_0, time_bytes, sizeof(time_bytes), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmi270_read_raw(&dev, &raw));
    TEST_ASSERT_EQUAL_INT16(0x1000, raw.acc_x);
    TEST_ASSERT_EQUAL_INT16((int16_t)0xF000, raw.acc_y);
    TEST_ASSERT_EQUAL_INT16(0x0400, raw.gyr_x);
    TEST_ASSERT_EQUAL_UINT32(0x332211U, raw.sensor_time);

    queue_i2c_read8(0x68U, BMI270_REG_STATUS, status, XY_DEVICE_OK);
    queue_i2c_read(0x68U, BMI270_REG_ACC_X_LSB, raw_bytes, sizeof(raw_bytes), XY_DEVICE_OK);
    queue_i2c_read(0x68U, BMI270_REG_SENSORTIME_0, time_bytes, sizeof(time_bytes), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmi270_read_data(&dev, &data));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0x1000 * dev.acc_scale, data.acc_x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, ((int16_t)0xF000) * dev.acc_scale, data.acc_y);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0x0400 * dev.gyr_scale, data.gyr_x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, data.temperature);
}

static void test_bmi270_not_ready_sleep_wakeup_and_deinit(void)
{
    xy_bmi270_t dev = ready_i2c_dev();
    bmi270_raw_data_t raw;
    uint8_t status = 0;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bmi270_set_range(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bmi270_enable_acc(NULL, true));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bmi270_get_status(&dev, NULL));

    queue_i2c_read8(0x68U, BMI270_REG_STATUS, 0x00U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EAGAIN, xy_bmi270_read_raw(&dev, &raw));

    queue_i2c_read8(0x68U, BMI270_REG_STATUS, (uint8_t)(BMI270_DRDY_ACC | BMI270_DRDY_GYR), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmi270_get_status(&dev, &status));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(BMI270_DRDY_ACC | BMI270_DRDY_GYR), status);

    queue_i2c_read8(0x68U, BMI270_REG_ACC_CONF, 0xA0U, XY_DEVICE_OK);
    { uint8_t expected = 0xA1U; queue_i2c_write(0x68U, BMI270_REG_ACC_CONF, &expected, 1U, XY_DEVICE_OK); }
    queue_i2c_read8(0x68U, BMI270_REG_GYR_CONF, 0xA0U, XY_DEVICE_OK);
    { uint8_t expected = 0xA1U; queue_i2c_write(0x68U, BMI270_REG_GYR_CONF, &expected, 1U, XY_DEVICE_OK); }
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmi270_wakeup(&dev));

    queue_i2c_read8(0x68U, BMI270_REG_ACC_CONF, 0xA1U, XY_DEVICE_OK);
    { uint8_t expected = 0xA0U; queue_i2c_write(0x68U, BMI270_REG_ACC_CONF, &expected, 1U, XY_DEVICE_OK); }
    queue_i2c_read8(0x68U, BMI270_REG_GYR_CONF, 0xA1U, XY_DEVICE_OK);
    { uint8_t expected = 0xA0U; queue_i2c_write(0x68U, BMI270_REG_GYR_CONF, &expected, 1U, XY_DEVICE_OK); }
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmi270_sleep(&dev));

    queue_i2c_read8(0x68U, BMI270_REG_ACC_CONF, 0xA1U, XY_DEVICE_OK);
    { uint8_t expected = 0xA0U; queue_i2c_write(0x68U, BMI270_REG_ACC_CONF, &expected, 1U, XY_DEVICE_OK); }
    queue_i2c_read8(0x68U, BMI270_REG_GYR_CONF, 0xA1U, XY_DEVICE_OK);
    { uint8_t expected = 0xA0U; queue_i2c_write(0x68U, BMI270_REG_GYR_CONF, &expected, 1U, XY_DEVICE_OK); }
    queue_i2c_read8(0x68U, BMI270_REG_ACC_CONF, 0xA0U, XY_DEVICE_OK);
    { uint8_t expected = 0xA0U; queue_i2c_write(0x68U, BMI270_REG_ACC_CONF, &expected, 1U, XY_DEVICE_OK); }
    queue_i2c_read8(0x68U, BMI270_REG_GYR_CONF, 0xA0U, XY_DEVICE_OK);
    { uint8_t expected = 0xA0U; queue_i2c_write(0x68U, BMI270_REG_GYR_CONF, &expected, 1U, XY_DEVICE_OK); }
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bmi270_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bmi270_deinit(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bmi270_invalid_paths_and_init_i2c);
    RUN_TEST(test_bmi270_register_access_and_spi_mode);
    RUN_TEST(test_bmi270_range_enable_and_raw_data);
    RUN_TEST(test_bmi270_not_ready_sleep_wakeup_and_deinit);
    return UNITY_END();
}
