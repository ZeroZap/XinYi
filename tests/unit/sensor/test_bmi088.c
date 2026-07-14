#include "unity.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_bmi088.h"
#include "xy_ret.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef enum {
    OP_READ,
    OP_WRITE,
} op_kind_t;

typedef struct {
    op_kind_t kind;
    uint8_t cs;
    uint8_t reg;
    uint8_t data[6];
    uint16_t len;
    xy_ret_t ret;
} spi_op_t;

static spi_op_t g_ops[128];
static uint8_t g_seen_writes[128][3];
static size_t g_op_count;
static size_t g_op_index;
static size_t g_seen_write_count;
static uint32_t g_delay_total;

static void queue_write(uint8_t cs, uint8_t reg, uint8_t value, xy_ret_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    g_ops[g_op_count].kind = OP_WRITE;
    g_ops[g_op_count].cs = cs;
    g_ops[g_op_count].reg = reg;
    g_ops[g_op_count].data[0] = value;
    g_ops[g_op_count].len = 1U;
    g_ops[g_op_count].ret = ret;
    g_op_count++;
}

static void queue_read(uint8_t cs, uint8_t reg, const uint8_t *data, uint16_t len, xy_ret_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    g_ops[g_op_count].kind = OP_READ;
    g_ops[g_op_count].cs = cs;
    g_ops[g_op_count].reg = reg;
    g_ops[g_op_count].len = len;
    g_ops[g_op_count].ret = ret;
    if (data != NULL) {
        memcpy(g_ops[g_op_count].data, data, len);
    }
    g_op_count++;
}

static void queue_read8(uint8_t cs, uint8_t reg, uint8_t value, xy_ret_t ret)
{
    queue_read(cs, reg, &value, 1U, ret);
}

static void queue_read_xyz(uint8_t cs, uint8_t reg, int16_t x, int16_t y, int16_t z, xy_ret_t ret)
{
    uint8_t data[6] = {
        (uint8_t)x, (uint8_t)(x >> 8),
        (uint8_t)y, (uint8_t)(y >> 8),
        (uint8_t)z, (uint8_t)(z >> 8),
    };
    queue_read(cs, reg, data, sizeof(data), ret);
}

static spi_op_t *next_op(op_kind_t kind)
{
    TEST_ASSERT_LESS_THAN_UINT(g_op_count, g_op_index);
    TEST_ASSERT_EQUAL_INT(kind, g_ops[g_op_index].kind);
    return &g_ops[g_op_index++];
}

xy_ret_t xy_spi_write_reg(xy_spi_dev_t *dev, uint8_t cs_pin, uint8_t reg_addr,
                          const uint8_t *data, uint16_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(data);
    spi_op_t *op = next_op(OP_WRITE);
    TEST_ASSERT_EQUAL_UINT8(op->cs, cs_pin);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg_addr);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    g_seen_writes[g_seen_write_count][0] = cs_pin;
    g_seen_writes[g_seen_write_count][1] = reg_addr;
    g_seen_writes[g_seen_write_count][2] = data[0];
    g_seen_write_count++;
    return op->ret;
}

xy_ret_t xy_spi_read_reg(xy_spi_dev_t *dev, uint8_t cs_pin, uint8_t reg_addr,
                         uint8_t *data, uint16_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(data);
    spi_op_t *op = next_op(OP_READ);
    TEST_ASSERT_EQUAL_UINT8(op->cs, cs_pin);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg_addr);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (op->ret == XY_OK) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

void xy_delay_ms(uint32_t ms)
{
    g_delay_total += ms;
}

void setUp(void)
{
    memset(g_ops, 0, sizeof(g_ops));
    memset(g_seen_writes, 0, sizeof(g_seen_writes));
    g_op_count = 0;
    g_op_index = 0;
    g_seen_write_count = 0;
    g_delay_total = 0;
}

void tearDown(void)
{
}

static xy_bmi088_config_t custom_config(void)
{
    xy_bmi088_config_t cfg = {
        .acc_range = XY_BMI088_ACC_RANGE_12G,
        .gyro_range = XY_BMI088_GYRO_RANGE_500,
        .acc_odr = XY_BMI088_ACC_ODR_400HZ,
        .gyro_odr = XY_BMI088_GYRO_ODR_400HZ,
        .bw = XY_BMI088_BW_OSR2,
        .enable_interrupt = true,
    };
    return cfg;
}

static void queue_init_sequence(const xy_bmi088_config_t *cfg)
{
    queue_write(0U, BMI088_ACC_SOFTRESET_ADDR, 0xB6U, XY_OK);
    queue_write(1U, BMI088_GYRO_SOFTRESET_ADDR, 0xB6U, XY_OK);
    queue_read8(0U, BMI088_ACC_CHIP_ID, BMI088_ACC_CHIP_ID_VALUE, XY_OK);
    queue_read8(1U, BMI088_GYRO_CHIP_ID, BMI088_GYRO_CHIP_ID_VALUE, XY_OK);
    queue_write(0U, BMI088_ACC_RANGE_ADDR, (uint8_t)cfg->acc_range, XY_OK);
    queue_write(0U, BMI088_ACC_BW_ADDR, (uint8_t)cfg->acc_odr, XY_OK);
    queue_write(0U, BMI088_ACC_PWR_CONF_ADDR, 0x00U, XY_OK);
    queue_write(0U, BMI088_ACC_PWR_CTRL_ADDR, 0x04U, XY_OK);
    queue_write(1U, BMI088_GYRO_RANGE_ADDR, (uint8_t)cfg->gyro_range, XY_OK);
    queue_write(1U, BMI088_GYRO_BANDWIDTH_ADDR, (uint8_t)cfg->gyro_odr, XY_OK);
    queue_write(1U, BMI088_GYRO_LPM1_ADDR, 0x80U, XY_OK);
}

static void init_bmi_ok(xy_bmi088_dev_t *dev, xy_spi_dev_t *spi)
{
    xy_bmi088_config_t cfg = custom_config();
    queue_init_sequence(&cfg);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_bmi088_init(dev, spi, &cfg));
}

static void test_bmi088_init_defaults_custom_and_invalid_paths(void)
{
    xy_bmi088_dev_t dev;
    xy_spi_dev_t spi = {0};
    xy_bmi088_config_t cfg = custom_config();

    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_init(NULL, &spi, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_init(&dev, NULL, &cfg));
    TEST_ASSERT_FALSE(xy_bmi088_is_ready(NULL));

    queue_init_sequence(&cfg);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_bmi088_init(&dev, &spi, &cfg));
    TEST_ASSERT_TRUE(xy_bmi088_is_ready(&dev));
    TEST_ASSERT_EQUAL_PTR(&spi, dev.spi);
    TEST_ASSERT_EQUAL_INT(XY_BMI088_ACC_RANGE_12G, dev.config.acc_range);
    TEST_ASSERT_EQUAL_INT(XY_BMI088_GYRO_RANGE_500, dev.config.gyro_range);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2730.0f, dev.acc_sensitivity);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 65.6f, dev.gyro_sensitivity);
    TEST_ASSERT_EQUAL_UINT32(95U, g_delay_total);

    queue_write(0U, BMI088_ACC_PWR_CTRL_ADDR, 0x00U, XY_OK);
    queue_write(1U, BMI088_GYRO_LPM1_ADDR, 0x03U, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_bmi088_deinit(&dev));
    TEST_ASSERT_FALSE(dev.is_initialized);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_deinit(NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_deinit(&dev));
}

static void test_bmi088_default_config_and_init_failures(void)
{
    xy_bmi088_dev_t dev;
    xy_spi_dev_t spi = {0};
    xy_bmi088_config_t defaults = {
        .acc_range = XY_BMI088_ACC_RANGE_6G,
        .gyro_range = XY_BMI088_GYRO_RANGE_2000,
        .acc_odr = XY_BMI088_ACC_ODR_200HZ,
        .gyro_odr = XY_BMI088_GYRO_ODR_200HZ,
        .bw = XY_BMI088_BW_NORMAL,
        .enable_interrupt = false,
    };

    queue_init_sequence(&defaults);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_bmi088_init(&dev, &spi, NULL));
    TEST_ASSERT_EQUAL_INT(XY_BMI088_ACC_RANGE_6G, dev.config.acc_range);
    TEST_ASSERT_EQUAL_INT(XY_BMI088_GYRO_RANGE_2000, dev.config.gyro_range);

    queue_write(0U, BMI088_ACC_SOFTRESET_ADDR, 0xB6U, XY_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_init(&dev, &spi, NULL));

    queue_write(0U, BMI088_ACC_SOFTRESET_ADDR, 0xB6U, XY_OK);
    queue_write(1U, BMI088_GYRO_SOFTRESET_ADDR, 0xB6U, XY_OK);
    queue_read8(0U, BMI088_ACC_CHIP_ID, 0x00U, XY_OK);
    queue_read8(1U, BMI088_GYRO_CHIP_ID, BMI088_GYRO_CHIP_ID_VALUE, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_init(&dev, &spi, NULL));

    queue_write(0U, BMI088_ACC_SOFTRESET_ADDR, 0xB6U, XY_OK);
    queue_write(1U, BMI088_GYRO_SOFTRESET_ADDR, 0xB6U, XY_OK);
    queue_read8(0U, BMI088_ACC_CHIP_ID, BMI088_ACC_CHIP_ID_VALUE, XY_OK);
    queue_read8(1U, BMI088_GYRO_CHIP_ID, 0x00U, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_init(&dev, &spi, NULL));
}

static void test_bmi088_read_raw_and_data_conversions(void)
{
    xy_bmi088_dev_t dev;
    xy_spi_dev_t spi = {0};
    xy_bmi088_raw_data_t raw;
    xy_bmi088_data_t data;

    init_bmi_ok(&dev, &spi);

    queue_read_xyz(0U, BMI088_ACC_X_LSB_ADDR, 2730, -2730, 1365, XY_OK);
    queue_read_xyz(1U, BMI088_GYRO_X_LSB_ADDR, 656, -656, 328, XY_OK);
    { uint8_t temp[2] = {0x80U, 0x02U}; queue_read(0U, BMI088_ACC_TEMP_LSB_ADDR, temp, 2U, XY_OK); }
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_bmi088_read_raw_data(&dev, &raw));
    TEST_ASSERT_EQUAL_INT16(2730, raw.acc_x);
    TEST_ASSERT_EQUAL_INT16(-2730, raw.acc_y);
    TEST_ASSERT_EQUAL_INT16(656, raw.gyro_x);
    TEST_ASSERT_EQUAL_INT16(0x0280, raw.temperature);

    dev.acc_offset[0] = 273.0f;
    dev.acc_offset[1] = -273.0f;
    dev.gyro_offset[0] = 65.6f;
    dev.gyro_offset[1] = -65.6f;
    queue_read_xyz(0U, BMI088_ACC_X_LSB_ADDR, 3003, -3003, 1365, XY_OK);
    queue_read_xyz(1U, BMI088_GYRO_X_LSB_ADDR, 722, -722, 328, XY_OK);
    { uint8_t temp[2] = {0x00U, 0x01U}; queue_read(0U, BMI088_ACC_TEMP_LSB_ADDR, temp, 2U, XY_OK); }
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_bmi088_read_data(&dev, &data));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 9.80665f, data.acc_x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -9.80665f, data.acc_y);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 4.903325f, data.acc_z);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.1746394f, data.gyro_x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -0.1746394f, data.gyro_y);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0872664f, data.gyro_z);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, data.temperature);
}

static void test_bmi088_error_paths_setters_and_calibration(void)
{
    xy_bmi088_dev_t dev = {0};
    xy_spi_dev_t spi = {0};
    xy_bmi088_raw_data_t raw;
    xy_bmi088_data_t data;

    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_read_chip_id(NULL, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_read_chip_id(&dev, NULL, NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_read_raw_data(NULL, &raw));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_read_raw_data(&dev, &raw));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_read_data(&dev, &data));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_set_acc_range(&dev, XY_BMI088_ACC_RANGE_3G));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_set_gyro_range(&dev, XY_BMI088_GYRO_RANGE_125));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_calibrate(&dev, 1));

    init_bmi_ok(&dev, &spi);

    raw.acc_x = 11;
    raw.acc_y = 22;
    raw.acc_z = 33;
    raw.gyro_x = 44;
    raw.gyro_y = 55;
    raw.gyro_z = 66;
    raw.temperature = 77;
    queue_read_xyz(0U, BMI088_ACC_X_LSB_ADDR, 0, 0, 0, XY_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_read_raw_data(&dev, &raw));
    TEST_ASSERT_EQUAL_INT16(11, raw.acc_x);
    TEST_ASSERT_EQUAL_INT16(77, raw.temperature);

    queue_read_xyz(0U, BMI088_ACC_X_LSB_ADDR, 100, 200, 300, XY_OK);
    queue_read_xyz(1U, BMI088_GYRO_X_LSB_ADDR, 10, 20, 30, XY_OK);
    { uint8_t temp[2] = {0U, 0U}; queue_read(0U, BMI088_ACC_TEMP_LSB_ADDR, temp, 2U, XY_ERROR); }
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_bmi088_read_raw_data(&dev, &raw));
    TEST_ASSERT_EQUAL_INT16(100, raw.acc_x);
    TEST_ASSERT_EQUAL_INT16(10, raw.gyro_x);
    TEST_ASSERT_EQUAL_INT16(77, raw.temperature);

    data.acc_x = 1.0f;
    data.gyro_x = 2.0f;
    data.temperature = 3.0f;
    queue_read_xyz(0U, BMI088_ACC_X_LSB_ADDR, 0, 0, 0, XY_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_read_data(&dev, &data));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, data.acc_x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, data.gyro_x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, data.temperature);

    queue_write(0U, BMI088_ACC_RANGE_ADDR, (uint8_t)XY_BMI088_ACC_RANGE_24G, XY_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_set_acc_range(&dev, XY_BMI088_ACC_RANGE_24G));
    TEST_ASSERT_EQUAL_INT(XY_BMI088_ACC_RANGE_12G, dev.config.acc_range);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2730.0f, dev.acc_sensitivity);

    queue_write(1U, BMI088_GYRO_RANGE_ADDR, (uint8_t)XY_BMI088_GYRO_RANGE_125, XY_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_bmi088_set_gyro_range(&dev, XY_BMI088_GYRO_RANGE_125));
    TEST_ASSERT_EQUAL_INT(XY_BMI088_GYRO_RANGE_500, dev.config.gyro_range);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 65.6f, dev.gyro_sensitivity);

    queue_write(0U, BMI088_ACC_RANGE_ADDR, (uint8_t)XY_BMI088_ACC_RANGE_24G, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_bmi088_set_acc_range(&dev, XY_BMI088_ACC_RANGE_24G));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1365.0f, dev.acc_sensitivity);
    queue_write(1U, BMI088_GYRO_RANGE_ADDR, (uint8_t)XY_BMI088_GYRO_RANGE_125, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_bmi088_set_gyro_range(&dev, XY_BMI088_GYRO_RANGE_125));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 262.4f, dev.gyro_sensitivity);

    queue_read_xyz(0U, BMI088_ACC_X_LSB_ADDR, 100, 200, 1365, XY_OK);
    queue_read_xyz(1U, BMI088_GYRO_X_LSB_ADDR, 10, 20, 30, XY_OK);
    { uint8_t temp[2] = {0U, 0U}; queue_read(0U, BMI088_ACC_TEMP_LSB_ADDR, temp, 2U, XY_OK); }
    queue_read_xyz(0U, BMI088_ACC_X_LSB_ADDR, 300, 400, 1365, XY_OK);
    queue_read_xyz(1U, BMI088_GYRO_X_LSB_ADDR, 30, 40, 50, XY_OK);
    { uint8_t temp[2] = {0U, 0U}; queue_read(0U, BMI088_ACC_TEMP_LSB_ADDR, temp, 2U, XY_OK); }
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_bmi088_calibrate(&dev, 2));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 200.0f, dev.acc_offset[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 300.0f, dev.acc_offset[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dev.acc_offset[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, dev.gyro_offset[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, dev.gyro_offset[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 40.0f, dev.gyro_offset[2]);
}

static void test_bmi088_set_calibration_and_inline_helpers(void)
{
    xy_bmi088_dev_t dev = {0};
    float acc[3] = {1.0f, 2.0f, 3.0f};
    float gyro[3] = {4.0f, 5.0f, 6.0f};

    xy_bmi088_set_calibration(NULL, acc, gyro);
    xy_bmi088_set_calibration(&dev, acc, gyro);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, dev.acc_offset[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 6.0f, dev.gyro_offset[2]);
    xy_bmi088_set_calibration(&dev, NULL, NULL);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, dev.acc_offset[0]);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 9.80665f, xy_bmi088_acc_raw_to_ms2(100, 100.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0174533f, xy_bmi088_gyro_raw_to_rads(100, 100.0f));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bmi088_init_defaults_custom_and_invalid_paths);
    RUN_TEST(test_bmi088_default_config_and_init_failures);
    RUN_TEST(test_bmi088_read_raw_and_data_conversions);
    RUN_TEST(test_bmi088_error_paths_setters_and_calibration);
    RUN_TEST(test_bmi088_set_calibration_and_inline_helpers);
    return UNITY_END();
}
