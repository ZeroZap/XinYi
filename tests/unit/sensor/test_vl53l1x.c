#include "unity.h"

#include "xy_vl53l1x.h"

#include <string.h>

#ifndef XY_OK
#define XY_OK 0
#endif
#ifndef XY_ERROR
#define XY_ERROR -1
#endif
#ifndef XY_TIMEOUT
#define XY_TIMEOUT -2
#endif

typedef enum {
    OP_READ,
    OP_WRITE,
} op_kind_t;

typedef struct {
    op_kind_t kind;
    uint16_t reg;
    uint8_t data[12];
    uint16_t len;
    xy_ret_t ret;
} i2c_op_t;

static i2c_op_t g_ops[96];
static unsigned g_op_count;
static unsigned g_op_index;
static uint32_t g_delay_total_ms;
static unsigned g_delay_calls;

static void expect_read_ret(uint16_t reg, const uint8_t *data, uint16_t len, xy_ret_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_ops) / sizeof(g_ops[0]), g_op_count);
    g_ops[g_op_count].kind = OP_READ;
    g_ops[g_op_count].reg = reg;
    g_ops[g_op_count].len = len;
    g_ops[g_op_count].ret = ret;
    if (data != NULL && len > 0U) {
        memcpy(g_ops[g_op_count].data, data, len);
    }
    g_op_count++;
}

static void expect_read(uint16_t reg, const uint8_t *data, uint16_t len)
{
    expect_read_ret(reg, data, len, XY_OK);
}

static void expect_write_ret(uint16_t reg, const uint8_t *data, uint16_t len, xy_ret_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_ops) / sizeof(g_ops[0]), g_op_count);
    g_ops[g_op_count].kind = OP_WRITE;
    g_ops[g_op_count].reg = reg;
    g_ops[g_op_count].len = len;
    g_ops[g_op_count].ret = ret;
    if (data != NULL && len > 0U) {
        memcpy(g_ops[g_op_count].data, data, len);
    }
    g_op_count++;
}

static void expect_write(uint16_t reg, const uint8_t *data, uint16_t len)
{
    expect_write_ret(reg, data, len, XY_OK);
}

static void expect_write_u8(uint16_t reg, uint8_t value)
{
    expect_write(reg, &value, 1);
}

static void expect_write_u16(uint16_t reg, uint16_t value)
{
    uint8_t data[2] = {(uint8_t)(value >> 8), (uint8_t)value};
    expect_write(reg, data, 2);
}

static void expect_write_u32(uint16_t reg, uint32_t value)
{
    uint8_t data[4] = {
        (uint8_t)(value >> 24),
        (uint8_t)(value >> 16),
        (uint8_t)(value >> 8),
        (uint8_t)value,
    };
    expect_write(reg, data, 4);
}

void setUp(void)
{
    memset(g_ops, 0, sizeof(g_ops));
    g_op_count = 0;
    g_op_index = 0;
    g_delay_total_ms = 0;
    g_delay_calls = 0;
}

void tearDown(void)
{
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);
}

xy_ret_t xy_i2c_write_reg16(xy_i2c_dev_t *dev, uint16_t reg_addr, const uint8_t *data, uint16_t len)
{
    (void)dev;
    TEST_ASSERT_LESS_THAN_UINT(g_op_count, g_op_index);
    i2c_op_t *op = &g_ops[g_op_index++];
    TEST_ASSERT_EQUAL_INT(OP_WRITE, op->kind);
    TEST_ASSERT_EQUAL_HEX16(op->reg, reg_addr);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    return op->ret;
}

xy_ret_t xy_i2c_read_reg16(xy_i2c_dev_t *dev, uint16_t reg_addr, uint8_t *data, uint16_t len)
{
    (void)dev;
    TEST_ASSERT_LESS_THAN_UINT(g_op_count, g_op_index);
    i2c_op_t *op = &g_ops[g_op_index++];
    TEST_ASSERT_EQUAL_INT(OP_READ, op->kind);
    TEST_ASSERT_EQUAL_HEX16(op->reg, reg_addr);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (op->ret == XY_OK) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

void xy_delay_ms(uint32_t ms)
{
    g_delay_calls++;
    g_delay_total_ms += ms;
}

static xy_vl53l1x_dev_t make_ready_dev(xy_i2c_dev_t *i2c)
{
    xy_vl53l1x_dev_t dev;
    memset(&dev, 0, sizeof(dev));
    dev.i2c = i2c;
    dev.is_initialized = true;
    dev.config.range = XY_VL53L1X_RANGE_MEDIUM;
    dev.config.timing = XY_VL53L1X_TIMING_33MS;
    dev.config.roi.centre_spad = 199;
    dev.config.roi.width = 16;
    dev.config.roi.height = 16;
    return dev;
}

static void expect_default_init_sequence(void)
{
    const uint8_t model = 0xEA;
    const uint8_t module = 0xCC;
    const uint8_t revision[2] = {0x01, 0x02};
    expect_read(0x010F, &model, 1);
    expect_read(0x0110, &module, 1);
    expect_read(0x0112, revision, 2);
    expect_write_u8(VL53L1X_SOFTWARE_RESET, 0x00);
    expect_read(0x0000, &(const uint8_t){0x00}, 1);
    expect_write_u8(0x0060, 0x0B);
    expect_write_u8(0x0070, 0x09);
    expect_write_u8(0x0064, 0x09);
    expect_write_u8(0x0074, 0x07);
    expect_write_u16(0x0008, 33000U);
    expect_write_u8(0x0016, 199U);
    expect_write_u8(0x0017, 0x00U);
}

void test_init_applies_default_configuration_and_device_info(void)
{
    xy_vl53l1x_dev_t dev;
    xy_i2c_dev_t i2c = {.address = VL53L1X_I2C_ADDR};
    expect_default_init_sequence();

    TEST_ASSERT_EQUAL_INT(XY_OK, xy_vl53l1x_init(&dev, &i2c, NULL));
    TEST_ASSERT_TRUE(dev.is_initialized);
    TEST_ASSERT_EQUAL_UINT8(0xEA, dev.model_id);
    TEST_ASSERT_EQUAL_UINT8(0xCC, dev.module_type);
    TEST_ASSERT_EQUAL_UINT16(0x0102, dev.revision_id);
    TEST_ASSERT_EQUAL_INT(XY_VL53L1X_RANGE_MEDIUM, dev.config.range);
    TEST_ASSERT_EQUAL_INT(XY_VL53L1X_TIMING_33MS, dev.config.timing);
    TEST_ASSERT_EQUAL_UINT32(70U, g_delay_total_ms);
}

void test_init_rejects_wrong_model_id(void)
{
    xy_vl53l1x_dev_t dev;
    xy_i2c_dev_t i2c = {.address = VL53L1X_I2C_ADDR};
    const uint8_t model = 0xAB;
    const uint8_t module = 0xCC;
    const uint8_t revision[2] = {0x01, 0x02};
    expect_read(0x010F, &model, 1);
    expect_read(0x0110, &module, 1);
    expect_read(0x0112, revision, 2);

    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_init(&dev, &i2c, NULL));
}

void test_start_stop_and_continuous_period_write_expected_commands(void)
{
    xy_i2c_dev_t i2c = {.address = VL53L1X_I2C_ADDR};
    xy_vl53l1x_dev_t dev = make_ready_dev(&i2c);

    expect_write_u8(VL53L1X_SYSTEM_START, 0x00);
    expect_write_u8(VL53L1X_SYSTEM_START, 0x10);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_vl53l1x_start_single(&dev));

    expect_write_u8(VL53L1X_SYSTEM_START, 0x00);
    expect_write_u32(VL53L1X_SYSTEM_INTERMEASUREMENT_PERIOD, 20000U);
    expect_write_u8(VL53L1X_SYSTEM_START, 0x02);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_vl53l1x_start_continuous(&dev, 20));
}


void test_start_commands_propagate_start_write_failures_after_stop(void)
{
    xy_i2c_dev_t i2c = {.address = VL53L1X_I2C_ADDR};
    xy_vl53l1x_dev_t dev = make_ready_dev(&i2c);

    expect_write_u8(VL53L1X_SYSTEM_START, 0x00);
    expect_write_ret(VL53L1X_SYSTEM_START, &(const uint8_t){0x10}, 1, XY_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_start_single(&dev));

    expect_write_u8(VL53L1X_SYSTEM_START, 0x00);
    expect_write_u32(VL53L1X_SYSTEM_INTERMEASUREMENT_PERIOD, 20000U);
    expect_write_ret(VL53L1X_SYSTEM_START, &(const uint8_t){0x02}, 1, XY_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_start_continuous(&dev, 20));
}

void test_data_ready_and_result_parsing_with_offset(void)
{
    xy_i2c_dev_t i2c = {.address = VL53L1X_I2C_ADDR};
    xy_vl53l1x_dev_t dev = make_ready_dev(&i2c);
    bool ready = false;
    uint8_t ready_status = 0x04;
    uint8_t result_bytes[10] = {
        XY_VL53L1X_STATUS_VALID, 9,
        0x12, 0x34,
        0, 0, 0, 0,
        0x04, 0xD2,
    };
    xy_vl53l1x_result_t result;

    xy_vl53l1x_set_offset(&dev, 10);
    expect_read(VL53L1X_RESULT_INTERRUPT_STATUS, &ready_status, 1);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_vl53l1x_check_data_ready(&dev, &ready));
    TEST_ASSERT_TRUE(ready);

    expect_read_ret(VL53L1X_RESULT_INTERRUPT_STATUS, NULL, 1, XY_ERROR);
    ready = true;
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_check_data_ready(&dev, &ready));
    TEST_ASSERT_FALSE(ready);

    expect_read(VL53L1X_RESULT_RANGE_STATUS, result_bytes, sizeof(result_bytes));
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_vl53l1x_read_result(&dev, &result));
    TEST_ASSERT_EQUAL_UINT8(XY_VL53L1X_STATUS_VALID, result.status);
    TEST_ASSERT_EQUAL_UINT8(9, result.spad_count);
    TEST_ASSERT_EQUAL_UINT16(0x1234, result.signal_rate);
    TEST_ASSERT_EQUAL_UINT16(1224, result.distance);
    TEST_ASSERT_EQUAL_UINT32(1, dev.measurement_count);
    TEST_ASSERT_EQUAL_UINT16(1224, xy_vl53l1x_get_last_result(&dev)->distance);

    expect_read_ret(VL53L1X_RESULT_RANGE_STATUS, NULL, sizeof(result_bytes), XY_ERROR);
    result.distance = 0xEEEEU;
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_read_result(&dev, &result));
    TEST_ASSERT_EQUAL_UINT16(0xEEEEU, result.distance);
    TEST_ASSERT_EQUAL_UINT32(1, dev.measurement_count);
    TEST_ASSERT_EQUAL_UINT16(1224, xy_vl53l1x_get_last_result(&dev)->distance);
}

void test_measure_timeout_stops_sensor(void)
{
    xy_i2c_dev_t i2c = {.address = VL53L1X_I2C_ADDR};
    xy_vl53l1x_dev_t dev = make_ready_dev(&i2c);
    xy_vl53l1x_result_t result;
    const uint8_t not_ready = 0x00;

    expect_write_u8(VL53L1X_SYSTEM_START, 0x00);
    expect_write_u8(VL53L1X_SYSTEM_START, 0x10);
    expect_read(VL53L1X_RESULT_INTERRUPT_STATUS, &not_ready, 1);
    expect_read(VL53L1X_RESULT_INTERRUPT_STATUS, &not_ready, 1);
    expect_write_u8(VL53L1X_SYSTEM_START, 0x00);

    TEST_ASSERT_EQUAL_INT(XY_TIMEOUT, xy_vl53l1x_measure(&dev, &result, 10));
}

void test_measure_success_reads_result_and_clears_interrupt(void)
{
    xy_i2c_dev_t i2c = {.address = VL53L1X_I2C_ADDR};
    xy_vl53l1x_dev_t dev = make_ready_dev(&i2c);
    xy_vl53l1x_result_t result;
    const uint8_t ready = 0x04;
    uint8_t result_bytes[10] = {XY_VL53L1X_STATUS_VALID, 4, 0x00, 0x64, 0, 0, 0, 0, 0x00, 0xFA};

    expect_write_u8(VL53L1X_SYSTEM_START, 0x00);
    expect_write_u8(VL53L1X_SYSTEM_START, 0x10);
    expect_read(VL53L1X_RESULT_INTERRUPT_STATUS, &ready, 1);
    expect_read(VL53L1X_RESULT_RANGE_STATUS, result_bytes, sizeof(result_bytes));
    expect_write_u8(VL53L1X_SYSTEM_INTERRUPT_CLEAR, 0x01);

    TEST_ASSERT_EQUAL_INT(XY_OK, xy_vl53l1x_measure(&dev, &result, 20));
    TEST_ASSERT_EQUAL_UINT16(250, result.distance);
}

void test_measure_read_or_clear_failures_preserve_public_contracts(void)
{
    xy_i2c_dev_t i2c = {.address = VL53L1X_I2C_ADDR};
    xy_vl53l1x_dev_t dev = make_ready_dev(&i2c);
    xy_vl53l1x_result_t result = {.distance = 0xEEEEU};
    const uint8_t ready = 0x04;
    uint8_t result_bytes[10] = {XY_VL53L1X_STATUS_VALID, 4, 0x00, 0x64, 0, 0, 0, 0, 0x00, 0xFA};

    dev.last_result.distance = 123U;
    dev.measurement_count = 7U;
    expect_write_u8(VL53L1X_SYSTEM_START, 0x00);
    expect_write_u8(VL53L1X_SYSTEM_START, 0x10);
    expect_read(VL53L1X_RESULT_INTERRUPT_STATUS, &ready, 1);
    expect_read_ret(VL53L1X_RESULT_RANGE_STATUS, NULL, sizeof(result_bytes), XY_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_measure(&dev, &result, 20));
    TEST_ASSERT_EQUAL_UINT16(0xEEEEU, result.distance);
    TEST_ASSERT_EQUAL_UINT16(123U, dev.last_result.distance);
    TEST_ASSERT_EQUAL_UINT32(7U, dev.measurement_count);

    setUp();
    result.distance = 0U;
    expect_write_u8(VL53L1X_SYSTEM_START, 0x00);
    expect_write_u8(VL53L1X_SYSTEM_START, 0x10);
    expect_read(VL53L1X_RESULT_INTERRUPT_STATUS, &ready, 1);
    expect_read(VL53L1X_RESULT_RANGE_STATUS, result_bytes, sizeof(result_bytes));
    expect_write_ret(VL53L1X_SYSTEM_INTERRUPT_CLEAR, &(const uint8_t){0x01}, 1, XY_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_vl53l1x_measure(&dev, &result, 20));
    TEST_ASSERT_EQUAL_UINT16(250U, result.distance);
}

void test_range_roi_interrupt_and_address_configuration(void)
{
    xy_i2c_dev_t i2c = {.address = VL53L1X_I2C_ADDR};
    xy_vl53l1x_dev_t dev = make_ready_dev(&i2c);
    xy_vl53l1x_roi_t roi = {.centre_spad = 42, .width = 8, .height = 6};

    expect_write_u8(0x0060, 0x0F);
    expect_write_u8(0x0070, 0x0D);
    expect_write_u8(0x0064, 0x0B);
    expect_write_u8(0x0074, 0x09);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_vl53l1x_set_range(&dev, XY_VL53L1X_RANGE_LONG));

    expect_write_u8(0x0016, 42);
    expect_write_u8(0x0017, 0x86);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_vl53l1x_set_roi(&dev, &roi));

    expect_write_u8(VL53L1X_SYSTEM_INTERRUPT_CONFIG_GPIO, XY_VL53L1X_INT_OUT_OF_WINDOW);
    expect_write_u16(VL53L1X_SYSTEM_THRESH_RATE_LOW, 100);
    expect_write_u16(VL53L1X_SYSTEM_THRESH_RATE_HIGH, 900);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_vl53l1x_configure_interrupt(&dev, XY_VL53L1X_INT_OUT_OF_WINDOW, 100, 900));

    expect_write_u8(0x0001, 0x60);
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_vl53l1x_change_i2c_address(&dev, 0x30));
    TEST_ASSERT_EQUAL_UINT8(0x30, i2c.address);
}


void test_configuration_write_failures_stop_at_first_failed_register(void)
{
    xy_i2c_dev_t i2c = {.address = VL53L1X_I2C_ADDR};
    xy_vl53l1x_dev_t dev = make_ready_dev(&i2c);
    xy_vl53l1x_roi_t roi = {.centre_spad = 42, .width = 8, .height = 6};

    expect_write_ret(0x0060, &(const uint8_t){0x0F}, 1, XY_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_set_range(&dev, XY_VL53L1X_RANGE_LONG));

    expect_write_ret(0x0016, &(const uint8_t){42}, 1, XY_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_set_roi(&dev, &roi));

    expect_write_ret(VL53L1X_SYSTEM_INTERRUPT_CONFIG_GPIO,
                     &(const uint8_t){XY_VL53L1X_INT_OUT_OF_WINDOW}, 1, XY_ERROR);
    TEST_ASSERT_EQUAL_INT(
        XY_ERROR,
        xy_vl53l1x_configure_interrupt(&dev, XY_VL53L1X_INT_OUT_OF_WINDOW, 100, 900));
}

void test_calibrate_offset_clamps_sample_count_and_averages_valid_measurements(void)
{
    xy_i2c_dev_t i2c = {.address = VL53L1X_I2C_ADDR};
    xy_vl53l1x_dev_t dev = make_ready_dev(&i2c);
    const uint8_t ready = 0x04;
    uint8_t result_bytes[10] = {XY_VL53L1X_STATUS_VALID, 1, 0, 1, 0, 0, 0, 0, 0x00, 0x78};

    for (unsigned i = 0; i < 10; ++i) {
        expect_write_u8(VL53L1X_SYSTEM_START, 0x00);
        expect_write_u8(VL53L1X_SYSTEM_START, 0x10);
        expect_read(VL53L1X_RESULT_INTERRUPT_STATUS, &ready, 1);
        expect_read(VL53L1X_RESULT_RANGE_STATUS, result_bytes, sizeof(result_bytes));
        expect_write_u8(VL53L1X_SYSTEM_INTERRUPT_CLEAR, 0x01);
    }

    TEST_ASSERT_EQUAL_INT(XY_OK, xy_vl53l1x_calibrate_offset(&dev, 100, 2));
    TEST_ASSERT_EQUAL_UINT16(20, dev.offset);
}

void test_calibrate_offset_ignores_invalid_measurements_and_reports_no_valid_samples(void)
{
    xy_i2c_dev_t i2c = {.address = VL53L1X_I2C_ADDR};
    xy_vl53l1x_dev_t dev = make_ready_dev(&i2c);
    const uint8_t ready = 0x04;
    uint8_t invalid_result[10] = {XY_VL53L1X_STATUS_RANGE_FAIL,
                                  1,
                                  0,
                                  1,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0x00,
                                  0x78};
    uint8_t valid_result[10] = {XY_VL53L1X_STATUS_VALID, 1, 0, 1, 0, 0, 0, 0, 0x00, 0x96};

    for (unsigned i = 0; i < 9; ++i) {
        expect_write_u8(VL53L1X_SYSTEM_START, 0x00);
        expect_write_u8(VL53L1X_SYSTEM_START, 0x10);
        expect_read(VL53L1X_RESULT_INTERRUPT_STATUS, &ready, 1);
        expect_read(VL53L1X_RESULT_RANGE_STATUS, invalid_result, sizeof(invalid_result));
        expect_write_u8(VL53L1X_SYSTEM_INTERRUPT_CLEAR, 0x01);
    }
    expect_write_u8(VL53L1X_SYSTEM_START, 0x00);
    expect_write_u8(VL53L1X_SYSTEM_START, 0x10);
    expect_read(VL53L1X_RESULT_INTERRUPT_STATUS, &ready, 1);
    expect_read(VL53L1X_RESULT_RANGE_STATUS, valid_result, sizeof(valid_result));
    expect_write_u8(VL53L1X_SYSTEM_INTERRUPT_CLEAR, 0x01);

    TEST_ASSERT_EQUAL_INT(XY_OK, xy_vl53l1x_calibrate_offset(&dev, 100, 1));
    TEST_ASSERT_EQUAL_UINT16(50U, dev.offset);

    setUp();
    dev.offset = 123U;
    for (unsigned i = 0; i < 10; ++i) {
        expect_write_u8(VL53L1X_SYSTEM_START, 0x00);
        expect_write_u8(VL53L1X_SYSTEM_START, 0x10);
        expect_read(VL53L1X_RESULT_INTERRUPT_STATUS, &ready, 1);
        expect_read(VL53L1X_RESULT_RANGE_STATUS, invalid_result, sizeof(invalid_result));
        expect_write_u8(VL53L1X_SYSTEM_INTERRUPT_CLEAR, 0x01);
    }

    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_calibrate_offset(&dev, 100, 1));
    TEST_ASSERT_EQUAL_UINT16(123U, dev.offset);
}

void test_public_guards_and_inline_helpers(void)
{
    xy_i2c_dev_t i2c = {.address = VL53L1X_I2C_ADDR};
    xy_vl53l1x_dev_t dev = make_ready_dev(&i2c);

    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_init(NULL, &i2c, NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_start_single(NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_start_continuous(NULL, 10U));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_stop(NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_check_data_ready(NULL, &(bool){false}));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_check_data_ready(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_read_result(NULL, &(xy_vl53l1x_result_t){0}));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_read_result(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_measure(NULL, &(xy_vl53l1x_result_t){0}, 1U));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_measure(&dev, NULL, 1U));
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_vl53l1x_set_roi(&dev, NULL));
    TEST_ASSERT_TRUE(xy_vl53l1x_is_ready(&dev));
    TEST_ASSERT_FALSE(xy_vl53l1x_is_ready(NULL));
    TEST_ASSERT_TRUE(xy_vl53l1x_is_status_valid(XY_VL53L1X_STATUS_VALID));
    TEST_ASSERT_FALSE(xy_vl53l1x_is_status_valid(XY_VL53L1X_STATUS_TIMEOUT));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.3f, xy_vl53l1x_mm_to_cm(123));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.234f, xy_vl53l1x_mm_to_m(1234));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_applies_default_configuration_and_device_info);
    RUN_TEST(test_init_rejects_wrong_model_id);
    RUN_TEST(test_start_stop_and_continuous_period_write_expected_commands);
    RUN_TEST(test_start_commands_propagate_start_write_failures_after_stop);
    RUN_TEST(test_data_ready_and_result_parsing_with_offset);
    RUN_TEST(test_measure_timeout_stops_sensor);
    RUN_TEST(test_measure_success_reads_result_and_clears_interrupt);
    RUN_TEST(test_measure_read_or_clear_failures_preserve_public_contracts);
    RUN_TEST(test_range_roi_interrupt_and_address_configuration);
    RUN_TEST(test_configuration_write_failures_stop_at_first_failed_register);
    RUN_TEST(test_calibrate_offset_clamps_sample_count_and_averages_valid_measurements);
    RUN_TEST(test_calibrate_offset_ignores_invalid_measurements_and_reports_no_valid_samples);
    RUN_TEST(test_public_guards_and_inline_helpers);
    return UNITY_END();
}
