#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_ak09918.h"
#include "sensor_ist8310.h"
#include "sensor_qmc5883l.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t bytes[8];
    uint16_t len;
    int ret;
} i2c_op_t;

static i2c_op_t g_reads[16];
static i2c_op_t g_writes[16];
static size_t g_read_count;
static size_t g_read_index;
static size_t g_write_count;
static size_t g_write_index;
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
    TEST_ASSERT_LESS_THAN_UINT(g_read_count, g_read_index);
    i2c_op_t *op = &g_reads[g_read_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (op->ret == SENSOR_EOK) {
        memcpy(data, op->bytes, len);
    }
    return op->ret;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_THAN_UINT(g_write_count, g_write_index);
    i2c_op_t *op = &g_writes[g_write_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_MEMORY(op->bytes, data, len);
    return op->ret;
}

static void queue_read(void *bus, uint8_t addr, uint8_t reg, const uint8_t *bytes, uint16_t len,
                       int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_count);
    i2c_op_t *op = &g_reads[g_read_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = len;
    op->ret = ret;
    if (bytes != NULL) {
        memcpy(op->bytes, bytes, len);
    }
}

static void queue_write(void *bus, uint8_t addr, uint8_t reg, uint8_t byte, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_writes), g_write_count);
    i2c_op_t *op = &g_writes[g_write_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = 1U;
    op->bytes[0] = byte;
    op->ret = ret;
}

void setUp(void)
{
    memset(g_reads, 0, sizeof(g_reads));
    memset(g_writes, 0, sizeof(g_writes));
    g_read_count = 0;
    g_read_index = 0;
    g_write_count = 0;
    g_write_index = 0;
    g_tick = 13579U;
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

static void assert_queues_drained(void)
{
    TEST_ASSERT_EQUAL_UINT(g_read_count, g_read_index);
    TEST_ASSERT_EQUAL_UINT(g_write_count, g_write_index);
}

static void test_qmc5883l_create_init_read_and_deinit_contracts(void)
{
    int fake_bus;
    const uint8_t ready = 0x01U;
    const uint8_t raw[6] = {0xE8U, 0x03U, 0x18U, 0xFCU, 0xD0U, 0x07U};
    sensor_data_t data = {0};
    sensor_device_t *sensor = qmc5883l_create("qmc5883l-main", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("qmc5883l-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("QST", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("QMC5883L", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_MAGNETOMETER, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MICRO_TESLA, sensor->info.unit);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_EQUAL_UINT8(QMC5883L_ADDR_DEFAULT, ((qmc5883l_priv_t *)sensor->priv_data)->i2c_addr);

    queue_write(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_CONTROL2, 0x80U, SENSOR_EOK);
    queue_write(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_CONTROL1, 0x0DU, SENSOR_EOK);
    queue_write(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_PERIOD, 0x01U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(13589U, g_tick);

    queue_read(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_STATUS, &ready, 1U, SENSOR_EOK);
    queue_read(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_DATA_X_LSB, raw, sizeof(raw),
               SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_MAGNETOMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MICRO_TESLA, data.unit);
    TEST_ASSERT_EQUAL_INT32(12, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-12, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(24, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(92, data.accuracy);

    queue_write(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_CONTROL1, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));

    assert_queues_drained();
    destroy_sensor(sensor);
}

static void test_qmc5883l_io_failures_stop_and_preserve_outputs(void)
{
    int fake_bus;
    const uint8_t not_ready = 0x00U;
    sensor_data_t data = {.type = SENSOR_TYPE_GYROSCOPE, .timestamp = 42U};
    sensor_device_t *sensor = qmc5883l_create("qmc5883l-fail", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);

    queue_write(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_CONTROL2, 0x80U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    queue_write(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_CONTROL2, 0x80U, SENSOR_EOK);
    queue_write(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_CONTROL1, 0x0DU, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    queue_write(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_CONTROL2, 0x80U, SENSOR_EOK);
    queue_write(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_CONTROL1, 0x0DU, SENSOR_EOK);
    queue_write(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_PERIOD, 0x01U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    queue_read(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_STATUS, &not_ready, 1U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EBUSY, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GYROSCOPE, data.type);
    TEST_ASSERT_EQUAL_UINT32(42U, data.timestamp);

    queue_write(&fake_bus, QMC5883L_ADDR_DEFAULT, QMC5883L_REG_CONTROL1, 0x00U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->deinit(sensor));

    assert_queues_drained();
    destroy_sensor(sensor);
}

static void test_ist8310_create_init_read_and_deinit_contracts(void)
{
    int fake_bus;
    const uint8_t whoami = IST8310_WHOAMI_VALUE;
    const uint8_t raw[6] = {0x01U, 0x00U, 0xFEU, 0xFFU, 0x10U, 0x00U};
    sensor_data_t data = {0};
    sensor_device_t *sensor = ist8310_create("ist8310-main", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("ist8310-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("iSentek", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("IST8310", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_MAGNETOMETER, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MICRO_TESLA, sensor->info.unit);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_EQUAL_UINT8(IST8310_ADDR_DEFAULT,
                            ((ist8310_priv_t *)sensor->priv_data)->i2c_addr);

    queue_read(&fake_bus, IST8310_ADDR_DEFAULT, IST8310_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_write(&fake_bus, IST8310_ADDR_DEFAULT, IST8310_REG_CTRL1, 0x1AU, SENSOR_EOK);
    queue_write(&fake_bus, IST8310_ADDR_DEFAULT, IST8310_REG_CTRL2, 0x40U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));

    for (uint8_t i = 0; i < sizeof(raw); ++i) {
        queue_read(&fake_bus, IST8310_ADDR_DEFAULT, (uint8_t)(IST8310_REG_DATA + i), &raw[i], 1U,
                   SENSOR_EOK);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_MAGNETOMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MICRO_TESLA, data.unit);
    TEST_ASSERT_EQUAL_INT32(15, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-30, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(240, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(85, data.accuracy);

    queue_write(&fake_bus, IST8310_ADDR_DEFAULT, IST8310_REG_CTRL1, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));

    assert_queues_drained();
    destroy_sensor(sensor);
}

static void test_ist8310_io_failures_stop_and_preserve_outputs(void)
{
    int fake_bus;
    const uint8_t bad_whoami = 0x00U;
    const uint8_t whoami = IST8310_WHOAMI_VALUE;
    const uint8_t raw0 = 0x7FU;
    sensor_data_t data = {.type = SENSOR_TYPE_GYROSCOPE, .timestamp = 77U};
    sensor_device_t *sensor = ist8310_create("ist8310-fail", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);

    queue_read(&fake_bus, IST8310_ADDR_DEFAULT, IST8310_REG_WHOAMI, NULL, 1U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    queue_read(&fake_bus, IST8310_ADDR_DEFAULT, IST8310_REG_WHOAMI, &bad_whoami, 1U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));

    queue_read(&fake_bus, IST8310_ADDR_DEFAULT, IST8310_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_write(&fake_bus, IST8310_ADDR_DEFAULT, IST8310_REG_CTRL1, 0x1AU, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    queue_read(&fake_bus, IST8310_ADDR_DEFAULT, IST8310_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_write(&fake_bus, IST8310_ADDR_DEFAULT, IST8310_REG_CTRL1, 0x1AU, SENSOR_EOK);
    queue_write(&fake_bus, IST8310_ADDR_DEFAULT, IST8310_REG_CTRL2, 0x40U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    queue_read(&fake_bus, IST8310_ADDR_DEFAULT, IST8310_REG_DATA, &raw0, 1U, SENSOR_EOK);
    queue_read(&fake_bus, IST8310_ADDR_DEFAULT, (uint8_t)(IST8310_REG_DATA + 1U), NULL, 1U,
               SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GYROSCOPE, data.type);
    TEST_ASSERT_EQUAL_UINT32(77U, data.timestamp);

    queue_write(&fake_bus, IST8310_ADDR_DEFAULT, IST8310_REG_CTRL1, 0x00U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->deinit(sensor));

    assert_queues_drained();
    destroy_sensor(sensor);
}

static void test_ak09918_create_init_read_and_deinit_contracts(void)
{
    int fake_bus;
    const uint8_t whoami = AK09918_WHOAMI_VALUE;
    const uint8_t raw[6] = {0x02U, 0x00U, 0xFDU, 0xFFU, 0x20U, 0x00U};
    sensor_data_t data = {0};
    sensor_device_t *sensor = ak09918_create("ak09918-main", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("ak09918-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("AKM", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("AK09918", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_MAGNETOMETER, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MICRO_TESLA, sensor->info.unit);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_EQUAL_UINT8(AK09918_ADDR_DEFAULT,
                            ((ak09918_priv_t *)sensor->priv_data)->i2c_addr);

    queue_read(&fake_bus, AK09918_ADDR_DEFAULT, AK09918_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_write(&fake_bus, AK09918_ADDR_DEFAULT, AK09918_REG_CTRL2, 0x08U, SENSOR_EOK);
    queue_write(&fake_bus, AK09918_ADDR_DEFAULT, AK09918_REG_CTRL1, 0x1FU, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(13629U, g_tick);

    for (uint8_t i = 0; i < sizeof(raw); ++i) {
        queue_read(&fake_bus, AK09918_ADDR_DEFAULT, (uint8_t)(AK09918_REG_DATA + i), &raw[i], 1U,
                   SENSOR_EOK);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_MAGNETOMETER, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_MICRO_TESLA, data.unit);
    TEST_ASSERT_EQUAL_INT32(30, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(-45, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(480, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(90, data.accuracy);

    queue_write(&fake_bus, AK09918_ADDR_DEFAULT, AK09918_REG_CTRL1, 0x00U, SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));

    assert_queues_drained();
    destroy_sensor(sensor);
}

static void test_ak09918_io_failures_stop_and_preserve_outputs(void)
{
    int fake_bus;
    const uint8_t bad_whoami = 0x00U;
    const uint8_t whoami = AK09918_WHOAMI_VALUE;
    const uint8_t raw0 = 0x7FU;
    sensor_data_t data = {.type = SENSOR_TYPE_GYROSCOPE, .timestamp = 88U};
    sensor_device_t *sensor = ak09918_create("ak09918-fail", &fake_bus);

    TEST_ASSERT_NULL(ak09918_create(NULL, &fake_bus));
    TEST_ASSERT_NOT_NULL(sensor);

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GYROSCOPE, data.type);
    TEST_ASSERT_EQUAL_UINT32(88U, data.timestamp);

    queue_read(&fake_bus, AK09918_ADDR_DEFAULT, AK09918_REG_WHOAMI, NULL, 1U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    queue_read(&fake_bus, AK09918_ADDR_DEFAULT, AK09918_REG_WHOAMI, &bad_whoami, 1U,
               SENSOR_EOK);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));

    queue_read(&fake_bus, AK09918_ADDR_DEFAULT, AK09918_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_write(&fake_bus, AK09918_ADDR_DEFAULT, AK09918_REG_CTRL2, 0x08U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    queue_read(&fake_bus, AK09918_ADDR_DEFAULT, AK09918_REG_WHOAMI, &whoami, 1U, SENSOR_EOK);
    queue_write(&fake_bus, AK09918_ADDR_DEFAULT, AK09918_REG_CTRL2, 0x08U, SENSOR_EOK);
    queue_write(&fake_bus, AK09918_ADDR_DEFAULT, AK09918_REG_CTRL1, 0x1FU, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    queue_read(&fake_bus, AK09918_ADDR_DEFAULT, AK09918_REG_DATA, &raw0, 1U, SENSOR_EOK);
    queue_read(&fake_bus, AK09918_ADDR_DEFAULT, (uint8_t)(AK09918_REG_DATA + 1U), NULL, 1U,
               SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GYROSCOPE, data.type);
    TEST_ASSERT_EQUAL_UINT32(88U, data.timestamp);

    queue_write(&fake_bus, AK09918_ADDR_DEFAULT, AK09918_REG_CTRL1, 0x00U, SENSOR_EIO);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->deinit(sensor));

    assert_queues_drained();
    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_qmc5883l_create_init_read_and_deinit_contracts);
    RUN_TEST(test_qmc5883l_io_failures_stop_and_preserve_outputs);
    RUN_TEST(test_ist8310_create_init_read_and_deinit_contracts);
    RUN_TEST(test_ist8310_io_failures_stop_and_preserve_outputs);
    RUN_TEST(test_ak09918_create_init_read_and_deinit_contracts);
    RUN_TEST(test_ak09918_io_failures_stop_and_preserve_outputs);
    return UNITY_END();
}
