#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_bh1750.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint8_t addr;
    uint8_t len;
    uint8_t data[4];
    int status;
} i2c_mem_read_call_t;

typedef struct {
    uint8_t addr;
    uint8_t len;
    uint8_t data[4];
    int status;
} i2c_mem_write_call_t;

static i2c_mem_read_call_t g_i2c_reads[4];
static i2c_mem_write_call_t g_i2c_writes[8];
static unsigned int g_i2c_read_count;
static unsigned int g_i2c_write_count;
static uint32_t g_tick;
static uint32_t g_delay_total_ms;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

void delay_ms(uint32_t ms)
{
    g_delay_total_ms += ms;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t *data, uint16_t len)
{
    (void)bus;
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_reads), g_i2c_read_count);
    i2c_mem_read_call_t *call = &g_i2c_reads[g_i2c_read_count++];
    call->addr = addr;
    call->len = (uint8_t)len;
    if (call->status != SENSOR_EOK) {
        return call->status;
    }
    memcpy(data, call->data, len);
    return SENSOR_EOK;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t *data, uint16_t len)
{
    (void)bus;
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_writes), g_i2c_write_count);
    i2c_mem_write_call_t *call = &g_i2c_writes[g_i2c_write_count++];
    call->addr = addr;
    call->len = (uint8_t)len;
    memcpy(call->data, data, len);
    return call->status;
}

void setUp(void)
{
    memset(g_i2c_reads, 0, sizeof(g_i2c_reads));
    memset(g_i2c_writes, 0, sizeof(g_i2c_writes));
    g_i2c_read_count = 0;
    g_i2c_write_count = 0;
    g_tick = 424242U;
    g_delay_total_ms = 0;
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

static void test_bh1750_create_and_init_write_sequence(void)
{
    int fake_bus;
    sensor_device_t *sensor = bh1750_create("bh-main", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("bh-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("ROHM", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("BH1750", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_LIGHT, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_NULL(sensor->ops->deinit);
    TEST_ASSERT_EQUAL_UINT8(BH1750_ADDR, ((bh1750_priv_t *)sensor->priv_data)->i2c_addr);

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));

    TEST_ASSERT_EQUAL_UINT(2U, g_i2c_write_count);
    TEST_ASSERT_EQUAL_UINT8(BH1750_ADDR, g_i2c_writes[0].addr);
    TEST_ASSERT_EQUAL_UINT8(1U, g_i2c_writes[0].len);
    TEST_ASSERT_EQUAL_UINT8(0x01U, g_i2c_writes[0].data[0]);
    TEST_ASSERT_EQUAL_UINT8(BH1750_ADDR, g_i2c_writes[1].addr);
    TEST_ASSERT_EQUAL_UINT8(1U, g_i2c_writes[1].len);
    TEST_ASSERT_EQUAL_UINT8(0x10U, g_i2c_writes[1].data[0]);

    destroy_sensor(sensor);
}

static void test_bh1750_read_triggers_one_time_measurement_and_converts_lux(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = bh1750_create("bh-read", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    g_i2c_reads[0].data[0] = 0x01U;
    g_i2c_reads[0].data[1] = 0xE0U; /* 480 raw / 1.2 = 400 lux */

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));

    TEST_ASSERT_EQUAL_UINT(1U, g_i2c_write_count);
    TEST_ASSERT_EQUAL_UINT8(BH1750_ADDR, g_i2c_writes[0].addr);
    TEST_ASSERT_EQUAL_UINT8(1U, g_i2c_writes[0].len);
    TEST_ASSERT_EQUAL_UINT8(0x20U, g_i2c_writes[0].data[0]);
    TEST_ASSERT_EQUAL_UINT32(20U, g_delay_total_ms);
    TEST_ASSERT_EQUAL_UINT(1U, g_i2c_read_count);
    TEST_ASSERT_EQUAL_UINT8(BH1750_ADDR, g_i2c_reads[0].addr);
    TEST_ASSERT_EQUAL_UINT8(2U, g_i2c_reads[0].len);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_LIGHT, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_LUX, data.unit);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 400.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    destroy_sensor(sensor);
}

static void test_bh1750_read_failure_preserves_output_after_trigger_delay(void)
{
    int fake_bus;
    sensor_data_t data;
    sensor_device_t *sensor = bh1750_create("bh-fail", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    memset(&data, 0xA5, sizeof(data));
    g_i2c_reads[0].status = SENSOR_EIO;

    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));

    TEST_ASSERT_EQUAL_UINT(1U, g_i2c_write_count);
    TEST_ASSERT_EQUAL_UINT8(0x20U, g_i2c_writes[0].data[0]);
    TEST_ASSERT_EQUAL_UINT32(20U, g_delay_total_ms);
    TEST_ASSERT_EQUAL_UINT(1U, g_i2c_read_count);
    TEST_ASSERT_EQUAL_UINT8(0xA5U, data.type);
    TEST_ASSERT_EQUAL_UINT32(0xA5A5A5A5U, data.timestamp);

    destroy_sensor(sensor);
}

static void test_bh1750_long_name_is_truncated_with_terminator(void)
{
    int fake_bus;
    char long_name[SENSOR_NAME_MAX_LEN * 2U];
    memset(long_name, 'B', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = '\0';

    sensor_device_t *sensor = bh1750_create(long_name, &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_UINT8('\0', sensor->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(sensor->info.name));

    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bh1750_create_and_init_write_sequence);
    RUN_TEST(test_bh1750_read_triggers_one_time_measurement_and_converts_lux);
    RUN_TEST(test_bh1750_read_failure_preserves_output_after_trigger_delay);
    RUN_TEST(test_bh1750_long_name_is_truncated_with_terminator);
    return UNITY_END();
}
