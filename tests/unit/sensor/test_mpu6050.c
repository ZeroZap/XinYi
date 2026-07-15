#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_device.h"
#include "xy_mpu6050.h"
#include "xy_os.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef enum {
    OP_READ_REG,
    OP_WRITE_REG,
} op_kind_t;

typedef struct {
    op_kind_t kind;
    uint8_t reg;
    uint8_t data[14];
    size_t len;
    xy_error_t ret;
} i2c_op_t;

static i2c_op_t g_ops[96];
static uint8_t g_seen_writes[96][2];
static size_t g_op_count;
static size_t g_op_index;
static size_t g_seen_write_count;
static uint16_t g_last_addr;
static uint32_t g_last_timeout;
static uint32_t g_delay_total;

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

static void queue_read_raw(int16_t ax, int16_t ay, int16_t az,
                           int16_t temp, int16_t gx, int16_t gy, int16_t gz,
                           xy_error_t ret)
{
    uint8_t data[14] = {
        (uint8_t)(ax >> 8), (uint8_t)ax,
        (uint8_t)(ay >> 8), (uint8_t)ay,
        (uint8_t)(az >> 8), (uint8_t)az,
        (uint8_t)(temp >> 8), (uint8_t)temp,
        (uint8_t)(gx >> 8), (uint8_t)gx,
        (uint8_t)(gy >> 8), (uint8_t)gy,
        (uint8_t)(gz >> 8), (uint8_t)gz,
    };
    queue_read(MPU6050_REG_ACCEL_XOUT_H, data, sizeof(data), ret);
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

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle, uint16_t addr, uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(i2c_handle);
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1;
    dev->i2c_handle = i2c_handle;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    g_last_addr = addr;
    g_last_timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);

    i2c_op_t *op = next_op(OP_READ_REG);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    if (op->ret == XY_DEVICE_OK) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);

    i2c_op_t *op = next_op(OP_WRITE_REG);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    g_seen_writes[g_seen_write_count][0] = reg;
    g_seen_writes[g_seen_write_count][1] = data[0];
    g_seen_write_count++;
    return op->ret;
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

xy_os_status_t xy_os_delay(uint32_t ticks)
{
    g_delay_total += ticks;
    return XY_OS_OK;
}

void setUp(void)
{
    memset(g_ops, 0, sizeof(g_ops));
    memset(g_seen_writes, 0, sizeof(g_seen_writes));
    g_op_count = 0;
    g_op_index = 0;
    g_seen_write_count = 0;
    g_last_addr = 0;
    g_last_timeout = 0;
    g_delay_total = 0;
}

void tearDown(void)
{
}

static void queue_init_ok(void)
{
    queue_read8(MPU6050_REG_WHO_AM_I, MPU6050_WHO_AM_I_VALUE, XY_DEVICE_OK);
    queue_write8(MPU6050_REG_PWR_MGMT_1, 0x00U, XY_DEVICE_OK);
    queue_write8(MPU6050_REG_SMPLRT_DIV, 0x00U, XY_DEVICE_OK);
    queue_write8(MPU6050_REG_CONFIG, MPU6050_DLPF_44HZ, XY_DEVICE_OK);
    queue_write8(MPU6050_REG_ACCEL_CONFIG, (uint8_t)(MPU6050_ACCEL_2G << 3), XY_DEVICE_OK);
    queue_write8(MPU6050_REG_GYRO_CONFIG, (uint8_t)(MPU6050_GYRO_250DPS << 3), XY_DEVICE_OK);
}

static void init_mpu_ok(xy_mpu6050_t *dev, int *bus)
{
    queue_init_ok();
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_OK, xy_mpu6050_init(dev, bus, MPU6050_ADDR_AD0_LOW));
}

static void test_mpu6050_init_defaults_and_invalid_paths(void)
{
    xy_mpu6050_t dev;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_init(NULL, &bus, MPU6050_ADDR_AD0_LOW));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_init(&dev, NULL, MPU6050_ADDR_AD0_LOW));

    init_mpu_ok(&dev, &bus);
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(MPU6050_ADDR_AD0_LOW, dev.addr);
    TEST_ASSERT_EQUAL_UINT16(MPU6050_ADDR_AD0_LOW, g_last_addr);
    TEST_ASSERT_EQUAL_UINT32(1000U, g_last_timeout);
    TEST_ASSERT_EQUAL_UINT32(100U, g_delay_total);
    TEST_ASSERT_EQUAL_INT(MPU6050_ACCEL_2G, dev.accel_range);
    TEST_ASSERT_EQUAL_INT(MPU6050_GYRO_250DPS, dev.gyro_range);
    TEST_ASSERT_EQUAL_INT(MPU6050_DLPF_44HZ, dev.dlpf);

    queue_write8(MPU6050_REG_PWR_MGMT_1, 0x40U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_OK, xy_mpu6050_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_deinit(NULL));
}

static void test_mpu6050_not_found_id_error_and_wakeup_failure(void)
{
    xy_mpu6050_t dev;
    int bus;

    queue_read8(MPU6050_REG_WHO_AM_I, 0x00U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_NOT_FOUND, xy_mpu6050_init(&dev, &bus, MPU6050_ADDR_AD0_LOW));

    queue_read8(MPU6050_REG_WHO_AM_I, 0x69U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_ID_ERROR, xy_mpu6050_init(&dev, &bus, MPU6050_ADDR_AD0_LOW));

    queue_read8(MPU6050_REG_WHO_AM_I, MPU6050_WHO_AM_I_VALUE, XY_DEVICE_OK);
    queue_write8(MPU6050_REG_PWR_MGMT_1, 0x00U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_mpu6050_init(&dev, &bus, MPU6050_ADDR_AD0_LOW));
}

static void test_mpu6050_raw_read_converts_accel_gyro_temperature(void)
{
    xy_mpu6050_t dev;
    int bus;

    init_mpu_ok(&dev, &bus);
    queue_write8(MPU6050_REG_ACCEL_CONFIG, (uint8_t)(MPU6050_ACCEL_4G << 3), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_mpu6050_set_accel_range(&dev, MPU6050_ACCEL_4G));
    queue_write8(MPU6050_REG_GYRO_CONFIG, (uint8_t)(MPU6050_GYRO_500DPS << 3), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_mpu6050_set_gyro_range(&dev, MPU6050_GYRO_500DPS));

    dev.calib.accel_offset_x = 100.0f;
    dev.calib.accel_offset_y = -100.0f;
    dev.calib.accel_offset_z = 0.0f;
    dev.calib.gyro_offset_x = 10.0f;
    dev.calib.gyro_offset_y = -10.0f;
    dev.calib.gyro_offset_z = 0.0f;

    queue_read_raw(8292, -8292, 4096, 340, 665, -665, 328, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_OK, xy_mpu6050_read_raw(&dev));
    TEST_ASSERT_EQUAL_INT16(8292, dev.raw.accel_x);
    TEST_ASSERT_EQUAL_INT16(-8292, dev.raw.accel_y);
    TEST_ASSERT_EQUAL_INT16(340, dev.raw.temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, dev.accel_g[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -1.0f, dev.accel_g[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, dev.accel_g[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 10.0f, dev.gyro_dps[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -10.0f, dev.gyro_dps[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0076f, dev.gyro_dps[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 37.53f, dev.temperature_c);
}

static void test_mpu6050_read_helpers_validate_outputs_and_io_failure_paths(void)
{
    xy_mpu6050_t dev = {0};
    float x = 1.0f, y = 2.0f, z = 3.0f, temp = 4.0f;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_read_raw(NULL));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_read_raw(&dev));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_read_accel(NULL, &x, &y, &z));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_read_accel(&dev, NULL, &y, &z));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_read_accel(&dev, &x, NULL, &z));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_read_accel(&dev, &x, &y, NULL));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_read_gyro(NULL, &x, &y, &z));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_read_gyro(&dev, NULL, &y, &z));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_read_gyro(&dev, &x, NULL, &z));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_read_gyro(&dev, &x, &y, NULL));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_read_temperature(NULL, &temp));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_read_temperature(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_set_accel_range(NULL, MPU6050_ACCEL_2G));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_set_accel_range(&dev, (xy_mpu6050_accel_range_t)4));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_set_gyro_range(NULL, MPU6050_GYRO_250DPS));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_set_gyro_range(&dev, (xy_mpu6050_gyro_range_t)4));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_calibrate(NULL, 1));
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_INVALID_PARAM, xy_mpu6050_calibrate(&dev, 0));

    init_mpu_ok(&dev, &bus);
    queue_read_raw(0, 0, 0, 0, 0, 0, 0, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_mpu6050_read_accel(&dev, &x, &y, &z));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.0f, z);

    queue_read_raw(0, 0, 0, 0, 0, 0, 0, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_mpu6050_read_gyro(&dev, &x, &y, &z));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.0f, z);

    temp = 4.0f;
    queue_read_raw(0, 0, 0, 0, 0, 0, 0, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_mpu6050_read_temperature(&dev, &temp));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 4.0f, temp);
}

static void test_mpu6050_range_write_failures_preserve_configured_range(void)
{
    xy_mpu6050_t dev;
    int bus;

    init_mpu_ok(&dev, &bus);
    dev.accel_range = MPU6050_ACCEL_2G;
    queue_write8(MPU6050_REG_ACCEL_CONFIG, (uint8_t)(MPU6050_ACCEL_8G << 3), XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_mpu6050_set_accel_range(&dev, MPU6050_ACCEL_8G));
    TEST_ASSERT_EQUAL_INT(MPU6050_ACCEL_8G, dev.accel_range);

    dev.gyro_range = MPU6050_GYRO_250DPS;
    queue_write8(MPU6050_REG_GYRO_CONFIG, (uint8_t)(MPU6050_GYRO_1000DPS << 3), XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_mpu6050_set_gyro_range(&dev, MPU6050_GYRO_1000DPS));
    TEST_ASSERT_EQUAL_INT(MPU6050_GYRO_1000DPS, dev.gyro_range);
}


static void test_mpu6050_init_noncritical_config_failures_still_mark_ready(void)
{
    xy_mpu6050_t dev;
    int bus;

    queue_read8(MPU6050_REG_WHO_AM_I, MPU6050_WHO_AM_I_VALUE, XY_DEVICE_OK);
    queue_write8(MPU6050_REG_PWR_MGMT_1, 0x00U, XY_DEVICE_OK);
    queue_write8(MPU6050_REG_SMPLRT_DIV, 0x00U, XY_DEVICE_ERROR);
    queue_write8(MPU6050_REG_CONFIG, MPU6050_DLPF_44HZ, XY_DEVICE_ERROR);
    queue_write8(MPU6050_REG_ACCEL_CONFIG, (uint8_t)(MPU6050_ACCEL_2G << 3), XY_DEVICE_ERROR);
    queue_write8(MPU6050_REG_GYRO_CONFIG, (uint8_t)(MPU6050_GYRO_250DPS << 3), XY_DEVICE_ERROR);

    TEST_ASSERT_EQUAL_INT(XY_MPU6050_OK, xy_mpu6050_init(&dev, &bus, MPU6050_ADDR_AD0_LOW));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_INT(MPU6050_ACCEL_2G, dev.accel_range);
    TEST_ASSERT_EQUAL_INT(MPU6050_GYRO_250DPS, dev.gyro_range);
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);
}

static void test_mpu6050_deinit_write_failure_still_clears_initialized(void)
{
    xy_mpu6050_t dev;
    int bus;

    init_mpu_ok(&dev, &bus);
    queue_write8(MPU6050_REG_PWR_MGMT_1, 0x40U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_OK, xy_mpu6050_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
}

static void test_mpu6050_read_raw_failure_preserves_cached_samples(void)
{
    xy_mpu6050_t dev;
    xy_mpu6050_raw_data_t before_raw;
    float before_accel[3];
    float before_gyro[3];
    float before_temp;
    int bus;

    init_mpu_ok(&dev, &bus);
    queue_read_raw(1000, -2000, 3000, 340, 131, -262, 393, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_OK, xy_mpu6050_read_raw(&dev));
    before_raw = dev.raw;
    memcpy(before_accel, dev.accel_g, sizeof(before_accel));
    memcpy(before_gyro, dev.gyro_dps, sizeof(before_gyro));
    before_temp = dev.temperature_c;

    queue_read_raw(0, 0, 0, 0, 0, 0, 0, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_mpu6050_read_raw(&dev));
    TEST_ASSERT_EQUAL_MEMORY(&before_raw, &dev.raw, sizeof(before_raw));
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(before_accel, dev.accel_g, 3);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(before_gyro, dev.gyro_dps, 3);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, before_temp, dev.temperature_c);
}

static void test_mpu6050_calibrate_averages_offsets(void)
{
    xy_mpu6050_t dev;
    int bus;

    init_mpu_ok(&dev, &bus);
    queue_read_raw(100, 200, 16384, 0, 10, 20, 30, XY_DEVICE_OK);
    queue_read_raw(300, 400, 16384, 0, 30, 40, 50, XY_DEVICE_OK);

    TEST_ASSERT_EQUAL_INT(XY_MPU6050_OK, xy_mpu6050_calibrate(&dev, 2));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 200.0f, dev.calib.accel_offset_x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 300.0f, dev.calib.accel_offset_y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dev.calib.accel_offset_z);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, dev.calib.gyro_offset_x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, dev.calib.gyro_offset_y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 40.0f, dev.calib.gyro_offset_z);
    TEST_ASSERT_EQUAL_UINT32(120U, g_delay_total);
}

static void test_mpu6050_calibrate_uses_previous_sample_when_read_fails(void)
{
    xy_mpu6050_t dev;
    int bus;

    init_mpu_ok(&dev, &bus);
    queue_read_raw(100, 200, 16384, 0, 10, 20, 30, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_MPU6050_OK, xy_mpu6050_read_raw(&dev));
    queue_read_raw(0, 0, 0, 0, 0, 0, 0, XY_DEVICE_ERROR);

    TEST_ASSERT_EQUAL_INT(XY_MPU6050_OK, xy_mpu6050_calibrate(&dev, 1));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, dev.calib.accel_offset_x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 200.0f, dev.calib.accel_offset_y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dev.calib.accel_offset_z);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, dev.calib.gyro_offset_x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, dev.calib.gyro_offset_y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, dev.calib.gyro_offset_z);
    TEST_ASSERT_EQUAL_UINT32(110U, g_delay_total);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mpu6050_init_defaults_and_invalid_paths);
    RUN_TEST(test_mpu6050_not_found_id_error_and_wakeup_failure);
    RUN_TEST(test_mpu6050_raw_read_converts_accel_gyro_temperature);
    RUN_TEST(test_mpu6050_read_helpers_validate_outputs_and_io_failure_paths);
    RUN_TEST(test_mpu6050_range_write_failures_preserve_configured_range);
    RUN_TEST(test_mpu6050_init_noncritical_config_failures_still_mark_ready);
    RUN_TEST(test_mpu6050_deinit_write_failure_still_clears_initialized);
    RUN_TEST(test_mpu6050_read_raw_failure_preserves_cached_samples);
    RUN_TEST(test_mpu6050_calibrate_averages_offsets);
    RUN_TEST(test_mpu6050_calibrate_uses_previous_sample_when_read_fails);
    return UNITY_END();
}
