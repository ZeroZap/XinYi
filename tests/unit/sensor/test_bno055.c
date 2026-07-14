#include "unity.h"

#include "xy_bno055.h"
#include "xy_device_error.h"
#include "xy_i2c.h"

#include <math.h>
#include <stdarg.h>
#include <string.h>

#ifndef XY_DEVICE_OK
#define XY_DEVICE_OK 0
#endif

typedef enum {
    OP_READ,
    OP_WRITE,
} op_kind_t;

typedef struct {
    op_kind_t kind;
    uint8_t reg;
    uint8_t data[48];
    uint16_t len;
    int ret;
} i2c_op_t;

static i2c_op_t g_ops[128];
static unsigned g_op_count;
static unsigned g_op_index;

static void expect_read_ret(uint8_t reg, const uint8_t *data, uint16_t len, int ret)
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

static void expect_read(uint8_t reg, const uint8_t *data, uint16_t len)
{
    expect_read_ret(reg, data, len, XY_DEVICE_OK);
}

static void expect_write_ret(uint8_t reg, const uint8_t *data, uint16_t len, int ret)
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

static void expect_write(uint8_t reg, const uint8_t *data, uint16_t len)
{
    expect_write_ret(reg, data, len, XY_DEVICE_OK);
}

static void expect_write_u8(uint8_t reg, uint8_t value)
{
    expect_write(reg, &value, 1);
}

void setUp(void)
{
    memset(g_ops, 0, sizeof(g_ops));
    g_op_count = 0;
    g_op_index = 0;
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

void tearDown(void)
{
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);
}

int xy_i2c_master_transmit(xy_i2c_t *i2c, uint8_t addr, const uint8_t *tx, uint16_t tx_len,
                           const void *data, uint16_t data_len, uint32_t timeout_ms)
{
    (void)i2c;
    (void)addr;
    (void)timeout_ms;
    TEST_ASSERT_EQUAL_UINT16(1, tx_len);
    TEST_ASSERT_NOT_NULL(tx);
    TEST_ASSERT_LESS_THAN_UINT(g_op_count, g_op_index);

    i2c_op_t *op = &g_ops[g_op_index++];
    TEST_ASSERT_EQUAL_UINT8(op->reg, tx[0]);
    TEST_ASSERT_EQUAL_UINT16(op->len, data_len);
    if (op->kind == OP_READ) {
        if (op->ret == XY_DEVICE_OK) {
            memcpy((void *)data, op->data, data_len);
        }
    } else {
        TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, data_len);
    }
    return op->ret;
}

static xy_bno055_t make_ready_dev(void *bus)
{
    xy_bno055_t dev;
    memset(&dev, 0, sizeof(dev));
    dev.bus_handle = bus;
    dev.bus_addr = 0x28;
    dev.mode = BNO055_MODE_NDOF;
    dev.initialized = true;
    return dev;
}

static void expect_init_success_sequence(void)
{
    const uint8_t chip = BNO055_CHIP_ID;
    const uint8_t sw[2] = {0x19, 0x03};
    const uint8_t mode_config = BNO055_MODE_CONFIG;
    const uint8_t sys_trigger = 0x00;
    const uint8_t units = BNO055_UNIT_DEG | BNO055_UNIT_CELSIUS | BNO055_UNIT_EULER | BNO055_UNIT_MS2 | BNO055_UNIT_UT;

    expect_write_u8(BNO055_REG_SYS_TRIGGER, 0x20);
    expect_read(BNO055_REG_CHIP_ID, &chip, 1);
    expect_read(BNO055_REG_SW_REV_ID_LSB, sw, 2);
    expect_read(BNO055_REG_OPR_MODE, &mode_config, 1);
    expect_write(BNO055_REG_SYS_TRIGGER, &sys_trigger, 1);
    expect_write(BNO055_REG_UNIT_SEL, &units, 1);
    expect_write_u8(BNO055_REG_OPR_MODE, BNO055_MODE_NDOF);
}

void test_init_configures_units_and_ndof_mode(void)
{
    xy_bno055_t dev;
    int bus;
    expect_init_success_sequence();

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_init(&dev, &bus, 0x28, false));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(0x28, dev.bus_addr);
    TEST_ASSERT_EQUAL_UINT16(BNO055_SW_REV_MIN, dev.sw_version);
    TEST_ASSERT_EQUAL_INT(BNO055_MODE_NDOF, dev.mode);
}

void test_init_rejects_wrong_chip_id(void)
{
    xy_bno055_t dev;
    int bus;
    const uint8_t bad_chip = 0x00;
    expect_write_u8(BNO055_REG_SYS_TRIGGER, 0x20);
    expect_read(BNO055_REG_CHIP_ID, &bad_chip, 1);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ENODEV, xy_bno055_init(&dev, &bus, 0x28, false));
}

void test_register_access_guards_and_i2c_round_trip(void)
{
    int bus;
    xy_bno055_t dev = make_ready_dev(&bus);
    uint8_t byte = 0;
    const uint8_t read_value = 0xA0;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bno055_read_regs(NULL, BNO055_REG_CHIP_ID, &byte, 1));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bno055_read_regs(&dev, BNO055_REG_CHIP_ID, NULL, 1));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bno055_read_regs(&dev, BNO055_REG_CHIP_ID, &byte, 0));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bno055_write_regs(&dev, BNO055_REG_PAGE_ID, NULL, 1));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_EINVAL, xy_bno055_write_regs(&dev, BNO055_REG_PAGE_ID, &byte, 0));

    expect_read(BNO055_REG_CHIP_ID, &read_value, 1);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_get_chip_id(&dev, &byte));
    TEST_ASSERT_EQUAL_UINT8(BNO055_CHIP_ID, byte);

    expect_read_ret(BNO055_REG_CHIP_ID, NULL, 1, XY_DEVICE_ERROR);
    byte = 0x5AU;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bno055_get_chip_id(&dev, &byte));
    TEST_ASSERT_EQUAL_UINT8(0x5AU, byte);

    byte = 0x02;
    expect_write(BNO055_REG_PAGE_ID, &byte, 1);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_write_regs(&dev, BNO055_REG_PAGE_ID, &byte, 1));
}

void test_mode_power_units_sleep_and_wakeup_write_expected_registers(void)
{
    int bus;
    xy_bno055_t dev = make_ready_dev(&bus);
    const uint8_t mode_ndof = BNO055_MODE_NDOF;

    expect_write_u8(BNO055_REG_OPR_MODE, BNO055_MODE_IMU);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_set_mode(&dev, BNO055_MODE_IMU));
    TEST_ASSERT_EQUAL_INT(BNO055_MODE_IMU, dev.mode);

    expect_write_u8(BNO055_REG_PWR_MODE, BNO055_PWR_LOWPOWER);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_set_power_mode(&dev, BNO055_PWR_LOWPOWER));

    expect_write_u8(BNO055_REG_UNIT_SEL, BNO055_UNIT_RAD | BNO055_UNIT_G | BNO055_UNIT_MG);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_set_units(&dev, BNO055_UNIT_RAD | BNO055_UNIT_G | BNO055_UNIT_MG));

    expect_write_u8(BNO055_REG_PWR_MODE, BNO055_PWR_SUSPEND);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_sleep(&dev));

    expect_write_u8(BNO055_REG_PWR_MODE, BNO055_PWR_NORMAL);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_wakeup(&dev));

    expect_read(BNO055_REG_OPR_MODE, &mode_ndof, 1);
    expect_write_u8(BNO055_REG_OPR_MODE, BNO055_MODE_CONFIG);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_set_mode(&dev, BNO055_MODE_CONFIG));
    TEST_ASSERT_EQUAL_INT(BNO055_MODE_CONFIG, dev.mode);

    const uint8_t pwr_normal = BNO055_PWR_NORMAL;
    expect_write_ret(BNO055_REG_PWR_MODE, &pwr_normal, 1, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bno055_set_power_mode(&dev, BNO055_PWR_NORMAL));
}

void test_euler_quaternion_calibration_and_raw_parsing(void)
{
    int bus;
    xy_bno055_t dev = make_ready_dev(&bus);
    bno055_euler_t euler;
    bno055_quaternion_t quat;
    bno055_calib_t calib;
    bno055_raw_data_t raw;
    uint8_t euler_bytes[6] = {0x20, 0x01, 0xE0, 0xFF, 0x40, 0x00};
    uint8_t quat_bytes[8] = {0x00, 0x40, 0x00, 0x20, 0x00, 0xE0, 0x00, 0x00};
    const uint8_t calib_byte = 0xE4;
    uint8_t raw_bytes[45];
    memset(raw_bytes, 0, sizeof(raw_bytes));
    raw_bytes[0] = 0x34; raw_bytes[1] = 0x12;
    raw_bytes[6] = 0xFE; raw_bytes[7] = 0xFF;
    raw_bytes[12] = 0x10; raw_bytes[13] = 0x00;
    raw_bytes[44] = 25;

    expect_read(BNO055_REG_EUL_HEADING_LSB, euler_bytes, sizeof(euler_bytes));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_get_euler(&dev, &euler));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 18.0f, euler.heading);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -2.0f, euler.roll);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, euler.pitch);

    expect_read(BNO055_REG_QUA_DATA_W_LSB, quat_bytes, sizeof(quat_bytes));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_get_quaternion(&dev, &quat));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, quat.w);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, quat.x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.5f, quat.y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, quat.z);

    expect_read(BNO055_REG_CALIB_STAT, &calib_byte, 1);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_get_calib_status(&dev, &calib));
    TEST_ASSERT_EQUAL_UINT8(3, calib.sys);
    TEST_ASSERT_EQUAL_UINT8(2, calib.gyro);
    TEST_ASSERT_EQUAL_UINT8(1, calib.acc);
    TEST_ASSERT_EQUAL_UINT8(0, calib.mag);

    expect_read(BNO055_REG_ACC_DATA_X_LSB, raw_bytes, 45);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_get_raw_data(&dev, &raw));
    TEST_ASSERT_EQUAL_INT16(0x1234, raw.acc_x);
    TEST_ASSERT_EQUAL_INT16(-2, raw.mag_x);
    TEST_ASSERT_EQUAL_INT16(16, raw.gyr_x);
    TEST_ASSERT_EQUAL_INT8(25, raw.temp);
}

void test_get_data_converts_fused_vectors(void)
{
    int bus;
    xy_bno055_t dev = make_ready_dev(&bus);
    bno055_data_t data;
    uint8_t euler_bytes[6] = {0x00, 0x01, 0x00, 0x00, 0xF0, 0xFF};
    uint8_t quat_bytes[8] = {0x00, 0x40, 0, 0, 0, 0, 0, 0};
    uint8_t acc_bytes[6] = {0x64, 0x00, 0x38, 0xFF, 0x00, 0x00};
    uint8_t lia_bytes[6] = {0xC8, 0x00, 0, 0, 0, 0};
    uint8_t grv_bytes[6] = {0, 0, 0, 0, 0xD2, 0x04};
    uint8_t mag_bytes[6] = {0x20, 0x00, 0xE0, 0xFF, 0, 0};
    uint8_t gyr_bytes[6] = {0x10, 0x00, 0xF0, 0xFF, 0, 0};
    const uint8_t temp = 23;
    const uint8_t calib = 0xFF;

    memset(&data, 0, sizeof(data));
    expect_read(BNO055_REG_EUL_HEADING_LSB, euler_bytes, sizeof(euler_bytes));
    expect_read(BNO055_REG_QUA_DATA_W_LSB, quat_bytes, sizeof(quat_bytes));
    expect_read(BNO055_REG_ACC_DATA_X_LSB, acc_bytes, sizeof(acc_bytes));
    expect_read(BNO055_REG_LIA_DATA_X_LSB, lia_bytes, sizeof(lia_bytes));
    expect_read(BNO055_REG_GRV_DATA_X_LSB, grv_bytes, sizeof(grv_bytes));
    expect_read(BNO055_REG_MAG_DATA_X_LSB, mag_bytes, sizeof(mag_bytes));
    expect_read(BNO055_REG_GYR_DATA_X_LSB, gyr_bytes, sizeof(gyr_bytes));
    expect_read(BNO055_REG_TEMP, &temp, 1);
    expect_read(BNO055_REG_CALIB_STAT, &calib, 1);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_get_data(&dev, &data));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 16.0f, data.euler.heading);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, data.euler.pitch);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, data.acc_x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -2.0f, data.acc_y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, data.linear_acc_x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.34f, data.gravity_z);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, data.mag_x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -2.0f, data.mag_y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f * 0.017453292519943295f, data.gyr_x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f * 0.017453292519943295f, data.gyr_y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 23.0f, data.temperature);
    TEST_ASSERT_EQUAL_UINT8(3, data.calib.sys);
}


void test_get_data_tolerates_optional_vector_read_failures_until_calib_failure(void)
{
    int bus;
    xy_bno055_t dev = make_ready_dev(&bus);
    bno055_data_t data;
    uint8_t euler_bytes[6] = {0x00, 0x01, 0x00, 0x00, 0xF0, 0xFF};
    uint8_t quat_bytes[8] = {0x00, 0x40, 0, 0, 0, 0, 0, 0};
    const uint8_t calib = 0xC0;

    memset(&data, 0xA5, sizeof(data));
    data.acc_x = 1.25f;
    data.linear_acc_x = 2.5f;
    data.gravity_z = 3.75f;
    data.mag_y = 4.5f;
    data.gyr_z = 5.25f;
    data.temperature = 6.0f;

    expect_read(BNO055_REG_EUL_HEADING_LSB, euler_bytes, sizeof(euler_bytes));
    expect_read(BNO055_REG_QUA_DATA_W_LSB, quat_bytes, sizeof(quat_bytes));
    expect_read_ret(BNO055_REG_ACC_DATA_X_LSB, NULL, 6, XY_DEVICE_ERROR);
    expect_read_ret(BNO055_REG_LIA_DATA_X_LSB, NULL, 6, XY_DEVICE_ERROR);
    expect_read_ret(BNO055_REG_GRV_DATA_X_LSB, NULL, 6, XY_DEVICE_ERROR);
    expect_read_ret(BNO055_REG_MAG_DATA_X_LSB, NULL, 6, XY_DEVICE_ERROR);
    expect_read_ret(BNO055_REG_GYR_DATA_X_LSB, NULL, 6, XY_DEVICE_ERROR);
    expect_read_ret(BNO055_REG_TEMP, NULL, 1, XY_DEVICE_ERROR);
    expect_read(BNO055_REG_CALIB_STAT, &calib, 1);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_get_data(&dev, &data));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 16.0f, data.euler.heading);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, data.euler.pitch);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, data.quat.w);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.25f, data.acc_x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.5f, data.linear_acc_x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.75f, data.gravity_z);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.5f, data.mag_y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.25f, data.gyr_z);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 6.0f, data.temperature);
    TEST_ASSERT_EQUAL_UINT8(3, data.calib.sys);

    expect_read(BNO055_REG_EUL_HEADING_LSB, euler_bytes, sizeof(euler_bytes));
    expect_read(BNO055_REG_QUA_DATA_W_LSB, quat_bytes, sizeof(quat_bytes));
    expect_read_ret(BNO055_REG_ACC_DATA_X_LSB, NULL, 6, XY_DEVICE_ERROR);
    expect_read_ret(BNO055_REG_LIA_DATA_X_LSB, NULL, 6, XY_DEVICE_ERROR);
    expect_read_ret(BNO055_REG_GRV_DATA_X_LSB, NULL, 6, XY_DEVICE_ERROR);
    expect_read_ret(BNO055_REG_MAG_DATA_X_LSB, NULL, 6, XY_DEVICE_ERROR);
    expect_read_ret(BNO055_REG_GYR_DATA_X_LSB, NULL, 6, XY_DEVICE_ERROR);
    expect_read_ret(BNO055_REG_TEMP, NULL, 1, XY_DEVICE_ERROR);
    expect_read_ret(BNO055_REG_CALIB_STAT, NULL, 1, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bno055_get_data(&dev, &data));
}

void test_axis_remap_and_status_helpers(void)
{
    int bus;
    xy_bno055_t dev = make_ready_dev(&bus);
    const uint8_t status = 0x05;
    const uint8_t self_test = 0x0F;
    const uint8_t mode_ndof = BNO055_MODE_NDOF;
    uint8_t value = 0;

    expect_read(BNO055_REG_SYS_STATUS, &status, 1);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_get_sys_status(&dev, &value));
    TEST_ASSERT_EQUAL_UINT8(status, value);

    expect_read_ret(BNO055_REG_SYS_STATUS, NULL, 1, XY_DEVICE_ERROR);
    value = 0xA5U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bno055_get_sys_status(&dev, &value));
    TEST_ASSERT_EQUAL_UINT8(0xA5U, value);

    expect_read(BNO055_REG_ST_RESULT, &self_test, 1);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_get_self_test(&dev, &value));
    TEST_ASSERT_EQUAL_UINT8(self_test, value);

    expect_read_ret(BNO055_REG_ST_RESULT, NULL, 1, XY_DEVICE_ERROR);
    value = 0x5AU;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bno055_get_self_test(&dev, &value));
    TEST_ASSERT_EQUAL_UINT8(0x5AU, value);

    expect_read(BNO055_REG_OPR_MODE, &mode_ndof, 1);
    expect_write_u8(BNO055_REG_OPR_MODE, BNO055_MODE_CONFIG);
    expect_write_u8(BNO055_REG_AXIS_MAP_CONFIG, 0x21);
    expect_write_u8(BNO055_REG_AXIS_MAP_SIGN, 0x04);
    expect_write_u8(BNO055_REG_OPR_MODE, BNO055_MODE_NDOF);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bno055_set_axis_remap(&dev, 0x21, 0x04));
}

void test_uart_mode_reports_not_supported(void)
{
    xy_bno055_t dev;
    int bus;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_SUPPORT, xy_bno055_init(&dev, &bus, 0x28, true));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_configures_units_and_ndof_mode);
    RUN_TEST(test_init_rejects_wrong_chip_id);
    RUN_TEST(test_register_access_guards_and_i2c_round_trip);
    RUN_TEST(test_mode_power_units_sleep_and_wakeup_write_expected_registers);
    RUN_TEST(test_euler_quaternion_calibration_and_raw_parsing);
    RUN_TEST(test_get_data_converts_fused_vectors);
    RUN_TEST(test_get_data_tolerates_optional_vector_read_failures_until_calib_failure);
    RUN_TEST(test_axis_remap_and_status_helpers);
    RUN_TEST(test_uart_mode_reports_not_supported);
    return UNITY_END();
}
