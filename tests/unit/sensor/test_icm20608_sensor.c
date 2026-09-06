#include "unity.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_icm20608.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[6];
    uint16_t len;
    int ret;
} bus_op_t;

static bus_op_t g_i2c_reads[32];
static bus_op_t g_i2c_writes[32];
static bus_op_t g_spi_reads[32];
static bus_op_t g_spi_writes[32];
static size_t g_i2c_read_count;
static size_t g_i2c_read_index;
static size_t g_i2c_write_count;
static size_t g_i2c_write_index;
static size_t g_spi_read_count;
static size_t g_spi_read_index;
static size_t g_spi_write_count;
static size_t g_spi_write_index;
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
    TEST_ASSERT_LESS_THAN_UINT(g_i2c_read_count, g_i2c_read_index);
    bus_op_t *op = &g_i2c_reads[g_i2c_read_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (op->ret == SENSOR_EOK) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(g_i2c_write_count, g_i2c_write_index);
    bus_op_t *op = &g_i2c_writes[g_i2c_write_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_MEMORY(op->data, data, len);
    return op->ret;
}

int hal_spi_read_reg(void *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(g_spi_read_count, g_spi_read_index);
    bus_op_t *op = &g_spi_reads[g_spi_read_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (op->ret == SENSOR_EOK) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

int hal_spi_write_reg(void *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(g_spi_write_count, g_spi_write_index);
    bus_op_t *op = &g_spi_writes[g_spi_write_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_MEMORY(op->data, data, len);
    return op->ret;
}

static void queue_read(bus_op_t *ops, size_t *count, void *bus, uint8_t addr, uint8_t reg,
                       const uint8_t *data, uint16_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_reads), *count);
    bus_op_t *op = &ops[(*count)++];
    memset(op, 0, sizeof(*op));
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = len;
    op->ret = ret;
    if (data != NULL) {
        memcpy(op->data, data, len);
    }
}

static void queue_write(bus_op_t *ops, size_t *count, void *bus, uint8_t addr, uint8_t reg,
                        uint8_t data, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_writes), *count);
    bus_op_t *op = &ops[(*count)++];
    memset(op, 0, sizeof(*op));
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->data[0] = data;
    op->len = 1U;
    op->ret = ret;
}

static void queue_i2c_read(void *bus, uint8_t reg, const uint8_t *data, uint16_t len, int ret)
{
    queue_read(g_i2c_reads, &g_i2c_read_count, bus, ICM20608_ADDR_DEFAULT, reg, data, len, ret);
}

static void queue_i2c_write(void *bus, uint8_t reg, uint8_t data, int ret)
{
    queue_write(g_i2c_writes, &g_i2c_write_count, bus, ICM20608_ADDR_DEFAULT, reg, data, ret);
}

static void queue_spi_read(void *bus, uint8_t reg, const uint8_t *data, uint16_t len, int ret)
{
    queue_read(g_spi_reads, &g_spi_read_count, bus, 0U, reg, data, len, ret);
}

static void queue_spi_write(void *bus, uint8_t reg, uint8_t data, int ret)
{
    queue_write(g_spi_writes, &g_spi_write_count, bus, 0U, reg, data, ret);
}

static void queue_i2c_init_success(void *bus)
{
    const uint8_t whoami = ICM20608_WHOAMI_VALUE;
    queue_i2c_read(bus, ICM20608_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_i2c_write(bus, ICM20608_REG_PWR_MGMT_1, 0x80U, SENSOR_EOK);
    queue_i2c_write(bus, ICM20608_REG_PWR_MGMT_1, 0x01U, SENSOR_EOK);
    queue_i2c_write(bus, ICM20608_REG_PWR_MGMT_2, 0x00U, SENSOR_EOK);
    queue_i2c_write(bus, ICM20608_REG_GYRO_CONFIG, 0x08U, SENSOR_EOK);
    queue_i2c_write(bus, ICM20608_REG_ACCEL_CONFIG, 0x08U, SENSOR_EOK);
    queue_i2c_write(bus, ICM20608_REG_CONFIG, 0x04U, SENSOR_EOK);
    queue_i2c_write(bus, ICM20608_REG_ACCEL_CONFIG2, 0x04U, SENSOR_EOK);
}

void setUp(void)
{
    memset(g_i2c_reads, 0, sizeof(g_i2c_reads));
    memset(g_i2c_writes, 0, sizeof(g_i2c_writes));
    memset(g_spi_reads, 0, sizeof(g_spi_reads));
    memset(g_spi_writes, 0, sizeof(g_spi_writes));
    g_i2c_read_count = 0;
    g_i2c_read_index = 0;
    g_i2c_write_count = 0;
    g_i2c_write_index = 0;
    g_spi_read_count = 0;
    g_spi_read_index = 0;
    g_spi_write_count = 0;
    g_spi_write_index = 0;
    g_tick = 7000U;
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

static void test_icm20608_create_identity_and_bus_contracts(void)
{
    int fake_bus;
    const char long_name[] = "icm20608-accelerometer-name-too-long";
    sensor_device_t *accel = icm20608_create_accel(long_name, &fake_bus, false);
    sensor_device_t *gyro = icm20608_create_gyro("icm-gyro", &fake_bus, false);
    sensor_device_t *temp = icm20608_create_temp("icm-temp", &fake_bus, true);

    TEST_ASSERT_NULL(icm20608_create_accel(NULL, &fake_bus, false));
    TEST_ASSERT_NULL(icm20608_create_gyro(NULL, &fake_bus, false));
    TEST_ASSERT_NULL(icm20608_create_temp(NULL, &fake_bus, false));

    TEST_ASSERT_NOT_NULL(accel);
    TEST_ASSERT_NOT_NULL(gyro);
    TEST_ASSERT_NOT_NULL(temp);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(accel->info.name));
    TEST_ASSERT_EQUAL_STRING("TDK InvenSense", accel->info.vendor);
    TEST_ASSERT_EQUAL_STRING("ICM20608", accel->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, accel->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, accel->info.unit);
    TEST_ASSERT_EQUAL_INT32(4000, accel->info.range_max);
    TEST_ASSERT_EQUAL_INT32(-4000, accel->info.range_min);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, accel->bus);
    TEST_ASSERT_FALSE(((icm20608_priv_t *)accel->priv_data)->use_spi);
    TEST_ASSERT_EQUAL_UINT8(4U, ((icm20608_priv_t *)accel->priv_data)->accel_range);

    TEST_ASSERT_EQUAL_STRING("icm-gyro", gyro->info.name);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GYROSCOPE, gyro->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_DEGREE_PER_SECOND, gyro->info.unit);
    TEST_ASSERT_EQUAL_INT32(500, gyro->info.range_max);
    TEST_ASSERT_EQUAL_INT32(-500, gyro->info.range_min);
    TEST_ASSERT_EQUAL_UINT16(500U, ((icm20608_priv_t *)gyro->priv_data)->gyro_range);

    TEST_ASSERT_EQUAL_STRING("icm-temp", temp->info.name);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_TEMPERATURE, temp->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_CELSIUS, temp->info.unit);
    TEST_ASSERT_TRUE(((icm20608_priv_t *)temp->priv_data)->use_spi);

    destroy_sensor(accel);
    destroy_sensor(gyro);
    destroy_sensor(temp);
}

static void test_icm20608_i2c_init_read_deinit_contracts(void)
{
    int fake_bus;
    const uint8_t accel_raw[6] = {0x00, 0x10, 0xFF, 0xF0, 0x20, 0x00};
    const uint8_t gyro_raw[6] = {0x00, 0x40, 0xFF, 0xC0, 0x10, 0x00};
    const uint8_t temp_raw[2] = {0x01, 0x46};
    sensor_data_t data = {0};
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    sensor_device_t *gyro = icm20608_create_gyro("icm-gyro", &fake_bus, false);
    sensor_device_t *temp = icm20608_create_temp("icm-temp", &fake_bus, false);

    TEST_ASSERT_NOT_NULL(accel);
    TEST_ASSERT_NOT_NULL(gyro);
    TEST_ASSERT_NOT_NULL(temp);

    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    TEST_ASSERT_EQUAL_UINT32(7100U, g_tick);

    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_XOUT_H, accel_raw, sizeof(accel_raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->read(accel, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, data.unit);
    TEST_ASSERT_EQUAL_INT32(1, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-1, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(1000, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(95, data.accuracy);

    queue_i2c_read(&fake_bus, ICM20608_REG_GYRO_XOUT_H, gyro_raw, sizeof(gyro_raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, gyro->ops->read(gyro, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GYROSCOPE, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_DEGREE_PER_SECOND, data.unit);
    TEST_ASSERT_EQUAL_INT32(0, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(0, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(62, data.value.val_3axis.z);

    queue_i2c_read(&fake_bus, ICM20608_REG_TEMP_OUT_H, temp_raw, sizeof(temp_raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, temp->ops->read(temp, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_TEMPERATURE, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_CELSIUS, data.unit);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 26.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT8(90, data.accuracy);

    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, 0x40U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->deinit(accel));

    destroy_sensor(accel);
    destroy_sensor(gyro);
    destroy_sensor(temp);
}

static void test_icm20608_failure_contracts_preserve_output(void)
{
    int fake_bus;
    const uint8_t wrong_whoami = 0x00U;
    const uint8_t whoami = ICM20608_WHOAMI_VALUE;
    sensor_data_t data = {.type = SENSOR_TYPE_ACCELEROMETER,
                          .unit = SENSOR_UNIT_MILLI_G,
                          .value.val_3axis = {11, 22, 33},
                          .timestamp = 1234U,
                          .accuracy = 44U};
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    TEST_ASSERT_NOT_NULL(accel);

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, accel->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, accel->ops->deinit(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, accel->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, accel->ops->read(accel, NULL));

    queue_i2c_read(&fake_bus, ICM20608_REG_WHOAMI, &wrong_whoami, 1U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, accel->ops->init(accel));

    queue_i2c_read(&fake_bus, ICM20608_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, 0x80U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, accel->ops->init(accel));
    TEST_ASSERT_EQUAL_UINT32(7000U, g_tick);

    queue_i2c_read(&fake_bus, ICM20608_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, 0x80U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, 0x01U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_2, 0x00U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, accel->ops->init(accel));

    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, 0x40U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, accel->ops->deinit(accel));

    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_XOUT_H, NULL, 6U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, accel->ops->read(accel, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, data.unit);
    TEST_ASSERT_EQUAL_INT32(11, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(22, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(33, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(1234U, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(44U, data.accuracy);

    SENSOR_FREE(accel->priv_data);
    accel->priv_data = NULL;
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, accel->ops->init(accel));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, accel->ops->deinit(accel));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, accel->ops->read(accel, &data));

    destroy_sensor(accel);
}

static void test_icm20608_accepts_pandora_identity(void)
{
    int fake_bus;
    const uint8_t pandora_whoami = ICM20608_WHOAMI_VALUE;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    TEST_ASSERT_NOT_NULL(accel);

    queue_i2c_read(&fake_bus, ICM20608_REG_WHOAMI, &pandora_whoami, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, 0x80U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, 0x01U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_2, 0x00U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_GYRO_CONFIG, 0x08U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_ACCEL_CONFIG, 0x08U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_CONFIG, 0x04U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_ACCEL_CONFIG2, 0x04U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));

    destroy_sensor(accel);
}

static void test_icm20608_spi_bus_path_smoke(void)
{
    int fake_bus;
    const uint8_t whoami = ICM20608_WHOAMI_VALUE;
    const uint8_t temp_raw[2] = {0x00, 0x00};
    sensor_data_t data = {0};
    sensor_device_t *temp = icm20608_create_temp("icm-temp", &fake_bus, true);
    TEST_ASSERT_NOT_NULL(temp);

    queue_spi_read(&fake_bus, ICM20608_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_spi_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, 0x80U, SENSOR_EOK);
    queue_spi_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, 0x01U, SENSOR_EOK);
    queue_spi_write(&fake_bus, ICM20608_REG_PWR_MGMT_2, 0x00U, SENSOR_EOK);
    queue_spi_write(&fake_bus, ICM20608_REG_GYRO_CONFIG, 0x08U, SENSOR_EOK);
    queue_spi_write(&fake_bus, ICM20608_REG_ACCEL_CONFIG, 0x08U, SENSOR_EOK);
    queue_spi_write(&fake_bus, ICM20608_REG_CONFIG, 0x04U, SENSOR_EOK);
    queue_spi_write(&fake_bus, ICM20608_REG_ACCEL_CONFIG2, 0x04U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, temp->ops->init(temp));

    queue_spi_read(&fake_bus, ICM20608_REG_TEMP_OUT_H, temp_raw, sizeof(temp_raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, temp->ops->read(temp, &data));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, data.value.val_float);

    destroy_sensor(temp);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_icm20608_create_identity_and_bus_contracts);
    RUN_TEST(test_icm20608_i2c_init_read_deinit_contracts);
    RUN_TEST(test_icm20608_failure_contracts_preserve_output);
    RUN_TEST(test_icm20608_accepts_pandora_identity);
    RUN_TEST(test_icm20608_spi_bus_path_smoke);
    return UNITY_END();
}
