#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_lsm9ds1.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data;
    int ret;
} i2c_op_t;

static i2c_op_t g_i2c_reads[96];
static i2c_op_t g_i2c_writes[96];
static size_t g_i2c_read_count;
static size_t g_i2c_read_index;
static size_t g_i2c_write_count;
static size_t g_i2c_write_index;
static uint32_t g_tick;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

void delay_ms(uint32_t ms)
{
    g_tick += ms;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_EQUAL_UINT16(1U, len);
    TEST_ASSERT_LESS_THAN_UINT(g_i2c_read_count, g_i2c_read_index);
    i2c_op_t *op = &g_i2c_reads[g_i2c_read_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    if (op->ret == SENSOR_EOK) {
        *data = op->data;
    }
    return op->ret;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_EQUAL_UINT16(1U, len);
    TEST_ASSERT_LESS_THAN_UINT(g_i2c_write_count, g_i2c_write_index);
    i2c_op_t *op = &g_i2c_writes[g_i2c_write_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT8(op->data, *data);
    return op->ret;
}

static void queue_i2c_read8(void *bus, uint8_t addr, uint8_t reg, uint8_t data, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_reads), g_i2c_read_count);
    g_i2c_reads[g_i2c_read_count++] = (i2c_op_t){bus, addr, reg, data, ret};
}

static void queue_i2c_write8(void *bus, uint8_t addr, uint8_t reg, uint8_t data, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_writes), g_i2c_write_count);
    g_i2c_writes[g_i2c_write_count++] = (i2c_op_t){bus, addr, reg, data, ret};
}

static void queue_lsm9ds1_init(void *bus, uint8_t imu_addr, uint8_t mag_addr)
{
    queue_i2c_read8(bus, imu_addr, LSM9DS1_REG_WHOAMI_IMU, LSM9DS1_IMU_WHOAMI_VALUE, SENSOR_EOK);
    queue_i2c_read8(bus, mag_addr, LSM9DS1_REG_WHOAMI_MAG, LSM9DS1_MAG_WHOAMI_VALUE, SENSOR_EOK);
    queue_i2c_write8(bus, imu_addr, LSM9DS1_REG_CTRL3_C, 0x01U, SENSOR_EOK);
    queue_i2c_write8(bus, imu_addr, LSM9DS1_REG_CTRL1_XL,
                     (uint8_t)((0x04U << 4) | LSM9DS1_ACCEL_RANGE_2G), SENSOR_EOK);
    queue_i2c_write8(bus, imu_addr, LSM9DS1_REG_CTRL2_G,
                     (uint8_t)((0x04U << 4) | LSM9DS1_GYRO_RANGE_250DPS), SENSOR_EOK);
    queue_i2c_write8(bus, mag_addr, LSM9DS1_REG_CTRL_REG1_M, 0x70U, SENSOR_EOK);
}

void setUp(void)
{
    memset(g_i2c_reads, 0, sizeof(g_i2c_reads));
    memset(g_i2c_writes, 0, sizeof(g_i2c_writes));
    g_i2c_read_count = 0;
    g_i2c_read_index = 0;
    g_i2c_write_count = 0;
    g_i2c_write_index = 0;
    g_tick = 51000U;
}

void tearDown(void)
{
}

static void destroy_sensor(sensor_device_t *sensor)
{
    if (sensor != NULL) {
        SENSOR_FREE(sensor->priv_data);
        SENSOR_FREE(sensor);
    }
}

static void test_lsm9ds1_create_default_contracts_and_name_truncation(void)
{
    int fake_bus;
    const char long_name[] = "lsm9ds1-accelerometer-name-too-long";
    sensor_device_t *accel = lsm9ds1_create_accel(long_name, &fake_bus);
    sensor_device_t *gyro = lsm9ds1_create_gyro("lsm9-gyr", &fake_bus);
    sensor_device_t *mag = lsm9ds1_create_mag("lsm9-mag", &fake_bus);

    TEST_ASSERT_NOT_NULL(accel);
    TEST_ASSERT_NOT_NULL(gyro);
    TEST_ASSERT_NOT_NULL(mag);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(accel->info.name));
    TEST_ASSERT_EQUAL_STRING("STMicro", accel->info.vendor);
    TEST_ASSERT_EQUAL_STRING("LSM9DS1", accel->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, accel->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, accel->info.unit);
    TEST_ASSERT_EQUAL_INT32(2000, accel->info.range_max);
    TEST_ASSERT_EQUAL_INT32(-2000, accel->info.range_min);
    TEST_ASSERT_EQUAL_INT32(100, accel->odr);
    TEST_ASSERT_EQUAL_UINT8(LSM9DS1_IMU_ADDR_DEFAULT, ((lsm9ds1_priv_t *)accel->priv_data)->imu_addr);
    TEST_ASSERT_EQUAL_UINT8(LSM9DS1_MAG_ADDR_DEFAULT, ((lsm9ds1_priv_t *)accel->priv_data)->mag_addr);

    TEST_ASSERT_EQUAL_STRING("lsm9-gyr", gyro->info.name);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GYROSCOPE, gyro->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_DEGREE_PER_SECOND, gyro->info.unit);
    TEST_ASSERT_EQUAL_INT32(250, gyro->info.range_max);
    TEST_ASSERT_EQUAL_INT32(-250, gyro->info.range_min);

    TEST_ASSERT_EQUAL_STRING("lsm9-mag", mag->info.name);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_MAGNETIC, mag->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MICRO_TESLA, mag->info.unit);
    TEST_ASSERT_EQUAL_INT32(4900, mag->info.range_max);
    TEST_ASSERT_EQUAL_INT32(-4900, mag->info.range_min);

    destroy_sensor(accel);
    destroy_sensor(gyro);
    destroy_sensor(mag);
}

static void test_lsm9ds1_init_read_deinit_and_failure_contracts(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    const uint8_t accel_raw[6] = {0x00, 0x10, 0x00, 0xF0, 0x00, 0x40};
    const uint8_t gyro_raw[6] = {0x00, 0x04, 0x00, 0xFC, 0x00, 0x10};
    const uint8_t mag_raw[6] = {0x00, 0x01, 0x00, 0xFF, 0x00, 0x02};
    sensor_device_t *accel = lsm9ds1_create_accel("lsm9-acc", &fake_bus);
    sensor_device_t *gyro = lsm9ds1_create_gyro("lsm9-gyr", &fake_bus);
    sensor_device_t *mag = lsm9ds1_create_mag("lsm9-mag", &fake_bus);

    TEST_ASSERT_NOT_NULL(accel);
    TEST_ASSERT_NOT_NULL(gyro);
    TEST_ASSERT_NOT_NULL(mag);

    queue_lsm9ds1_init(&fake_bus, LSM9DS1_IMU_ADDR_DEFAULT, LSM9DS1_MAG_ADDR_DEFAULT);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    TEST_ASSERT_EQUAL_UINT32(51010U, g_tick);

    for (uint8_t i = 0; i < sizeof(accel_raw); ++i) {
        queue_i2c_read8(&fake_bus, LSM9DS1_IMU_ADDR_DEFAULT, (uint8_t)(LSM9DS1_REG_OUTX_L_XL + i),
                        accel_raw[i], SENSOR_EOK);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->read(accel, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, data.unit);
    TEST_ASSERT_EQUAL_INT32(249, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-249, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(999, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(95, data.accuracy);

    ((lsm9ds1_priv_t *)gyro->priv_data)->gyro_range = 250U;
    for (uint8_t i = 0; i < sizeof(gyro_raw); ++i) {
        queue_i2c_read8(&fake_bus, LSM9DS1_IMU_ADDR_DEFAULT, (uint8_t)(LSM9DS1_REG_OUTX_L_G + i),
                        gyro_raw[i], SENSOR_EOK);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, gyro->ops->read(gyro, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GYROSCOPE, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_DEGREE_PER_SECOND, data.unit);
    TEST_ASSERT_EQUAL_INT32(9, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-9, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(36, data.value.val_3axis.z);

    for (uint8_t i = 0; i < sizeof(mag_raw); ++i) {
        queue_i2c_read8(&fake_bus, LSM9DS1_MAG_ADDR_DEFAULT, (uint8_t)(LSM9DS1_REG_OUTX_L_M + i),
                        mag_raw[i], SENSOR_EOK);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, mag->ops->read(mag, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_MAGNETIC, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MICRO_TESLA, data.unit);
    TEST_ASSERT_EQUAL_INT32(3840, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-3840, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(7680, data.value.val_3axis.z);

    queue_i2c_write8(&fake_bus, LSM9DS1_IMU_ADDR_DEFAULT, LSM9DS1_REG_CTRL1_XL, 0x00U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LSM9DS1_IMU_ADDR_DEFAULT, LSM9DS1_REG_CTRL2_G, 0x00U, SENSOR_EOK);
    queue_i2c_write8(&fake_bus, LSM9DS1_MAG_ADDR_DEFAULT, LSM9DS1_REG_CTRL_REG1_M, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->deinit(accel));

    queue_i2c_read8(&fake_bus, LSM9DS1_IMU_ADDR_DEFAULT, LSM9DS1_REG_WHOAMI_IMU, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, accel->ops->init(accel));
    queue_i2c_read8(&fake_bus, LSM9DS1_IMU_ADDR_DEFAULT, LSM9DS1_REG_OUTX_L_XL, 0x00U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, accel->ops->read(accel, &data));

    destroy_sensor(accel);
    destroy_sensor(gyro);
    destroy_sensor(mag);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lsm9ds1_create_default_contracts_and_name_truncation);
    RUN_TEST(test_lsm9ds1_init_read_deinit_and_failure_contracts);
    return UNITY_END();
}
