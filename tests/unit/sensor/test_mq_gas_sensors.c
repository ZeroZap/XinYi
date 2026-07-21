#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_mq135.h"
#include "sensor_mq3.h"
#include "sensor_mq7.h"

static uint16_t g_adc_value;
static uint8_t g_adc_pin;
static unsigned int g_adc_read_count;
static uint32_t g_tick;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

uint16_t hal_adc_read(uint8_t pin)
{
    g_adc_pin = pin;
    g_adc_read_count++;
    return g_adc_value;
}

void setUp(void)
{
    g_adc_value = 0;
    g_adc_pin = 0;
    g_adc_read_count = 0;
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

static void assert_common_gas_identity(sensor_device_t *sensor, const char *name, const char *model,
                                       uint8_t pin)
{
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING(name, sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("国产", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING(model, sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GAS, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_NULL(sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_NOT_NULL(sensor->priv_data);
    TEST_ASSERT_EQUAL_UINT8(pin, ((mq3_priv_t *)sensor->priv_data)->adc_pin);
}

static void assert_adc_read(sensor_device_t *sensor, uint8_t pin, uint16_t raw, float expected)
{
    sensor_data_t data = {0};
    g_adc_value = raw;

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));

    TEST_ASSERT_EQUAL_UINT(1U, g_adc_read_count);
    TEST_ASSERT_EQUAL_UINT8(pin, g_adc_pin);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GAS, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
}

static void test_mq3_create_and_read_converts_adc_divide_by_10(void)
{
    sensor_device_t *sensor = mq3_create("mq3-main", 3U);
    assert_common_gas_identity(sensor, "mq3-main", "MQ-3", 3U);

    assert_adc_read(sensor, 3U, 321U, 32.1f);

    destroy_sensor(sensor);
}

static void test_mq7_create_and_read_converts_adc_divide_by_10(void)
{
    sensor_device_t *sensor = mq7_create("mq7-main", 7U);
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("mq7-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("国产", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("MQ-7", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GAS, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_NULL(sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_EQUAL_UINT8(7U, ((mq7_priv_t *)sensor->priv_data)->adc_pin);

    assert_adc_read(sensor, 7U, 456U, 45.6f);

    destroy_sensor(sensor);
}

static void test_mq135_create_and_read_converts_adc_divide_by_8(void)
{
    sensor_device_t *sensor = mq135_create("mq135-main", 9U);
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("mq135-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("国产", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("MQ-135", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GAS, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_NULL(sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_EQUAL_UINT8(9U, ((mq135_priv_t *)sensor->priv_data)->adc_pin);

    assert_adc_read(sensor, 9U, 400U, 50.0f);

    destroy_sensor(sensor);
}

static void test_long_names_are_truncated_with_terminator(void)
{
    char long_name[SENSOR_NAME_MAX_LEN * 2U];
    memset(long_name, 'G', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = '\0';

    sensor_device_t *mq3 = mq3_create(long_name, 1U);
    sensor_device_t *mq7 = mq7_create(long_name, 2U);
    sensor_device_t *mq135 = mq135_create(long_name, 3U);

    TEST_ASSERT_NOT_NULL(mq3);
    TEST_ASSERT_NOT_NULL(mq7);
    TEST_ASSERT_NOT_NULL(mq135);
    TEST_ASSERT_EQUAL_UINT8('\0', mq3->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', mq7->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', mq135->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(mq3->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(mq7->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(mq135->info.name));

    destroy_sensor(mq3);
    destroy_sensor(mq7);
    destroy_sensor(mq135);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mq3_create_and_read_converts_adc_divide_by_10);
    RUN_TEST(test_mq7_create_and_read_converts_adc_divide_by_10);
    RUN_TEST(test_mq135_create_and_read_converts_adc_divide_by_8);
    RUN_TEST(test_long_names_are_truncated_with_terminator);
    return UNITY_END();
}
