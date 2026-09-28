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
    uint8_t data[14];
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

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(handle);
    TEST_ASSERT_TRUE(address == ICM20608_ADDR_DEFAULT || address == ICM20608_ADDR_ALT);
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1U;
    dev->i2c_handle = handle;
    dev->dev_addr = address;
    dev->timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    return (xy_error_t)hal_i2c_mem_read(dev->i2c_handle, (uint8_t)dev->dev_addr, reg, data,
                                        (uint16_t)len);
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data,
                                    size_t len)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    return (xy_error_t)hal_i2c_mem_write(dev->i2c_handle, (uint8_t)dev->dev_addr, reg,
                                         (uint8_t *)data, (uint16_t)len);
}

uint32_t get_tick_ms(void)
{
    return g_tick;
}

void delay_ms(uint32_t ms)
{
    g_tick += ms;
}

void xy_hal_delay_ms(uint32_t ms)
{
    delay_ms(ms);
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
    queue_i2c_write(bus, ICM20608_REG_SMPLRT_DIV, 0x09U, SENSOR_EOK);
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
    TEST_ASSERT_NULL(icm20608_create_accel("icm-acc", NULL, false));
    TEST_ASSERT_NULL(icm20608_create_gyro("icm-gyro", NULL, false));
    TEST_ASSERT_NULL(icm20608_create_temp("icm-temp", NULL, true));

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
    TEST_ASSERT_EQUAL_UINT32(SENSOR_FLAG_INT_SUPPORT, accel->info.flags);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, accel->bus);
    TEST_ASSERT_FALSE(((icm20608_priv_t *)accel->priv_data)->use_spi);
    TEST_ASSERT_EQUAL_UINT8(4U, ((icm20608_priv_t *)accel->priv_data)->accel_range);

    TEST_ASSERT_EQUAL_STRING("icm-gyro", gyro->info.name);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GYROSCOPE, gyro->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_DEGREE_PER_SECOND, gyro->info.unit);
    TEST_ASSERT_EQUAL_INT32(500, gyro->info.range_max);
    TEST_ASSERT_EQUAL_INT32(-500, gyro->info.range_min);
    TEST_ASSERT_EQUAL_UINT32(SENSOR_FLAG_INT_SUPPORT, gyro->info.flags);
    TEST_ASSERT_EQUAL_UINT16(500U, ((icm20608_priv_t *)gyro->priv_data)->gyro_range);

    TEST_ASSERT_EQUAL_STRING("icm-temp", temp->info.name);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_TEMPERATURE, temp->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_CELSIUS, temp->info.unit);
    TEST_ASSERT_EQUAL_UINT32(0U, temp->info.flags);
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
    TEST_ASSERT_EQUAL_UINT16(100U,
                             ((icm20608_priv_t *)accel->priv_data)->device.odr_hz);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, gyro->ops->init(gyro));
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, temp->ops->init(temp));

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
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 26.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT8(90, data.accuracy);

    {
        const uint8_t active_power = 0x01U;
        queue_i2c_read(&fake_bus, ICM20608_REG_PWR_MGMT_1, &active_power, 1U, SENSOR_EOK);
    }
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, 0x41U, SENSOR_EOK);
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

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, accel->ops->deinit(accel));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, accel->ops->read(accel, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MILLI_G, data.unit);
    TEST_ASSERT_EQUAL_INT32(11, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(22, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(33, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(1234U, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(44U, data.accuracy);

    accel->bus = NULL;
    sensor_data_t snapshot = data;
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, accel->ops->init(accel));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, accel->ops->deinit(accel));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, accel->ops->read(accel, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);

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
    queue_i2c_write(&fake_bus, ICM20608_REG_SMPLRT_DIV, 0x09U, SENSOR_EOK);
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
    queue_spi_write(&fake_bus, ICM20608_REG_SMPLRT_DIV, 0x09U, SENSOR_EOK);
    queue_spi_write(&fake_bus, ICM20608_REG_CONFIG, 0x04U, SENSOR_EOK);
    queue_spi_write(&fake_bus, ICM20608_REG_ACCEL_CONFIG2, 0x04U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, temp->ops->init(temp));

    queue_spi_read(&fake_bus, ICM20608_REG_TEMP_OUT_H, temp_raw, sizeof(temp_raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, temp->ops->read(temp, &data));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, data.value.val_float);

    destroy_sensor(temp);
}

static void test_icm20608_propagates_first_transport_error(void)
{
    int fake_bus;
    const uint8_t whoami = ICM20608_WHOAMI_VALUE;
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .timestamp = 123U};
    sensor_data_t snapshot = data;
    sensor_device_t *accel = icm20608_create_accel("icm-error", &fake_bus, false);

    TEST_ASSERT_NOT_NULL(accel);
    queue_i2c_read(&fake_bus, ICM20608_REG_WHOAMI, &whoami, 1U, SENSOR_ETIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, accel->ops->init(accel));

    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_XOUT_H, NULL, 6U, SENSOR_ETIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, accel->ops->read(accel, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));

    {
        const uint8_t active_power = 0x01U;
        queue_i2c_read(&fake_bus, ICM20608_REG_PWR_MGMT_1, &active_power, 1U, SENSOR_EOK);
    }
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, 0x41U, SENSOR_ETIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, accel->ops->deinit(accel));
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
}

static void test_icm20608_runtime_ranges_update_scaling_and_metadata(void)
{
    int fake_bus;
    const uint8_t config = 0xA5U;
    const uint8_t half_scale[6] = {0x40, 0x00, 0, 0, 0, 0};
    sensor_data_t data = {0};
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    sensor_device_t *gyro = icm20608_create_gyro("icm-gyro", &fake_bus, false);

    TEST_ASSERT_NOT_NULL(accel);
    TEST_ASSERT_NOT_NULL(gyro);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_CONFIG, &config, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_ACCEL_CONFIG, 0xBDU, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK,
                          icm20608_set_accel_range(accel, XY_ICM20608_ACCEL_RANGE_16G));
    TEST_ASSERT_EQUAL_INT32(-16000, accel->info.range_min);
    TEST_ASSERT_EQUAL_INT32(16000, accel->info.range_max);
    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_XOUT_H, half_scale, 6U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->read(accel, &data));
    TEST_ASSERT_EQUAL_INT32(8000, data.value.val_3axis.x);

    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, gyro->ops->init(gyro));
    queue_i2c_read(&fake_bus, ICM20608_REG_GYRO_CONFIG, &config, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_GYRO_CONFIG, 0xBDU, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK,
                          icm20608_set_gyro_range(gyro, XY_ICM20608_GYRO_RANGE_2000DPS));
    TEST_ASSERT_EQUAL_INT32(-2000, gyro->info.range_min);
    TEST_ASSERT_EQUAL_INT32(2000, gyro->info.range_max);
    queue_i2c_read(&fake_bus, ICM20608_REG_GYRO_XOUT_H, half_scale, 6U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, gyro->ops->read(gyro, &data));
    TEST_ASSERT_EQUAL_INT32(1000, data.value.val_3axis.x);

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL,
                          icm20608_set_accel_range(gyro, XY_ICM20608_ACCEL_RANGE_2G));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL,
                          icm20608_set_gyro_range(accel, XY_ICM20608_GYRO_RANGE_250DPS));
    destroy_sensor(accel);
    destroy_sensor(gyro);
}

static void test_icm20608_runtime_odr_is_exact_and_failure_atomic(void)
{
    int fake_bus;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);

    TEST_ASSERT_NOT_NULL(accel);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));

    {
        const uint8_t divider = 0x09U;
        queue_i2c_write(&fake_bus, ICM20608_REG_SMPLRT_DIV, divider, SENSOR_EOK);
        queue_i2c_read(&fake_bus, ICM20608_REG_SMPLRT_DIV, &divider, 1U, SENSOR_EOK);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, icm20608_set_odr(accel, 100U));
    TEST_ASSERT_EQUAL_UINT16(100U, accel->odr);
    TEST_ASSERT_EQUAL_UINT16(100U,
                             ((icm20608_priv_t *)accel->priv_data)->device.odr_hz);

    queue_i2c_write(&fake_bus, ICM20608_REG_SMPLRT_DIV, 0x04U, SENSOR_ETIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, icm20608_set_odr(accel, 200U));
    TEST_ASSERT_EQUAL_UINT16(100U, accel->odr);
    TEST_ASSERT_EQUAL_UINT16(100U,
                             ((icm20608_priv_t *)accel->priv_data)->device.odr_hz);

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_set_odr(accel, 333U));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_set_odr(accel, 3U));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_set_odr(accel, 1001U));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_set_odr(NULL, 100U));

    {
        const uint8_t stale_divider = 0x09U;
        queue_i2c_write(&fake_bus, ICM20608_REG_SMPLRT_DIV, 0x04U, SENSOR_EOK);
        queue_i2c_read(&fake_bus, ICM20608_REG_SMPLRT_DIV, &stale_divider, 1U, SENSOR_EOK);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, icm20608_set_odr(accel, 200U));
    TEST_ASSERT_EQUAL_UINT16(100U, accel->odr);
    TEST_ASSERT_EQUAL_UINT16(
        100U, ((icm20608_priv_t *)accel->priv_data)->device.odr_hz);
    TEST_ASSERT_FALSE(
        ((icm20608_priv_t *)accel->priv_data)->device.configuration_synchronized);
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);

    destroy_sensor(accel);
}

static void test_icm20608_power_mode_preserves_register_and_cache_on_failure(void)
{
    int fake_bus;
    const uint8_t active_power = 0x21U;
    const uint8_t sleeping_power = 0x61U;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);

    TEST_ASSERT_NOT_NULL(accel);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    TEST_ASSERT_FALSE(((icm20608_priv_t *)accel->priv_data)->device.sleeping);

    queue_i2c_read(&fake_bus, ICM20608_REG_PWR_MGMT_1, &active_power, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, sleeping_power, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK,
                          accel->ops->set_power_mode(accel, SENSOR_POWER_MODE_SLEEP));
    TEST_ASSERT_TRUE(((icm20608_priv_t *)accel->priv_data)->device.sleeping);
    TEST_ASSERT_EQUAL_UINT32(7100U, g_tick);

    queue_i2c_read(&fake_bus, ICM20608_REG_PWR_MGMT_1, &sleeping_power, 1U,
                   SENSOR_ETIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT,
                          accel->ops->set_power_mode(accel, SENSOR_POWER_MODE_NORMAL));
    TEST_ASSERT_TRUE(((icm20608_priv_t *)accel->priv_data)->device.sleeping);
    TEST_ASSERT_EQUAL_UINT32(7100U, g_tick);

    queue_i2c_read(&fake_bus, ICM20608_REG_PWR_MGMT_1, &sleeping_power, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, active_power, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK,
                          accel->ops->set_power_mode(accel, SENSOR_POWER_MODE_NORMAL));
    TEST_ASSERT_FALSE(((icm20608_priv_t *)accel->priv_data)->device.sleeping);
    TEST_ASSERT_EQUAL_UINT32(7135U, g_tick);

    queue_i2c_read(&fake_bus, ICM20608_REG_PWR_MGMT_1, &active_power, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, active_power, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK,
                          accel->ops->set_power_mode(accel, SENSOR_POWER_MODE_NORMAL));
    TEST_ASSERT_EQUAL_UINT32(7135U, g_tick);

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL,
                          accel->ops->set_power_mode(accel, (sensor_power_mode_t)99));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL,
                          accel->ops->set_power_mode(NULL, SENSOR_POWER_MODE_SLEEP));
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
}

static void test_icm20608_data_ready_interrupt_preserves_register_and_reports_status(void)
{
    int fake_bus;
    const uint8_t int_enable_other = 0xA0U;
    const uint8_t int_enable_data_ready = 0xA1U;
    const uint8_t int_status = 0x11U;
    uint8_t status = 0xCCU;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);

    TEST_ASSERT_NOT_NULL(accel);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));

    queue_i2c_read(&fake_bus, ICM20608_REG_INT_ENABLE, &int_enable_other, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_INT_ENABLE, int_enable_data_ready, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(
        SENSOR_EOK,
        accel->ops->interrupt_enable(accel, SENSOR_INT_DATA_READY, true));
    TEST_ASSERT_TRUE(
        ((icm20608_priv_t *)accel->priv_data)->device.data_ready_interrupt_enabled);

    queue_i2c_read(&fake_bus, ICM20608_REG_INT_ENABLE, &int_enable_data_ready, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_INT_ENABLE, int_enable_other, SENSOR_ETIMEOUT);
    TEST_ASSERT_EQUAL_INT(
        SENSOR_ETIMEOUT,
        accel->ops->interrupt_enable(accel, SENSOR_INT_DATA_READY, false));
    TEST_ASSERT_TRUE(
        ((icm20608_priv_t *)accel->priv_data)->device.data_ready_interrupt_enabled);

    queue_i2c_read(&fake_bus, ICM20608_REG_INT_STATUS, &int_status, 1U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, icm20608_read_interrupt_status(accel, &status));
    TEST_ASSERT_EQUAL_HEX8(int_status, status);

    queue_i2c_read(&fake_bus, ICM20608_REG_INT_ENABLE, &int_enable_data_ready, 1U,
                   SENSOR_ETIMEOUT);
    TEST_ASSERT_EQUAL_INT(
        SENSOR_ETIMEOUT,
        accel->ops->interrupt_enable(accel, SENSOR_INT_DATA_READY, false));

    status = 0xCCU;
    queue_i2c_read(&fake_bus, ICM20608_REG_INT_STATUS, NULL, 1U, SENSOR_ETIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, icm20608_read_interrupt_status(accel, &status));
    TEST_ASSERT_EQUAL_HEX8(0xCCU, status);

    TEST_ASSERT_EQUAL_INT(
        SENSOR_EINVAL,
        accel->ops->interrupt_enable(accel, 0x80000000U, true));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_read_interrupt_status(accel, NULL));
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
}

static void test_icm20608_coherent_sample_is_single_burst_and_failure_atomic(void)
{
    int fake_bus;
    const uint8_t raw[14] = {
        0x20, 0x00, 0xE0, 0x00, 0x40, 0x00, 0x01,
        0x46, 0x10, 0x00, 0xF0, 0x00, 0x08, 0x00,
    };
    xy_icm20608_sample_t sample = {0};
    xy_icm20608_sample_t snapshot;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    icm20608_priv_t *priv;

    TEST_ASSERT_NOT_NULL(accel);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    priv = (icm20608_priv_t *)accel->priv_data;

    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_XOUT_H, raw, sizeof(raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_read_sample(&priv->device, &sample));
    TEST_ASSERT_EQUAL_INT32(1000, sample.accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(-1000, sample.accel.y_mg);
    TEST_ASSERT_EQUAL_INT32(2000, sample.accel.z_mg);
    TEST_ASSERT_EQUAL_INT32(2599, sample.temperature_centi_c);
    TEST_ASSERT_EQUAL_INT32(62500, sample.gyro.x_mdps);
    TEST_ASSERT_EQUAL_INT32(-62500, sample.gyro.y_mdps);
    TEST_ASSERT_EQUAL_INT32(31250, sample.gyro.z_mdps);
    TEST_ASSERT_EQUAL_MEMORY(&sample.accel, &priv->device.accel, sizeof(sample.accel));
    TEST_ASSERT_EQUAL_MEMORY(&sample.gyro, &priv->device.gyro, sizeof(sample.gyro));
    snapshot = sample;

    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_XOUT_H, NULL, sizeof(raw), SENSOR_ETIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT,
                          xy_icm20608_read_sample(&priv->device, &sample));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &sample, sizeof(sample));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot.accel, &priv->device.accel, sizeof(snapshot.accel));

    priv->device.sleeping = 1U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_read_sample(&priv->device, &sample));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &sample, sizeof(sample));
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    destroy_sensor(accel);
}

static void test_icm20608_sleep_blocks_all_sample_reads_without_bus_access(void)
{
    int fake_bus;
    const uint8_t active_power = 0x21U;
    const uint8_t sleeping_power = 0x61U;
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM,
                          .unit = SENSOR_UNIT_NONE,
                          .value.val_3axis = {11, 22, 33},
                          .timestamp = 44U,
                          .accuracy = 55U};
    sensor_data_t snapshot = data;
    xy_icm20608_gyro_t gyro = {101, 202, 303};
    xy_icm20608_gyro_t gyro_snapshot = gyro;
    int32_t temperature = 1234;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    icm20608_priv_t *priv;

    TEST_ASSERT_NOT_NULL(accel);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    priv = (icm20608_priv_t *)accel->priv_data;
    queue_i2c_read(&fake_bus, ICM20608_REG_PWR_MGMT_1, &active_power, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, sleeping_power, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK,
                          accel->ops->set_power_mode(accel, SENSOR_POWER_MODE_SLEEP));

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, accel->ops->read(accel, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_read_gyro(&priv->device, &gyro));
    TEST_ASSERT_EQUAL_MEMORY(&gyro_snapshot, &gyro, sizeof(gyro));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_read_temperature(&priv->device, &temperature));
    TEST_ASSERT_EQUAL_INT32(1234, temperature);
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
}

static void test_icm20608_failed_reinit_preserves_live_owner(void)
{
    int fake_bus;
    const uint8_t wrong_whoami = 0x00U;
    const uint8_t accel_raw[6] = {0x20, 0x00, 0, 0, 0, 0};
    sensor_data_t data = {0};
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    icm20608_priv_t *priv;
    xy_icm20608_t snapshot;

    TEST_ASSERT_NOT_NULL(accel);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    priv = (icm20608_priv_t *)accel->priv_data;
    snapshot = priv->device;

    queue_i2c_read(&fake_bus, ICM20608_REG_WHOAMI, &wrong_whoami, 1U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, accel->ops->init(accel));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &priv->device, sizeof(snapshot));

    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_XOUT_H, accel_raw, sizeof(accel_raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->read(accel, &data));
    TEST_ASSERT_EQUAL_INT32(1000, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
}

static void test_icm20608_deinit_preserves_power_bits_and_owner_on_failure(void)
{
    int fake_bus;
    const uint8_t active_power = 0x25U;
    const uint8_t sleeping_power = 0x65U;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    icm20608_priv_t *priv;
    xy_icm20608_t snapshot;

    TEST_ASSERT_NOT_NULL(accel);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    priv = (icm20608_priv_t *)accel->priv_data;
    snapshot = priv->device;

    queue_i2c_read(&fake_bus, ICM20608_REG_PWR_MGMT_1, &active_power, 1U, SENSOR_ETIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, accel->ops->deinit(accel));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &priv->device, sizeof(snapshot));

    queue_i2c_read(&fake_bus, ICM20608_REG_PWR_MGMT_1, &active_power, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_PWR_MGMT_1, sleeping_power, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->deinit(accel));
    TEST_ASSERT_FALSE(priv->device.initialized);
    TEST_ASSERT_NULL(priv->device.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
}

static void test_icm20608_dlpf_control_preserves_bits_and_rolls_back(void)
{
    int fake_bus;
    const uint8_t gyro_config = 0xACU;
    const uint8_t accel_config = 0xBCU;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    icm20608_priv_t *priv;

    TEST_ASSERT_NOT_NULL(accel);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    priv = (icm20608_priv_t *)accel->priv_data;
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_DLPF_20HZ, priv->device.gyro_dlpf);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_DLPF_21HZ, priv->device.accel_dlpf);

    queue_i2c_read(&fake_bus, ICM20608_REG_CONFIG, &gyro_config, 1U, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_CONFIG2, &accel_config, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_CONFIG, 0xAAU, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_ACCEL_CONFIG2, 0xB9U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_icm20608_set_dlpf(&priv->device, XY_ICM20608_DLPF_92HZ,
                                              XY_ICM20608_ACCEL_DLPF_218HZ_ALT));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_DLPF_92HZ, priv->device.gyro_dlpf);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_DLPF_218HZ_ALT, priv->device.accel_dlpf);

    queue_i2c_read(&fake_bus, ICM20608_REG_CONFIG, &gyro_config, 1U, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_CONFIG2, &accel_config, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_CONFIG, 0xADU, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_ACCEL_CONFIG2, 0xBDU, SENSOR_ETIMEOUT);
    queue_i2c_write(&fake_bus, ICM20608_REG_CONFIG, gyro_config, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_CONFIG, NULL, 1U, SENSOR_ETIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT,
                          xy_icm20608_set_dlpf(&priv->device, XY_ICM20608_DLPF_10HZ,
                                              XY_ICM20608_ACCEL_DLPF_10HZ));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_DLPF_92HZ, priv->device.gyro_dlpf);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_DLPF_218HZ_ALT, priv->device.accel_dlpf);
    TEST_ASSERT_FALSE(priv->device.configuration_synchronized);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_set_dlpf(&priv->device, (xy_icm20608_dlpf_t)7,
                                              XY_ICM20608_ACCEL_DLPF_21HZ));
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
}

static void test_icm20608_failed_dlpf_rollback_fail_closes_until_reinit(void)
{
    int fake_bus;
    const uint8_t gyro_config = 0x84U;
    const uint8_t accel_config = 0x94U;
    xy_icm20608_accel_t accel_sample = {11, 22, 33};
    xy_icm20608_accel_t snapshot = accel_sample;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    icm20608_priv_t *priv;

    TEST_ASSERT_NOT_NULL(accel);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    priv = (icm20608_priv_t *)accel->priv_data;

    queue_i2c_read(&fake_bus, ICM20608_REG_CONFIG, &gyro_config, 1U, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_CONFIG2, &accel_config, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_CONFIG, 0x85U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_ACCEL_CONFIG2, 0x95U, SENSOR_ETIMEOUT);
    queue_i2c_write(&fake_bus, ICM20608_REG_CONFIG, gyro_config, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT,
                          xy_icm20608_set_dlpf(&priv->device, XY_ICM20608_DLPF_10HZ,
                                              XY_ICM20608_ACCEL_DLPF_10HZ));
    TEST_ASSERT_FALSE(priv->device.configuration_synchronized);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_read_accel(&priv->device, &accel_sample));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &accel_sample, sizeof(accel_sample));
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);

    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    TEST_ASSERT_TRUE(priv->device.configuration_synchronized);
    destroy_sensor(accel);
}

static void test_icm20608_wrapper_dlpf_control_maps_errors_and_validates_type(void)
{
    int fake_bus;
    const uint8_t gyro_config = 0x84U;
    const uint8_t accel_config = 0x94U;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    sensor_device_t *temp = icm20608_create_temp("icm-temp", &fake_bus, false);
    icm20608_priv_t *priv;

    TEST_ASSERT_NOT_NULL(accel);
    TEST_ASSERT_NOT_NULL(temp);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    priv = (icm20608_priv_t *)accel->priv_data;

    queue_i2c_read(&fake_bus, ICM20608_REG_CONFIG, &gyro_config, 1U, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_CONFIG2, &accel_config, 1U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_CONFIG, 0x82U, SENSOR_EOK);
    queue_i2c_write(&fake_bus, ICM20608_REG_ACCEL_CONFIG2, 0x91U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK,
                          icm20608_set_dlpf(accel, XY_ICM20608_DLPF_92HZ,
                                           XY_ICM20608_ACCEL_DLPF_218HZ_ALT));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_DLPF_92HZ, priv->device.gyro_dlpf);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_DLPF_218HZ_ALT, priv->device.accel_dlpf);

    queue_i2c_read(&fake_bus, ICM20608_REG_CONFIG, NULL, 1U, SENSOR_ETIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT,
                          icm20608_set_dlpf(accel, XY_ICM20608_DLPF_20HZ,
                                           XY_ICM20608_ACCEL_DLPF_21HZ));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_DLPF_92HZ, priv->device.gyro_dlpf);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_DLPF_218HZ_ALT, priv->device.accel_dlpf);

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL,
                          icm20608_set_dlpf(temp, XY_ICM20608_DLPF_20HZ,
                                           XY_ICM20608_ACCEL_DLPF_21HZ));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL,
                          icm20608_set_dlpf(NULL, XY_ICM20608_DLPF_20HZ,
                                           XY_ICM20608_ACCEL_DLPF_21HZ));
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
    destroy_sensor(temp);
}

static void test_icm20608_bias_is_applied_to_all_sample_paths(void)
{
    int fake_bus;
    const uint8_t sample_raw[14] = {
        0x08, 0x00, 0xF8, 0x00, 0x20, 0x00, 0x00,
        0x00, 0x04, 0x00, 0xFC, 0x00, 0x10, 0x00,
    };
    const uint8_t accel_raw[6] = {0x08, 0x00, 0xF8, 0x00, 0x20, 0x00};
    const uint8_t gyro_raw[6] = {0x04, 0x00, 0xFC, 0x00, 0x10, 0x00};
    const xy_icm20608_accel_t accel_bias = {50, -25, 100};
    const xy_icm20608_gyro_t gyro_bias = {10000, -5000, 20000};
    xy_icm20608_sample_t sample;
    xy_icm20608_accel_t accel;
    xy_icm20608_gyro_t gyro;
    sensor_device_t *sensor = icm20608_create_accel("icm-acc", &fake_bus, false);
    icm20608_priv_t *priv;

    TEST_ASSERT_NOT_NULL(sensor);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    priv = (icm20608_priv_t *)sensor->priv_data;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_icm20608_set_bias(&priv->device, &accel_bias, &gyro_bias));

    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_XOUT_H, sample_raw, sizeof(sample_raw),
                   SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_read_sample(&priv->device, &sample));
    TEST_ASSERT_EQUAL_INT32(200, sample.accel.x_mg);
    TEST_ASSERT_EQUAL_INT32(-225, sample.accel.y_mg);
    TEST_ASSERT_EQUAL_INT32(900, sample.accel.z_mg);
    TEST_ASSERT_EQUAL_INT32(5625, sample.gyro.x_mdps);
    TEST_ASSERT_EQUAL_INT32(-10625, sample.gyro.y_mdps);
    TEST_ASSERT_EQUAL_INT32(42500, sample.gyro.z_mdps);

    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_XOUT_H, accel_raw, sizeof(accel_raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_read_accel(&priv->device, &accel));
    TEST_ASSERT_EQUAL_MEMORY(&sample.accel, &accel, sizeof(accel));
    queue_i2c_read(&fake_bus, ICM20608_REG_GYRO_XOUT_H, gyro_raw, sizeof(gyro_raw), SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_icm20608_read_gyro(&priv->device, &gyro));
    TEST_ASSERT_EQUAL_MEMORY(&sample.gyro, &gyro, sizeof(gyro));

    accel = accel_bias;
    accel.x_mg = 4001;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_set_bias(&priv->device, &accel, &gyro_bias));
    TEST_ASSERT_EQUAL_MEMORY(&accel_bias, &priv->device.accel_bias, sizeof(accel_bias));
    TEST_ASSERT_EQUAL_MEMORY(&gyro_bias, &priv->device.gyro_bias, sizeof(gyro_bias));
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(sensor);
}

static void test_icm20608_wrapper_bias_validates_type_and_preserves_state(void)
{
    int fake_bus;
    const xy_icm20608_accel_t accel_bias = {100, -200, 300};
    const xy_icm20608_gyro_t gyro_bias = {1000, -2000, 3000};
    xy_icm20608_accel_t invalid_accel_bias = accel_bias;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    sensor_device_t *temp = icm20608_create_temp("icm-temp", &fake_bus, false);
    icm20608_priv_t *priv;

    TEST_ASSERT_NOT_NULL(accel);
    TEST_ASSERT_NOT_NULL(temp);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    priv = (icm20608_priv_t *)accel->priv_data;

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, icm20608_set_bias(accel, &accel_bias, &gyro_bias));
    TEST_ASSERT_EQUAL_MEMORY(&accel_bias, &priv->device.accel_bias, sizeof(accel_bias));
    TEST_ASSERT_EQUAL_MEMORY(&gyro_bias, &priv->device.gyro_bias, sizeof(gyro_bias));

    invalid_accel_bias.x_mg = 4001;
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL,
                          icm20608_set_bias(accel, &invalid_accel_bias, &gyro_bias));
    TEST_ASSERT_EQUAL_MEMORY(&accel_bias, &priv->device.accel_bias, sizeof(accel_bias));
    TEST_ASSERT_EQUAL_MEMORY(&gyro_bias, &priv->device.gyro_bias, sizeof(gyro_bias));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_set_bias(temp, &accel_bias, &gyro_bias));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_set_bias(NULL, &accel_bias, &gyro_bias));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_set_bias(accel, NULL, &gyro_bias));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_set_bias(accel, &accel_bias, NULL));
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
    destroy_sensor(temp);
}

static void test_icm20608_bias_getters_preserve_outputs_on_rejection(void)
{
    int fake_bus;
    const xy_icm20608_accel_t accel_bias = {100, -200, 300};
    const xy_icm20608_gyro_t gyro_bias = {1000, -2000, 3000};
    xy_icm20608_accel_t accel_out = {11, 22, 33};
    xy_icm20608_gyro_t gyro_out = {44, 55, 66};
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    sensor_device_t *temp = icm20608_create_temp("icm-temp", &fake_bus, false);
    icm20608_priv_t *priv;

    TEST_ASSERT_NOT_NULL(accel);
    TEST_ASSERT_NOT_NULL(temp);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    priv = (icm20608_priv_t *)accel->priv_data;
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, icm20608_set_bias(accel, &accel_bias, &gyro_bias));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_icm20608_get_bias(&priv->device, &accel_out, &gyro_out));
    TEST_ASSERT_EQUAL_MEMORY(&accel_bias, &accel_out, sizeof(accel_out));
    TEST_ASSERT_EQUAL_MEMORY(&gyro_bias, &gyro_out, sizeof(gyro_out));

    accel_out = (xy_icm20608_accel_t){11, 22, 33};
    gyro_out = (xy_icm20608_gyro_t){44, 55, 66};
    priv->device.configuration_synchronized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_get_bias(&priv->device, &accel_out, &gyro_out));
    TEST_ASSERT_EQUAL_INT32(11, accel_out.x_mg);
    TEST_ASSERT_EQUAL_INT32(44, gyro_out.x_mdps);
    priv->device.configuration_synchronized = 1U;

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, icm20608_get_bias(accel, &accel_out, &gyro_out));
    TEST_ASSERT_EQUAL_MEMORY(&accel_bias, &accel_out, sizeof(accel_out));
    TEST_ASSERT_EQUAL_MEMORY(&gyro_bias, &gyro_out, sizeof(gyro_out));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_get_bias(temp, &accel_out, &gyro_out));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_get_bias(NULL, &accel_out, &gyro_out));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_get_bias(accel, NULL, &gyro_out));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_get_bias(accel, &accel_out, NULL));
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
    destroy_sensor(temp);
}

static void test_icm20608_configuration_readback_is_staged_and_guarded(void)
{
    int fake_bus;
    xy_icm20608_configuration_t configuration = {
        .accel_range = XY_ICM20608_ACCEL_RANGE_16G,
        .gyro_range = XY_ICM20608_GYRO_RANGE_2000DPS,
        .gyro_dlpf = XY_ICM20608_DLPF_5HZ,
        .accel_dlpf = XY_ICM20608_ACCEL_DLPF_5HZ,
        .odr_hz = 999U,
        .sleeping = 1U,
        .data_ready_interrupt_enabled = 1U,
    };
    const xy_icm20608_configuration_t sentinel = configuration;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    sensor_device_t *temp = icm20608_create_temp("icm-temp", &fake_bus, false);
    icm20608_priv_t *priv;

    TEST_ASSERT_NOT_NULL(accel);
    TEST_ASSERT_NOT_NULL(temp);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    priv = (icm20608_priv_t *)accel->priv_data;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_icm20608_get_configuration(&priv->device, &configuration));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_RANGE_4G, configuration.accel_range);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_GYRO_RANGE_500DPS, configuration.gyro_range);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_DLPF_20HZ, configuration.gyro_dlpf);
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_DLPF_21HZ, configuration.accel_dlpf);
    TEST_ASSERT_EQUAL_UINT16(100U, configuration.odr_hz);
    TEST_ASSERT_FALSE(configuration.sleeping);
    TEST_ASSERT_FALSE(configuration.data_ready_interrupt_enabled);

    configuration = sentinel;
    priv->device.configuration_synchronized = 0U;
    TEST_ASSERT_EQUAL_INT(
        XY_DEVICE_INVALID_PARAM,
        xy_icm20608_get_configuration(&priv->device, &configuration));
    TEST_ASSERT_EQUAL_MEMORY(&sentinel, &configuration, sizeof(configuration));
    priv->device.configuration_synchronized = 1U;

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, icm20608_get_configuration(accel, &configuration));
    TEST_ASSERT_EQUAL_UINT16(100U, configuration.odr_hz);
    configuration = sentinel;
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_get_configuration(temp, &configuration));
    TEST_ASSERT_EQUAL_MEMORY(&sentinel, &configuration, sizeof(configuration));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_get_configuration(NULL, &configuration));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_get_configuration(accel, NULL));
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
    destroy_sensor(temp);
}

static void queue_i2c_configuration_readback(void *bus, uint8_t accel_config,
                                             uint8_t gyro_config, uint8_t gyro_dlpf,
                                             uint8_t accel_dlpf, uint8_t divider,
                                             uint8_t power, uint8_t power2,
                                             uint8_t int_enable)
{
    const uint8_t whoami = ICM20608_WHOAMI_VALUE;

    queue_i2c_read(bus, ICM20608_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_i2c_read(bus, ICM20608_REG_ACCEL_CONFIG, &accel_config, 1U, SENSOR_EOK);
    queue_i2c_read(bus, ICM20608_REG_GYRO_CONFIG, &gyro_config, 1U, SENSOR_EOK);
    queue_i2c_read(bus, ICM20608_REG_CONFIG, &gyro_dlpf, 1U, SENSOR_EOK);
    queue_i2c_read(bus, ICM20608_REG_ACCEL_CONFIG2, &accel_dlpf, 1U, SENSOR_EOK);
    queue_i2c_read(bus, ICM20608_REG_SMPLRT_DIV, &divider, 1U, SENSOR_EOK);
    queue_i2c_read(bus, ICM20608_REG_PWR_MGMT_1, &power, 1U, SENSOR_EOK);
    queue_i2c_read(bus, ICM20608_REG_PWR_MGMT_2, &power2, 1U, SENSOR_EOK);
    queue_i2c_read(bus, ICM20608_REG_INT_ENABLE, &int_enable, 1U, SENSOR_EOK);
}

static void test_icm20608_configuration_verification_detects_hardware_drift(void)
{
    int fake_bus;
    const uint8_t accel_config = 0x08U;
    const uint8_t gyro_config = 0x08U;
    const uint8_t gyro_dlpf = 0xE4U;
    const uint8_t accel_dlpf = 0xB4U;
    const uint8_t divider = 0x09U;
    const uint8_t power = 0x01U;
    const uint8_t power2 = 0x00U;
    const uint8_t int_enable = 0xA0U;
    const uint8_t drifted_accel_config = 0xB0U;
    xy_icm20608_accel_t sample = {11, 22, 33};
    const xy_icm20608_accel_t sample_snapshot = sample;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    sensor_device_t *temp = icm20608_create_temp("icm-temp", &fake_bus, false);
    icm20608_priv_t *priv;

    TEST_ASSERT_NOT_NULL(accel);
    TEST_ASSERT_NOT_NULL(temp);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    priv = (icm20608_priv_t *)accel->priv_data;

    queue_i2c_configuration_readback(&fake_bus, accel_config, gyro_config, gyro_dlpf,
                                     accel_dlpf, divider, power, power2, int_enable);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, icm20608_verify_configuration(accel));
    TEST_ASSERT_TRUE(priv->device.configuration_synchronized);

    {
        const uint8_t whoami = ICM20608_WHOAMI_VALUE;
        queue_i2c_read(&fake_bus, ICM20608_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    }
    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_CONFIG, &drifted_accel_config, 1U,
                   SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, icm20608_verify_configuration(accel));
    TEST_ASSERT_FALSE(priv->device.configuration_synchronized);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_icm20608_read_accel(&priv->device, &sample));
    TEST_ASSERT_EQUAL_MEMORY(&sample_snapshot, &sample, sizeof(sample));

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_verify_configuration(temp));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, icm20608_verify_configuration(NULL));
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
    destroy_sensor(temp);
}

static void test_icm20608_configuration_verification_propagates_transport_error(void)
{
    int fake_bus;
    const uint8_t whoami = ICM20608_WHOAMI_VALUE;
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    icm20608_priv_t *priv;

    TEST_ASSERT_NOT_NULL(accel);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    priv = (icm20608_priv_t *)accel->priv_data;

    queue_i2c_read(&fake_bus, ICM20608_REG_WHOAMI, &whoami, 1U, SENSOR_ETIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, icm20608_verify_configuration(accel));
    TEST_ASSERT_TRUE(priv->device.configuration_synchronized);
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
}

static void test_icm20608_configuration_verification_rejects_identity_and_power_drift(void)
{
    int fake_bus;
    const uint8_t wrong_whoami = 0xAFU;
    const uint8_t accel_config = 0x08U;
    const uint8_t gyro_config = 0x08U;
    const uint8_t gyro_dlpf = 0x04U;
    const uint8_t accel_dlpf = 0x04U;
    const uint8_t divider = 0x09U;
    const uint8_t power = 0x01U;
    const uint8_t disabled_axis = 0x01U;
    const uint8_t int_enable = 0x00U;
    sensor_device_t *identity = icm20608_create_accel("icm-id", &fake_bus, false);
    sensor_device_t *power_state = icm20608_create_accel("icm-power", &fake_bus, false);
    icm20608_priv_t *identity_priv;
    icm20608_priv_t *power_priv;

    TEST_ASSERT_NOT_NULL(identity);
    TEST_ASSERT_NOT_NULL(power_state);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, identity->ops->init(identity));
    identity_priv = (icm20608_priv_t *)identity->priv_data;
    queue_i2c_read(&fake_bus, ICM20608_REG_WHOAMI, &wrong_whoami, 1U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, icm20608_verify_configuration(identity));
    TEST_ASSERT_FALSE(identity_priv->device.configuration_synchronized);

    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, power_state->ops->init(power_state));
    power_priv = (icm20608_priv_t *)power_state->priv_data;
    queue_i2c_configuration_readback(&fake_bus, accel_config, gyro_config, gyro_dlpf,
                                     accel_dlpf, divider, power, disabled_axis, int_enable);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, icm20608_verify_configuration(power_state));
    TEST_ASSERT_FALSE(power_priv->device.configuration_synchronized);
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count - 1U, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(identity);
    destroy_sensor(power_state);
}

static void test_icm20608_configuration_verification_rejects_filter_bypass_drift(void)
{
    int fake_bus;
    const uint8_t whoami = ICM20608_WHOAMI_VALUE;
    const uint8_t accel_config = 0x08U;
    const uint8_t gyro_bypass = 0x09U;
    const uint8_t gyro_config = 0x08U;
    const uint8_t gyro_dlpf = 0x04U;
    const uint8_t accel_bypass = 0x0CU;
    sensor_device_t *gyro_drift = icm20608_create_accel("icm-gyro-bypass", &fake_bus, false);
    sensor_device_t *accel_drift = icm20608_create_accel("icm-accel-bypass", &fake_bus, false);
    icm20608_priv_t *gyro_priv;
    icm20608_priv_t *accel_priv;

    TEST_ASSERT_NOT_NULL(gyro_drift);
    TEST_ASSERT_NOT_NULL(accel_drift);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, gyro_drift->ops->init(gyro_drift));
    gyro_priv = (icm20608_priv_t *)gyro_drift->priv_data;
    queue_i2c_read(&fake_bus, ICM20608_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_CONFIG, &accel_config, 1U, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_GYRO_CONFIG, &gyro_bypass, 1U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, icm20608_verify_configuration(gyro_drift));
    TEST_ASSERT_FALSE(gyro_priv->device.configuration_synchronized);

    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel_drift->ops->init(accel_drift));
    accel_priv = (icm20608_priv_t *)accel_drift->priv_data;
    queue_i2c_read(&fake_bus, ICM20608_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_CONFIG, &accel_config, 1U, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_GYRO_CONFIG, &gyro_config, 1U, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_CONFIG, &gyro_dlpf, 1U, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_CONFIG2, &accel_bypass, 1U,
                   SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, icm20608_verify_configuration(accel_drift));
    TEST_ASSERT_FALSE(accel_priv->device.configuration_synchronized);

    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(gyro_drift);
    destroy_sensor(accel_drift);
}

static void test_icm20608_configuration_verification_rejects_self_test_drift(void)
{
    int fake_bus;
    const uint8_t whoami = ICM20608_WHOAMI_VALUE;
    const uint8_t accel_self_test = 0x28U;
    const uint8_t accel_config = 0x08U;
    const uint8_t gyro_self_test = 0x28U;
    sensor_device_t *accel_drift = icm20608_create_accel("icm-accel-self-test", &fake_bus, false);
    sensor_device_t *gyro_drift = icm20608_create_accel("icm-gyro-self-test", &fake_bus, false);
    icm20608_priv_t *accel_priv;
    icm20608_priv_t *gyro_priv;

    TEST_ASSERT_NOT_NULL(accel_drift);
    TEST_ASSERT_NOT_NULL(gyro_drift);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel_drift->ops->init(accel_drift));
    accel_priv = (icm20608_priv_t *)accel_drift->priv_data;
    queue_i2c_read(&fake_bus, ICM20608_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_CONFIG, &accel_self_test, 1U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, icm20608_verify_configuration(accel_drift));
    TEST_ASSERT_FALSE(accel_priv->device.configuration_synchronized);

    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, gyro_drift->ops->init(gyro_drift));
    gyro_priv = (icm20608_priv_t *)gyro_drift->priv_data;
    queue_i2c_read(&fake_bus, ICM20608_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_ACCEL_CONFIG, &accel_config, 1U, SENSOR_EOK);
    queue_i2c_read(&fake_bus, ICM20608_REG_GYRO_CONFIG, &gyro_self_test, 1U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, icm20608_verify_configuration(gyro_drift));
    TEST_ASSERT_FALSE(gyro_priv->device.configuration_synchronized);

    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel_drift);
    destroy_sensor(gyro_drift);
}

static void test_icm20608_range_change_rejects_incompatible_bias_without_bus_access(void)
{
    int fake_bus;
    const xy_icm20608_accel_t accel_bias = {3000, 0, 0};
    const xy_icm20608_gyro_t gyro_bias = {400000, 0, 0};
    sensor_device_t *accel = icm20608_create_accel("icm-acc", &fake_bus, false);
    sensor_device_t *gyro = icm20608_create_gyro("icm-gyro", &fake_bus, false);
    icm20608_priv_t *accel_priv;
    icm20608_priv_t *gyro_priv;

    TEST_ASSERT_NOT_NULL(accel);
    TEST_ASSERT_NOT_NULL(gyro);
    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, accel->ops->init(accel));
    accel_priv = (icm20608_priv_t *)accel->priv_data;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_icm20608_set_bias(&accel_priv->device, &accel_bias,
                                              &(xy_icm20608_gyro_t){0, 0, 0}));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL,
                          icm20608_set_accel_range(accel, XY_ICM20608_ACCEL_RANGE_2G));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_ACCEL_RANGE_4G, accel_priv->device.accel_range);
    TEST_ASSERT_EQUAL_INT32(-4000, accel->info.range_min);
    TEST_ASSERT_EQUAL_INT32(4000, accel->info.range_max);

    queue_i2c_init_success(&fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, gyro->ops->init(gyro));
    gyro_priv = (icm20608_priv_t *)gyro->priv_data;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_icm20608_set_bias(&gyro_priv->device,
                                              &(xy_icm20608_accel_t){0, 0, 0},
                                              &gyro_bias));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL,
                          icm20608_set_gyro_range(gyro, XY_ICM20608_GYRO_RANGE_250DPS));
    TEST_ASSERT_EQUAL_INT(XY_ICM20608_GYRO_RANGE_500DPS, gyro_priv->device.gyro_range);
    TEST_ASSERT_EQUAL_INT32(-500, gyro->info.range_min);
    TEST_ASSERT_EQUAL_INT32(500, gyro->info.range_max);

    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(g_i2c_write_count, g_i2c_write_index);
    destroy_sensor(accel);
    destroy_sensor(gyro);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_icm20608_create_identity_and_bus_contracts);
    RUN_TEST(test_icm20608_i2c_init_read_deinit_contracts);
    RUN_TEST(test_icm20608_failure_contracts_preserve_output);
    RUN_TEST(test_icm20608_accepts_pandora_identity);
    RUN_TEST(test_icm20608_spi_bus_path_smoke);
    RUN_TEST(test_icm20608_propagates_first_transport_error);
    RUN_TEST(test_icm20608_runtime_ranges_update_scaling_and_metadata);
    RUN_TEST(test_icm20608_runtime_odr_is_exact_and_failure_atomic);
    RUN_TEST(test_icm20608_power_mode_preserves_register_and_cache_on_failure);
    RUN_TEST(test_icm20608_data_ready_interrupt_preserves_register_and_reports_status);
    RUN_TEST(test_icm20608_coherent_sample_is_single_burst_and_failure_atomic);
    RUN_TEST(test_icm20608_sleep_blocks_all_sample_reads_without_bus_access);
    RUN_TEST(test_icm20608_failed_reinit_preserves_live_owner);
    RUN_TEST(test_icm20608_deinit_preserves_power_bits_and_owner_on_failure);
    RUN_TEST(test_icm20608_dlpf_control_preserves_bits_and_rolls_back);
    RUN_TEST(test_icm20608_failed_dlpf_rollback_fail_closes_until_reinit);
    RUN_TEST(test_icm20608_wrapper_dlpf_control_maps_errors_and_validates_type);
    RUN_TEST(test_icm20608_bias_is_applied_to_all_sample_paths);
    RUN_TEST(test_icm20608_wrapper_bias_validates_type_and_preserves_state);
    RUN_TEST(test_icm20608_bias_getters_preserve_outputs_on_rejection);
    RUN_TEST(test_icm20608_configuration_readback_is_staged_and_guarded);
    RUN_TEST(test_icm20608_configuration_verification_detects_hardware_drift);
    RUN_TEST(test_icm20608_configuration_verification_propagates_transport_error);
    RUN_TEST(test_icm20608_configuration_verification_rejects_identity_and_power_drift);
    RUN_TEST(test_icm20608_configuration_verification_rejects_filter_bypass_drift);
    RUN_TEST(test_icm20608_configuration_verification_rejects_self_test_drift);
    RUN_TEST(test_icm20608_range_change_rejects_incompatible_bias_without_bus_access);
    return UNITY_END();
}
