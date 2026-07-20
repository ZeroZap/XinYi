#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_guvas12sd.h"
#include "sensor_max44009.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint8_t addr;
    uint8_t reg;
    uint8_t len;
    uint8_t value;
} i2c_read_call_t;

static i2c_read_call_t g_i2c_reads[8];
static unsigned int g_i2c_read_count;
static uint16_t g_adc_value;
static uint8_t g_adc_pin;
static unsigned int g_adc_read_count;
static uint32_t g_tick;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    (void)bus;
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_i2c_reads), g_i2c_read_count);
    i2c_read_call_t *call = &g_i2c_reads[g_i2c_read_count++];
    call->addr = addr;
    call->reg = reg;
    call->len = (uint8_t)len;
    *data = call->value;
    return SENSOR_EOK;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)addr;
    (void)reg;
    (void)data;
    (void)len;
    return SENSOR_EOK;
}

uint16_t hal_adc_read(uint8_t pin)
{
    g_adc_pin = pin;
    g_adc_read_count++;
    return g_adc_value;
}

void setUp(void)
{
    memset(g_i2c_reads, 0, sizeof(g_i2c_reads));
    g_i2c_read_count = 0;
    g_adc_value = 0;
    g_adc_pin = 0;
    g_adc_read_count = 0;
    g_tick = 24680U;
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

static void test_max44009_create_and_read_converts_exponent_mantissa_to_lux(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = max44009_create("max44009-main", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("max44009-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Maxim", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("MAX44009", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_LIGHT, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_EQUAL_UINT8(MAX44009_ADDR, ((max44009_priv_t *)sensor->priv_data)->i2c_addr);

    g_i2c_reads[0].value = 0x23U; /* exponent 2, mantissa high nibble 3 */
    g_i2c_reads[1].value = 0x05U; /* mantissa low nibble 5 => 0x35 */
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));

    TEST_ASSERT_EQUAL_UINT(2U, g_i2c_read_count);
    TEST_ASSERT_EQUAL_UINT8(MAX44009_ADDR, g_i2c_reads[0].addr);
    TEST_ASSERT_EQUAL_UINT8(0x03U, g_i2c_reads[0].reg);
    TEST_ASSERT_EQUAL_UINT8(1U, g_i2c_reads[0].len);
    TEST_ASSERT_EQUAL_UINT8(MAX44009_ADDR, g_i2c_reads[1].addr);
    TEST_ASSERT_EQUAL_UINT8(0x04U, g_i2c_reads[1].reg);
    TEST_ASSERT_EQUAL_UINT8(1U, g_i2c_reads[1].len);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_LIGHT, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_LUX, data.unit);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.54f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    destroy_sensor(sensor);
}

static void test_guvas12sd_create_and_read_converts_adc_to_uv_index(void)
{
    sensor_data_t data = {0};
    sensor_device_t *sensor = guvas12sd_create("uv-main", 7U);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("uv-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("国产", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("GUVA-S12SD", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_UV_INDEX, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_NULL(sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_EQUAL_UINT8(7U, ((guvas12sd_priv_t *)sensor->priv_data)->adc_pin);

    g_adc_value = 123U;
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));

    TEST_ASSERT_EQUAL_UINT(1U, g_adc_read_count);
    TEST_ASSERT_EQUAL_UINT8(7U, g_adc_pin);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_UV_INDEX, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.3f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    destroy_sensor(sensor);
}

static void test_long_names_are_truncated_with_terminator(void)
{
    int fake_bus;
    char long_name[SENSOR_NAME_MAX_LEN * 2U];
    memset(long_name, 'L', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = '\0';

    sensor_device_t *light = max44009_create(long_name, &fake_bus);
    sensor_device_t *uv = guvas12sd_create(long_name, 3U);

    TEST_ASSERT_NOT_NULL(light);
    TEST_ASSERT_NOT_NULL(uv);
    TEST_ASSERT_EQUAL_UINT8('\0', light->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', uv->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(light->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(uv->info.name));

    destroy_sensor(light);
    destroy_sensor(uv);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_max44009_create_and_read_converts_exponent_mantissa_to_lux);
    RUN_TEST(test_guvas12sd_create_and_read_converts_adc_to_uv_index);
    RUN_TEST(test_long_names_are_truncated_with_terminator);
    return UNITY_END();
}
