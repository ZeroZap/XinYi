#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_vl53l1x.h"

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[2];
    uint16_t len;
    int ret;
} i2c_op_t;

static i2c_op_t g_reads[4];
static i2c_op_t g_writes[4];
static size_t g_read_count;
static size_t g_read_index;
static size_t g_write_count;
static size_t g_write_index;
static uint32_t g_tick;
static uint32_t g_delay_ms;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

void delay_ms(uint32_t ms)
{
    g_delay_ms += ms;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    i2c_op_t *op;
    TEST_ASSERT_LESS_THAN_UINT(g_read_count, g_read_index);
    op = &g_reads[g_read_index++];
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
    i2c_op_t *op;
    TEST_ASSERT_LESS_THAN_UINT(g_write_count, g_write_index);
    op = &g_writes[g_write_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_EQUAL_MEMORY(op->data, data, len);
    return op->ret;
}

static void queue_write(void *bus, uint8_t value, int ret)
{
    i2c_op_t *op = &g_writes[g_write_count++];
    memset(op, 0, sizeof(*op));
    op->bus = bus;
    op->addr = VL53L1X_ADDR_DEFAULT;
    op->reg = 0x2DU;
    op->data[0] = value;
    op->len = 1U;
    op->ret = ret;
}

static void queue_read(void *bus, const uint8_t data[2], int ret)
{
    i2c_op_t *op = &g_reads[g_read_count++];
    memset(op, 0, sizeof(*op));
    op->bus = bus;
    op->addr = VL53L1X_ADDR_DEFAULT;
    op->reg = 0x6EU;
    memcpy(op->data, data, 2U);
    op->len = 2U;
    op->ret = ret;
}

void setUp(void)
{
    memset(g_reads, 0, sizeof(g_reads));
    memset(g_writes, 0, sizeof(g_writes));
    g_read_count = 0U;
    g_read_index = 0U;
    g_write_count = 0U;
    g_write_index = 0U;
    g_tick = 123U;
    g_delay_ms = 0U;
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

static void test_factory_and_public_guard_contracts(void)
{
    int bus;
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_uint32 = 0xA5A5U,
                          .timestamp = 77U, .accuracy = 9U};
    sensor_data_t snapshot = data;
    sensor_device_t *sensor;

    TEST_ASSERT_NULL(vl53l1x_create(NULL, &bus, 0U));
    TEST_ASSERT_NULL(vl53l1x_create("vl53", NULL, 0U));
    sensor = vl53l1x_create("vl53", &bus, 0U);
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_NOT_NULL(sensor->ops->deinit);
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, NULL));

    sensor->bus = NULL;
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT(0U, g_read_index);
    TEST_ASSERT_EQUAL_UINT(0U, g_write_index);
    destroy_sensor(sensor);
}

static void test_init_propagates_each_write_failure(void)
{
    int bus;
    sensor_device_t *sensor = vl53l1x_create("vl53", &bus, 0U);
    TEST_ASSERT_NOT_NULL(sensor);

    queue_write(&bus, 0x00U, -31);
    TEST_ASSERT_EQUAL_INT(-31, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(0U, g_delay_ms);
    TEST_ASSERT_EQUAL_UINT(1U, g_write_index);

    queue_write(&bus, 0x00U, SENSOR_EOK);
    queue_write(&bus, 0x01U, -32);
    TEST_ASSERT_EQUAL_INT(-32, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(100U, g_delay_ms);
    TEST_ASSERT_EQUAL_UINT(3U, g_write_index);
    destroy_sensor(sensor);
}

static void test_read_failure_preserves_output_and_deinit_is_callable(void)
{
    int bus;
    const uint8_t raw[2] = {0x01U, 0x23U};
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_uint32 = 0x55AAU,
                          .timestamp = 88U, .accuracy = 7U};
    sensor_data_t snapshot = data;
    sensor_device_t *sensor = vl53l1x_create("vl53", &bus, 0U);
    TEST_ASSERT_NOT_NULL(sensor);

    queue_read(&bus, raw, -41);
    TEST_ASSERT_EQUAL_INT(-41, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));
    TEST_ASSERT_EQUAL_UINT(0U, g_write_index);
    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_factory_and_public_guard_contracts);
    RUN_TEST(test_init_propagates_each_write_failure);
    RUN_TEST(test_read_failure_preserves_output_and_deinit_is_callable);
    return UNITY_END();
}
