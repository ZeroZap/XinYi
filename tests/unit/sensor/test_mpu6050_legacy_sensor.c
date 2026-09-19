#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_mpu6050.h"
#include "xy_os.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef enum { OP_READ, OP_WRITE } op_kind_t;
typedef struct {
    op_kind_t kind;
    uint8_t reg;
    uint8_t data[14];
    size_t len;
    xy_error_t result;
} op_t;

static op_t ops[32];
static size_t op_count;
static size_t op_index;
static uint32_t tick;
static uint32_t delay_total;
static xy_error_t init_result;

static void queue_read(uint8_t reg, const uint8_t *data, size_t len, xy_error_t result)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(ops), op_count);
    ops[op_count].kind = OP_READ;
    ops[op_count].reg = reg;
    ops[op_count].len = len;
    ops[op_count].result = result;
    if (data != NULL) {
        memcpy(ops[op_count].data, data, len);
    }
    op_count++;
}

static void queue_read8(uint8_t reg, uint8_t value, xy_error_t result)
{
    queue_read(reg, &value, 1U, result);
}

static void queue_raw(int16_t ax, int16_t ay, int16_t az, int16_t gx, int16_t gy,
                      int16_t gz, xy_error_t result)
{
    uint8_t data[14] = {
        (uint8_t)(ax >> 8), (uint8_t)ax, (uint8_t)(ay >> 8), (uint8_t)ay,
        (uint8_t)(az >> 8), (uint8_t)az, 0U, 0U,
        (uint8_t)(gx >> 8), (uint8_t)gx, (uint8_t)(gy >> 8), (uint8_t)gy,
        (uint8_t)(gz >> 8), (uint8_t)gz,
    };
    queue_read(MPU6050_REG_ACCEL_XOUT_H, data, sizeof(data), result);
}

static void queue_write(uint8_t reg, uint8_t value, xy_error_t result)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(ops), op_count);
    ops[op_count].kind = OP_WRITE;
    ops[op_count].reg = reg;
    ops[op_count].data[0] = value;
    ops[op_count].len = 1U;
    ops[op_count].result = result;
    op_count++;
}

static op_t *next_op(op_kind_t kind, uint8_t reg, size_t len)
{
    op_t *op;
    TEST_ASSERT_LESS_THAN_UINT(op_count, op_index);
    op = &ops[op_index++];
    TEST_ASSERT_EQUAL_INT(kind, op->kind);
    TEST_ASSERT_EQUAL_UINT8(reg, op->reg);
    TEST_ASSERT_EQUAL_UINT(len, op->len);
    return op;
}

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *bus, uint16_t addr,
                              uint32_t timeout)
{
    if (init_result != XY_DEVICE_OK) {
        return init_result;
    }
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = true;
    dev->i2c_handle = bus;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data,
                                  size_t len)
{
    op_t *op;
    TEST_ASSERT_TRUE(dev->base.initialized);
    op = next_op(OP_READ, reg, len);
    if (op->result == XY_DEVICE_OK) {
        memcpy(data, op->data, len);
    }
    return op->result;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg,
                                   const uint8_t *data, size_t len)
{
    op_t *op;
    TEST_ASSERT_TRUE(dev->base.initialized);
    op = next_op(OP_WRITE, reg, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    return op->result;
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

xy_os_status_t xy_os_delay(uint32_t ticks)
{
    delay_total += ticks;
    tick += ticks;
    return XY_OS_OK;
}

void xy_hal_delay_ms(uint32_t ms)
{
    delay_total += ms;
    tick += ms;
}

uint32_t get_tick_ms(void)
{
    return tick;
}

void delay_ms(uint32_t ms)
{
    xy_hal_delay_ms(ms);
}

void setUp(void)
{
    memset(ops, 0, sizeof(ops));
    op_count = 0U;
    op_index = 0U;
    tick = 424242U;
    delay_total = 0U;
    init_result = XY_DEVICE_OK;
}

void tearDown(void)
{
}

static void queue_init_ok(void)
{
    queue_read8(MPU6050_REG_WHO_AM_I, MPU6050_WHO_AM_I_VALUE, XY_DEVICE_OK);
    queue_write(MPU6050_REG_PWR_MGMT_1, 0x00U, XY_DEVICE_OK);
    queue_write(MPU6050_REG_SMPLRT_DIV, 0x00U, XY_DEVICE_OK);
    queue_write(MPU6050_REG_CONFIG, MPU6050_DLPF_44HZ, XY_DEVICE_OK);
    queue_write(MPU6050_REG_ACCEL_CONFIG, 0x00U, XY_DEVICE_OK);
    queue_write(MPU6050_REG_GYRO_CONFIG, 0x00U, XY_DEVICE_OK);
}

static sensor_device_t *create_accel(void)
{
    static int bus;
    sensor_device_t *sensor = mpu6050_create_accel("mpu-accel", &bus);
    TEST_ASSERT_NOT_NULL(sensor);
    return sensor;
}

static sensor_device_t *create_gyro(void)
{
    static int bus;
    sensor_device_t *sensor = mpu6050_create_gyro("mpu-gyro", &bus);
    TEST_ASSERT_NOT_NULL(sensor);
    return sensor;
}

static void destroy_sensor(sensor_device_t *sensor)
{
    if (sensor != NULL) {
        SENSOR_FREE(sensor->priv_data);
        SENSOR_FREE(sensor);
    }
}

static void test_create_guards_and_identity(void)
{
    static int bus;
    sensor_device_t *accel;
    sensor_device_t *gyro;

    TEST_ASSERT_NULL(mpu6050_create_accel(NULL, &bus));
    TEST_ASSERT_NULL(mpu6050_create_accel("bad", NULL));
    TEST_ASSERT_NULL(mpu6050_create_gyro(NULL, &bus));
    TEST_ASSERT_NULL(mpu6050_create_gyro("bad", NULL));
    accel = create_accel();
    gyro = create_gyro();
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, accel->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, accel->info.unit);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GYROSCOPE, gyro->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_DEGREE_PER_SECOND, gyro->info.unit);
    TEST_ASSERT_EQUAL_UINT8(MPU6050_ADDR_DEFAULT,
                            ((mpu6050_priv_t *)accel->priv_data)->i2c_addr);
    destroy_sensor(accel);
    destroy_sensor(gyro);
}

static void test_init_delegates_and_maps_failures(void)
{
    sensor_device_t *sensor = create_accel();
    mpu6050_priv_t *priv = (mpu6050_priv_t *)sensor->priv_data;

    queue_init_ok();
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_TRUE(priv->device.initialized);
    TEST_ASSERT_EQUAL_UINT32(100U, delay_total);
    queue_write(MPU6050_REG_PWR_MGMT_1, 0x40U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->deinit(sensor));
    TEST_ASSERT_TRUE(priv->device.initialized);
    queue_write(MPU6050_REG_PWR_MGMT_1, 0x40U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));
    TEST_ASSERT_FALSE(priv->device.initialized);
    destroy_sensor(sensor);

    sensor = create_accel();
    op_count = 0U;
    op_index = 0U;
    init_result = XY_DEVICE_IO_ERROR;
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT(0U, op_index);
    destroy_sensor(sensor);

    sensor = create_accel();
    init_result = XY_DEVICE_OK;
    queue_read8(MPU6050_REG_WHO_AM_I, 0x69U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ENODEV, sensor->ops->init(sensor));
    destroy_sensor(sensor);
}

static void test_accel_read_delegates_and_preserves_output_on_failure(void)
{
    sensor_device_t *sensor = create_accel();
    sensor_data_t data;
    sensor_data_t snapshot;

    queue_init_ok();
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    queue_raw(16384, -8192, 4096, 131, -262, 393, XY_DEVICE_OK);
    memset(&data, 0, sizeof(data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, data.unit);
    TEST_ASSERT_EQUAL_INT32(1000, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-500, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(250, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(tick, data.timestamp);

    memset(&data, 0xA5, sizeof(data));
    snapshot = data;
    queue_raw(0, 0, 0, 0, 0, 0, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
    destroy_sensor(sensor);
}

static void test_gyro_read_delegates_and_public_guards_are_side_effect_free(void)
{
    sensor_device_t *sensor = create_gyro();
    sensor_data_t data;

    queue_init_ok();
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    queue_raw(0, 0, 0, 131, -262, 393, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GYROSCOPE, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_DEGREE_PER_SECOND, data.unit);
    TEST_ASSERT_EQUAL_INT32(1, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-2, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(3, data.value.val_3axis.z);

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(NULL));
    TEST_ASSERT_EQUAL_UINT(op_count, op_index);
    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_create_guards_and_identity);
    RUN_TEST(test_init_delegates_and_maps_failures);
    RUN_TEST(test_accel_read_delegates_and_preserves_output_on_failure);
    RUN_TEST(test_gyro_read_delegates_and_public_guards_are_side_effect_free);
    return UNITY_END();
}
