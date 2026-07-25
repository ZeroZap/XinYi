#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_as5048.h"
#include "sensor_as5600.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint8_t addr;
    uint8_t reg;
    uint8_t data[2];
    uint16_t len;
    int ret;
} read_call_t;

static read_call_t g_reads[8];
static unsigned int g_read_count;
static uint32_t g_tick;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    (void)bus;
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_count);
    read_call_t *call = &g_reads[g_read_count++];
    call->addr = addr;
    call->reg = reg;
    call->len = len;
    if (call->ret != SENSOR_EOK) {
        return call->ret;
    }
    memcpy(data, call->data, len);
    return SENSOR_EOK;
}

void setUp(void)
{
    memset(g_reads, 0, sizeof(g_reads));
    g_read_count = 0;
    g_tick = 98765U;
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

static void queue_raw_le(uint16_t raw)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_count);
    g_reads[g_read_count].data[0] = (uint8_t)raw;
    g_reads[g_read_count].data[1] = (uint8_t)(raw >> 8);
}

static void assert_common_identity(sensor_device_t *sensor, const char *name, const char *model,
                                   uint8_t addr, void *bus)
{
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING(name, sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("AMS", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING(model, sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ANGLE, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_EQUAL_PTR(bus, sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_NOT_NULL(sensor->priv_data);
    TEST_ASSERT_EQUAL_UINT8(addr, ((as5600_priv_t *)sensor->priv_data)->i2c_addr);
}

static void test_as5600_create_and_read_converts_12bit_little_endian_angle(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = as5600_create("as5600-main", &fake_bus);
    assert_common_identity(sensor, "as5600-main", "AS5600", AS5600_ADDR, &fake_bus);
    queue_raw_le(2048U); /* half scale => 180 deg */

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));

    TEST_ASSERT_EQUAL_UINT(1U, g_read_count);
    TEST_ASSERT_EQUAL_UINT8(AS5600_ADDR, g_reads[0].addr);
    TEST_ASSERT_EQUAL_UINT8(0x0EU, g_reads[0].reg);
    TEST_ASSERT_EQUAL_UINT16(2U, g_reads[0].len);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ANGLE, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 180.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    destroy_sensor(sensor);
}

static void test_as5048_create_and_read_converts_14bit_little_endian_angle(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = as5048_create("as5048-main", &fake_bus);
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("as5048-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("AMS", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("AS5048", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ANGLE, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_EQUAL_UINT8(AS5048_ADDR, ((as5048_priv_t *)sensor->priv_data)->i2c_addr);
    queue_raw_le(4096U); /* quarter scale => 90 deg */

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));

    TEST_ASSERT_EQUAL_UINT(1U, g_read_count);
    TEST_ASSERT_EQUAL_UINT8(AS5048_ADDR, g_reads[0].addr);
    TEST_ASSERT_EQUAL_UINT8(0xFEU, g_reads[0].reg);
    TEST_ASSERT_EQUAL_UINT16(2U, g_reads[0].len);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ANGLE, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    destroy_sensor(sensor);
}

static void test_long_names_are_truncated_with_terminator(void)
{
    int fake_bus;
    char long_name[SENSOR_NAME_MAX_LEN * 2U];
    memset(long_name, 'B', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = '\0';

    sensor_device_t *as5600 = as5600_create(long_name, &fake_bus);
    sensor_device_t *as5048 = as5048_create(long_name, &fake_bus);

    TEST_ASSERT_NOT_NULL(as5600);
    TEST_ASSERT_NOT_NULL(as5048);
    TEST_ASSERT_EQUAL_UINT8('\0', as5600->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', as5048->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(as5600->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(as5048->info.name));

    destroy_sensor(as5600);
    destroy_sensor(as5048);
}

static void test_create_rejects_null_names_without_i2c_side_effects(void)
{
    int fake_bus;

    TEST_ASSERT_NULL(as5600_create(NULL, &fake_bus));
    TEST_ASSERT_NULL(as5048_create(NULL, &fake_bus));
    TEST_ASSERT_EQUAL_UINT(0U, g_read_count);
}

static void test_public_guards_reject_null_inputs_and_missing_private_state(void)
{
    int fake_bus;
    sensor_data_t data;
    memset(&data, 0xA5, sizeof(data));
    sensor_device_t *as5600 = as5600_create("as5600", &fake_bus);
    sensor_device_t *as5048 = as5048_create("as5048", &fake_bus);
    TEST_ASSERT_NOT_NULL(as5600);
    TEST_ASSERT_NOT_NULL(as5048);

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, as5600->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, as5048->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, as5600->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, as5048->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, as5600->ops->read(as5600, NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, as5048->ops->read(as5048, NULL));
    TEST_ASSERT_EQUAL_UINT(0U, g_read_count);

    SENSOR_FREE(as5600->priv_data);
    as5600->priv_data = NULL;
    SENSOR_FREE(as5048->priv_data);
    as5048->priv_data = NULL;
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, as5600->ops->init(as5600));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, as5048->ops->init(as5048));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, as5600->ops->read(as5600, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, as5048->ops->read(as5048, &data));

    destroy_sensor(as5600);
    destroy_sensor(as5048);
}

static void test_i2c_read_failures_preserve_output(void)
{
    int fake_bus;
    sensor_data_t data;
    memset(&data, 0xA5, sizeof(data));
    sensor_device_t *as5600 = as5600_create("as5600", &fake_bus);
    sensor_device_t *as5048 = as5048_create("as5048", &fake_bus);
    TEST_ASSERT_NOT_NULL(as5600);
    TEST_ASSERT_NOT_NULL(as5048);

    g_reads[0].ret = SENSOR_EIO;
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, as5600->ops->read(as5600, &data));
    TEST_ASSERT_EQUAL_UINT(1U, g_read_count);
    TEST_ASSERT_EQUAL_UINT8(0xA5U, data.value.val_bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(0xA5U, data.value.val_bytes[sizeof(data.value.val_bytes) - 1U]);

    g_read_count = 0;
    memset(&data, 0x5A, sizeof(data));
    g_reads[0].ret = SENSOR_EIO;
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, as5048->ops->read(as5048, &data));
    TEST_ASSERT_EQUAL_UINT(1U, g_read_count);
    TEST_ASSERT_EQUAL_UINT8(0x5AU, data.value.val_bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(0x5AU, data.value.val_bytes[sizeof(data.value.val_bytes) - 1U]);

    destroy_sensor(as5600);
    destroy_sensor(as5048);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_as5600_create_and_read_converts_12bit_little_endian_angle);
    RUN_TEST(test_as5048_create_and_read_converts_14bit_little_endian_angle);
    RUN_TEST(test_long_names_are_truncated_with_terminator);
    RUN_TEST(test_create_rejects_null_names_without_i2c_side_effects);
    RUN_TEST(test_public_guards_reject_null_inputs_and_missing_private_state);
    RUN_TEST(test_i2c_read_failures_preserve_output);
    return UNITY_END();
}
