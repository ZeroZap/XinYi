#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_apds9960.h"

#define I2C_QUEUE_MAX 12U

typedef enum {
    I2C_OP_READ,
    I2C_OP_WRITE,
} i2c_op_kind_t;

typedef struct {
    i2c_op_kind_t kind;
    void *bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t data[16];
    uint16_t len;
    int ret;
} i2c_op_t;

static uint32_t g_tick;
static i2c_op_t g_i2c_ops[I2C_QUEUE_MAX];
static unsigned int g_i2c_count;
static unsigned int g_i2c_index;
static unsigned int g_i2c_unexpected;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

static void queue_i2c_read(void *bus, uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len,
                           int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(I2C_QUEUE_MAX, g_i2c_count);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_i2c_ops[0].data), len);
    i2c_op_t *op = &g_i2c_ops[g_i2c_count++];
    op->kind = I2C_OP_READ;
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->len = len;
    op->ret = ret;
    memset(op->data, 0, sizeof(op->data));
    if (data != NULL && len > 0U) {
        memcpy(op->data, data, len);
    }
}

static void queue_i2c_write(void *bus, uint8_t addr, uint8_t reg, uint8_t value, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(I2C_QUEUE_MAX, g_i2c_count);
    i2c_op_t *op = &g_i2c_ops[g_i2c_count++];
    op->kind = I2C_OP_WRITE;
    op->bus = bus;
    op->addr = addr;
    op->reg = reg;
    op->data[0] = value;
    op->len = 1U;
    op->ret = ret;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (g_i2c_index >= g_i2c_count || g_i2c_ops[g_i2c_index].kind != I2C_OP_READ) {
        g_i2c_unexpected++;
        return -99;
    }

    const i2c_op_t *op = &g_i2c_ops[g_i2c_index++];
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
    if (g_i2c_index >= g_i2c_count || g_i2c_ops[g_i2c_index].kind != I2C_OP_WRITE) {
        g_i2c_unexpected++;
        return -99;
    }

    const i2c_op_t *op = &g_i2c_ops[g_i2c_index++];
    TEST_ASSERT_EQUAL_PTR(op->bus, bus);
    TEST_ASSERT_EQUAL_UINT8(op->addr, addr);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT16(op->len, len);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT8(op->data[0], data[0]);
    return op->ret;
}

void setUp(void)
{
    g_tick = 876543U;
    memset(g_i2c_ops, 0, sizeof(g_i2c_ops));
    g_i2c_count = 0;
    g_i2c_index = 0;
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
    TEST_ASSERT_EQUAL_UINT(g_i2c_count, g_i2c_index);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_unexpected);
}

static void assert_output_unchanged(const sensor_data_t *snapshot, const sensor_data_t *data)
{
    TEST_ASSERT_EQUAL_INT(snapshot->type, data->type);
    TEST_ASSERT_EQUAL_INT(snapshot->unit, data->unit);
    TEST_ASSERT_EQUAL_UINT32(snapshot->value.val_uint32, data->value.val_uint32);
    TEST_ASSERT_EQUAL_UINT32(snapshot->timestamp, data->timestamp);
    TEST_ASSERT_EQUAL_UINT8(snapshot->accuracy, data->accuracy);
}

static void test_rgb_create_init_and_read_little_endian_channels(void)
{
    int bus;
    sensor_device_t *sensor = apds9960_create_rgb("apds-rgb", &bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("apds-rgb", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Avago", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("APDS9960", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_RGB, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_EQUAL_PTR(&bus, sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->deinit);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_EQUAL_UINT8(APDS9960_ADDR, ((apds9960_priv_t *)sensor->priv_data)->i2c_addr);

    uint8_t id = 0xABU;
    queue_i2c_read(&bus, APDS9960_ADDR, APDS9960_REG_ID, &id, 1U, 0);
    queue_i2c_write(&bus, APDS9960_ADDR, APDS9960_REG_ENABLE, 0x4FU, 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));

    sensor_data_t data = {0};
    uint8_t raw[] = {0x78U, 0x56U, 0x34U, 0x12U, 0xCDU, 0xABU, 0xEFU, 0xBEU};
    queue_i2c_read(&bus, APDS9960_ADDR, APDS9960_REG_CDATAL, raw, sizeof(raw), 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_RGB, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_NONE, data.unit);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(90U, data.accuracy);

    apds9960_rgb_t rgb;
    memcpy(&rgb, data.value.val_bytes, sizeof(rgb));
    TEST_ASSERT_EQUAL_UINT16(0x1234U, rgb.r);
    TEST_ASSERT_EQUAL_UINT16(0xABCDU, rgb.g);
    TEST_ASSERT_EQUAL_UINT16(0xBEEFU, rgb.b);
    TEST_ASSERT_EQUAL_UINT16(0x5678U, rgb.c);
    assert_no_extra_i2c();

    destroy_sensor(sensor);
}

static void test_rgb_init_propagates_id_and_enable_failures(void)
{
    int bus;
    sensor_device_t *sensor = apds9960_create_rgb("apds-init", &bus);
    TEST_ASSERT_NOT_NULL(sensor);

    uint8_t id = 0x00U;
    queue_i2c_read(&bus, APDS9960_ADDR, APDS9960_REG_ID, &id, 1U, -5);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    id = 0x42U;
    queue_i2c_read(&bus, APDS9960_ADDR, APDS9960_REG_ID, &id, 1U, 0);
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(sensor));

    id = 0x9CU;
    queue_i2c_read(&bus, APDS9960_ADDR, APDS9960_REG_ID, &id, 1U, 0);
    queue_i2c_write(&bus, APDS9960_ADDR, APDS9960_REG_ENABLE, 0x4FU, -6);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));
    assert_no_extra_i2c();

    destroy_sensor(sensor);
}

static void test_deinit_propagates_disable_write_failure(void)
{
    int bus;
    sensor_device_t *sensor = apds9960_create_rgb("apds-deinit", &bus);
    TEST_ASSERT_NOT_NULL(sensor);

    queue_i2c_write(&bus, APDS9960_ADDR, APDS9960_REG_ENABLE, 0x00U, -7);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->deinit(sensor));
    assert_no_extra_i2c();

    destroy_sensor(sensor);
}

static void test_read_failure_preserves_output(void)
{
    int bus;
    sensor_device_t *sensor = apds9960_create_rgb("apds-read-fail", &bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM,
                          .unit = SENSOR_UNIT_PPM,
                          .value.val_uint32 = 0xA5A5A5A5U,
                          .timestamp = 123U,
                          .accuracy = 12U};
    sensor_data_t snapshot = data;

    TEST_ASSERT_NOT_NULL(sensor);
    queue_i2c_read(&bus, APDS9960_ADDR, APDS9960_REG_CDATAL, NULL, 8U, -8);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));
    assert_output_unchanged(&snapshot, &data);
    assert_no_extra_i2c();

    destroy_sensor(sensor);
}

static void test_public_guards_reject_null_inputs_without_i2c_side_effects(void)
{
    int bus;
    sensor_device_t *sensor = apds9960_create_rgb("apds-guard", &bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_uint32 = 99U, .timestamp = 44U};
    sensor_data_t snapshot = data;

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, NULL));
    assert_output_unchanged(&snapshot, &data);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_unexpected);

    destroy_sensor(sensor);
}

static void test_missing_private_data_is_rejected_without_i2c_side_effects(void)
{
    int bus;
    sensor_device_t *sensor = apds9960_create_rgb("apds-no-priv", &bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_uint32 = 77U, .timestamp = 55U};
    sensor_data_t snapshot = data;

    TEST_ASSERT_NOT_NULL(sensor);
    SENSOR_FREE(sensor->priv_data);
    sensor->priv_data = NULL;

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, &data));
    assert_output_unchanged(&snapshot, &data);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_i2c_unexpected);

    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rgb_create_init_and_read_little_endian_channels);
    RUN_TEST(test_rgb_init_propagates_id_and_enable_failures);
    RUN_TEST(test_deinit_propagates_disable_write_failure);
    RUN_TEST(test_read_failure_preserves_output);
    RUN_TEST(test_public_guards_reject_null_inputs_without_i2c_side_effects);
    RUN_TEST(test_missing_private_data_is_rejected_without_i2c_side_effects);
    return UNITY_END();
}
