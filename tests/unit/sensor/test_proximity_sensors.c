#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_vcnl4040.h"

#define I2C_QUEUE_MAX 4U

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[2];
    uint16_t len;
    int ret;
} i2c_read_op_t;

static uint32_t g_tick;
static i2c_read_op_t g_i2c_reads[I2C_QUEUE_MAX];
static unsigned int g_i2c_read_count;
static unsigned int g_i2c_read_index;
static unsigned int g_i2c_write_count;
static unsigned int g_i2c_unexpected;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

static void queue_i2c_read(void *bus, uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len,
                           int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(I2C_QUEUE_MAX, g_i2c_read_count);
    i2c_read_op_t *op = &g_i2c_reads[g_i2c_read_count++];
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = len;
    op->ret = ret;
    memset(op->data, 0, sizeof(op->data));
    if (data != NULL && len > 0U) {
        TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(op->data), len);
        memcpy(op->data, data, len);
    }
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (g_i2c_read_index >= g_i2c_read_count) {
        g_i2c_unexpected++;
        return -99;
    }
    const i2c_read_op_t *op = &g_i2c_reads[g_i2c_read_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    if (op->ret == 0 && data != NULL && len > 0U) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)addr;
    (void)reg;
    (void)data;
    (void)len;
    g_i2c_write_count++;
    return 0;
}

void setUp(void)
{
    g_tick = 314159U;
    memset(g_i2c_reads, 0, sizeof(g_i2c_reads));
    g_i2c_read_count = 0U;
    g_i2c_read_index = 0U;
    g_i2c_write_count = 0U;
    g_i2c_unexpected = 0U;
}

void tearDown(void) {}

static void destroy_sensor(sensor_device_t *sensor)
{
    if (sensor != NULL) {
        SENSOR_FREE(sensor->priv_data);
        SENSOR_FREE(sensor);
    }
}

static void assert_no_extra_i2c(void)
{
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_unexpected);
}

static void test_vcnl4040_create_and_little_endian_proximity_read(void)
{
    int bus;
    sensor_device_t *sensor = vcnl4040_create("vcnl4040-main", &bus);
    sensor_data_t data = {0};
    uint8_t raw[] = {0x34U, 0x12U};

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("vcnl4040-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Vishay", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("VCNL4040", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PROXIMITY, sensor->info.type);
    TEST_ASSERT_EQUAL_UINT8(VCNL4040_ADDR, ((vcnl4040_priv_t *)sensor->priv_data)->i2c_addr);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));

    queue_i2c_read(&bus, VCNL4040_ADDR, 0x08U, raw, sizeof(raw), 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PROXIMITY, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4660.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    assert_no_extra_i2c();
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));
    destroy_sensor(sensor);
}

static void test_vcnl4040_guards_and_transport_failure_preserve_output(void)
{
    int bus;
    sensor_device_t *sensor = vcnl4040_create("vcnl4040-guard", &bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_float = 77.0f, .timestamp = 11U};
    sensor_data_t snapshot = data;

    TEST_ASSERT_NULL(vcnl4040_create(NULL, &bus));
    TEST_ASSERT_NULL(vcnl4040_create("vcnl4040-null-bus", NULL));
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, NULL));
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_read_count);

    queue_i2c_read(&bus, VCNL4040_ADDR, 0x08U, NULL, 2U, SENSOR_EBUSY);
    TEST_ASSERT_EQUAL_INT(SENSOR_EBUSY, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
    assert_no_extra_i2c();
    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_vcnl4040_create_and_little_endian_proximity_read);
    RUN_TEST(test_vcnl4040_guards_and_transport_failure_preserve_output);
    return UNITY_END();
}
