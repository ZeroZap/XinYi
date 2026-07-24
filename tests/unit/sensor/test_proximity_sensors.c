#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_pa122.h"
#include "sensor_vcnl4040.h"

#define I2C_QUEUE_MAX 8U

typedef struct {
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[4];
    uint16_t len;
    int ret;
} i2c_read_op_t;

static uint32_t g_tick;
static i2c_read_op_t g_i2c_reads[I2C_QUEUE_MAX];
static unsigned int g_i2c_read_count;
static unsigned int g_i2c_read_index;
static unsigned int g_i2c_write_count;
static int g_i2c_unexpected;

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
    g_i2c_read_count = 0;
    g_i2c_read_index = 0;
    g_i2c_write_count = 0;
    g_i2c_unexpected = 0;
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

static void assert_no_extra_i2c(void)
{
    TEST_ASSERT_EQUAL_UINT(g_i2c_read_count, g_i2c_read_index);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_unexpected);
}

static void test_pa122_create_init_and_near_far_threshold(void)
{
    int bus;
    sensor_device_t *sensor = pa122_create("pa122-main", &bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("pa122-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Perela", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("PA122", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PROXIMITY, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_EQUAL_PTR(&bus, sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_EQUAL_UINT8(PA122_ADDR, ((pa122_priv_t *)sensor->priv_data)->i2c_addr);

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_read_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_write_count);

    sensor_data_t data = {0};
    uint8_t near_status = 0x80U;
    queue_i2c_read(&bus, PA122_ADDR, 0x08U, &near_status, 1U, 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PROXIMITY, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    assert_no_extra_i2c();

    g_tick = 271828U;
    uint8_t far_status = 0x00U;
    queue_i2c_read(&bus, PA122_ADDR, 0x08U, &far_status, 1U, 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 200.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    assert_no_extra_i2c();

    destroy_sensor(sensor);
}

static void test_vcnl4040_create_and_little_endian_proximity_read(void)
{
    int bus;
    sensor_device_t *sensor = vcnl4040_create("vcnl4040-main", &bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("vcnl4040-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Vishay", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("VCNL4040", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PROXIMITY, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_EQUAL_PTR(&bus, sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_EQUAL_UINT8(VCNL4040_ADDR, ((vcnl4040_priv_t *)sensor->priv_data)->i2c_addr);

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_read_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_write_count);

    sensor_data_t data = {0};
    uint8_t raw[] = {0x34U, 0x12U};
    queue_i2c_read(&bus, VCNL4040_ADDR, 0x08U, raw, sizeof(raw), 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PROXIMITY, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4660.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    assert_no_extra_i2c();

    destroy_sensor(sensor);
}

static void test_create_rejects_null_names_without_i2c_side_effects(void)
{
    int bus;

    TEST_ASSERT_NULL(pa122_create(NULL, &bus));
    TEST_ASSERT_NULL(vcnl4040_create(NULL, &bus));
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_read_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_write_count);
}

static void test_long_names_are_truncated_with_terminator(void)
{
    int bus;
    char long_name[SENSOR_NAME_MAX_LEN * 2U];
    memset(long_name, 'P', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = '\0';

    sensor_device_t *pa122 = pa122_create(long_name, &bus);
    sensor_device_t *vcnl4040 = vcnl4040_create(long_name, &bus);

    TEST_ASSERT_NOT_NULL(pa122);
    TEST_ASSERT_NOT_NULL(vcnl4040);
    TEST_ASSERT_EQUAL_UINT8('\0', pa122->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', vcnl4040->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(pa122->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(vcnl4040->info.name));

    destroy_sensor(pa122);
    destroy_sensor(vcnl4040);
}

static void test_public_ops_guard_null_inputs_without_i2c_side_effects(void)
{
    int bus;
    sensor_device_t *pa122 = pa122_create("pa122-guard", &bus);
    sensor_device_t *vcnl4040 = vcnl4040_create("vcnl4040-guard", &bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_float = 123.0f, .timestamp = 42U};
    sensor_data_t snapshot = data;

    TEST_ASSERT_NOT_NULL(pa122);
    TEST_ASSERT_NOT_NULL(vcnl4040);
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, pa122->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, vcnl4040->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, pa122->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, pa122->ops->read(pa122, NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, vcnl4040->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, vcnl4040->ops->read(vcnl4040, NULL));
    TEST_ASSERT_EQUAL_INT(snapshot.type, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, snapshot.value.val_float, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(snapshot.timestamp, data.timestamp);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_read_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_write_count);

    destroy_sensor(pa122);
    destroy_sensor(vcnl4040);
}

static void test_i2c_read_failures_preserve_output(void)
{
    int bus;
    sensor_device_t *pa122 = pa122_create("pa122-fail", &bus);
    sensor_device_t *vcnl4040 = vcnl4040_create("vcnl4040-fail", &bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_float = 77.0f, .timestamp = 11U};
    sensor_data_t snapshot = data;

    TEST_ASSERT_NOT_NULL(pa122);
    TEST_ASSERT_NOT_NULL(vcnl4040);

    queue_i2c_read(&bus, PA122_ADDR, 0x08U, NULL, 1U, -5);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, pa122->ops->read(pa122, &data));
    TEST_ASSERT_EQUAL_INT(snapshot.type, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, snapshot.value.val_float, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(snapshot.timestamp, data.timestamp);

    queue_i2c_read(&bus, VCNL4040_ADDR, 0x08U, NULL, 2U, -6);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, vcnl4040->ops->read(vcnl4040, &data));
    TEST_ASSERT_EQUAL_INT(snapshot.type, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, snapshot.value.val_float, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(snapshot.timestamp, data.timestamp);
    assert_no_extra_i2c();

    destroy_sensor(pa122);
    destroy_sensor(vcnl4040);
}

static void test_create_accepts_null_bus_and_read_uses_registered_bus_pointer(void)
{
    sensor_device_t *pa122 = pa122_create("pa122-null-bus", NULL);
    sensor_device_t *vcnl4040 = vcnl4040_create("vcnl4040-null-bus", NULL);
    sensor_data_t data = {0};

    TEST_ASSERT_NOT_NULL(pa122);
    TEST_ASSERT_NOT_NULL(vcnl4040);
    TEST_ASSERT_NULL(pa122->bus);
    TEST_ASSERT_NULL(vcnl4040->bus);

    uint8_t pa122_near = 0x80U;
    queue_i2c_read(NULL, PA122_ADDR, 0x08U, &pa122_near, 1U, 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, pa122->ops->read(pa122, &data));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    g_tick = 123456U;
    uint8_t vcnl_raw[] = {0xFFU, 0xFFU};
    queue_i2c_read(NULL, VCNL4040_ADDR, 0x08U, vcnl_raw, sizeof(vcnl_raw), 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, vcnl4040->ops->read(vcnl4040, &data));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 65535.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    assert_no_extra_i2c();

    destroy_sensor(pa122);
    destroy_sensor(vcnl4040);
}

static void test_missing_private_data_is_rejected_without_i2c_side_effects(void)
{
    int bus;
    sensor_device_t *pa122 = pa122_create("pa122-no-priv", &bus);
    sensor_device_t *vcnl4040 = vcnl4040_create("vcnl4040-no-priv", &bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_float = -9.0f, .timestamp = 55U};
    sensor_data_t snapshot = data;

    TEST_ASSERT_NOT_NULL(pa122);
    TEST_ASSERT_NOT_NULL(vcnl4040);

    SENSOR_FREE(pa122->priv_data);
    pa122->priv_data = NULL;
    SENSOR_FREE(vcnl4040->priv_data);
    vcnl4040->priv_data = NULL;

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, pa122->ops->init(pa122));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, pa122->ops->read(pa122, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, vcnl4040->ops->init(vcnl4040));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, vcnl4040->ops->read(vcnl4040, &data));
    TEST_ASSERT_EQUAL_INT(snapshot.type, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, snapshot.value.val_float, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(snapshot.timestamp, data.timestamp);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_read_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_write_count);

    destroy_sensor(pa122);
    destroy_sensor(vcnl4040);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pa122_create_init_and_near_far_threshold);
    RUN_TEST(test_vcnl4040_create_and_little_endian_proximity_read);
    RUN_TEST(test_create_rejects_null_names_without_i2c_side_effects);
    RUN_TEST(test_long_names_are_truncated_with_terminator);
    RUN_TEST(test_public_ops_guard_null_inputs_without_i2c_side_effects);
    RUN_TEST(test_i2c_read_failures_preserve_output);
    RUN_TEST(test_create_accepts_null_bus_and_read_uses_registered_bus_pointer);
    RUN_TEST(test_missing_private_data_is_rejected_without_i2c_side_effects);
    return UNITY_END();
}
