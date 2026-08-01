#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_acs712.h"
#include "sensor_fsr.h"
#include "sensor_mg811.h"

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
    g_tick = 112233U;
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

static void assert_adc_sample(sensor_device_t *sensor, uint8_t pin, uint16_t raw,
                              sensor_type_t expected_type, float expected_value)
{
    sensor_data_t data = {0};
    g_adc_value = raw;
    g_adc_pin = 0xFFU;
    g_adc_read_count = 0U;

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));

    TEST_ASSERT_EQUAL_UINT(1U, g_adc_read_count);
    TEST_ASSERT_EQUAL_UINT8(pin, g_adc_pin);
    TEST_ASSERT_EQUAL_INT(expected_type, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected_value, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
}

static void test_acs712_create_and_read_converts_adc_to_current(void)
{
    sensor_device_t *sensor = acs712_create("acs712-main", 5U);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("acs712-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Allegro", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("ACS712", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_CURRENT, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_NULL(sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_EQUAL_UINT8(5U, ((acs712_priv_t *)sensor->priv_data)->adc_pin);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));

    assert_adc_sample(sensor, 5U, 2048U, SENSOR_TYPE_CURRENT, 0.0f);

    destroy_sensor(sensor);
}

static void test_fsr_create_and_read_converts_adc_to_pressure(void)
{
    sensor_device_t *sensor = fsr_create("fsr-main", 6U);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("fsr-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Interlink", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("FSR", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PRESSURE, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_NULL(sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_EQUAL_UINT8(6U, ((fsr_priv_t *)sensor->priv_data)->adc_pin);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));

    assert_adc_sample(sensor, 6U, 1000U, SENSOR_TYPE_PRESSURE, 250.0f);

    destroy_sensor(sensor);
}

static void test_mg811_create_and_read_converts_adc_to_co2_ppm_like_value(void)
{
    sensor_device_t *sensor = mg811_create("mg811-main", 8U);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("mg811-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("国产", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("MG811", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GAS, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_NULL(sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_EQUAL_UINT8(8U, ((mg811_priv_t *)sensor->priv_data)->adc_pin);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));

    assert_adc_sample(sensor, 8U, 2048U, SENSOR_TYPE_GAS, 5000.0f);

    destroy_sensor(sensor);
}

static void test_adc_boundaries_are_converted_for_each_analog_sensor(void)
{
    sensor_device_t *acs712 = acs712_create("acs-boundary", 1U);
    sensor_device_t *fsr = fsr_create("fsr-boundary", 2U);
    sensor_device_t *mg811 = mg811_create("mg-boundary", 3U);

    TEST_ASSERT_NOT_NULL(acs712);
    TEST_ASSERT_NOT_NULL(fsr);
    TEST_ASSERT_NOT_NULL(mg811);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, acs712->ops->init(acs712));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, fsr->ops->init(fsr));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, mg811->ops->init(mg811));

    assert_adc_sample(acs712, 1U, 0U, SENSOR_TYPE_CURRENT, -25.0f);
    assert_adc_sample(acs712, 1U, 4095U, SENSOR_TYPE_CURRENT, 24.9878f);
    assert_adc_sample(fsr, 2U, 0U, SENSOR_TYPE_PRESSURE, 0.0f);
    assert_adc_sample(fsr, 2U, 4095U, SENSOR_TYPE_PRESSURE, 1023.75f);
    assert_adc_sample(mg811, 3U, 0U, SENSOR_TYPE_GAS, 0.0f);
    assert_adc_sample(mg811, 3U, 4095U, SENSOR_TYPE_GAS, 9997.5586f);

    destroy_sensor(acs712);
    destroy_sensor(fsr);
    destroy_sensor(mg811);
}

static void test_long_names_are_truncated_with_terminator(void)
{
    char long_name[SENSOR_NAME_MAX_LEN * 2U];
    memset(long_name, 'A', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = '\0';

    sensor_device_t *acs712 = acs712_create(long_name, 1U);
    sensor_device_t *fsr = fsr_create(long_name, 2U);
    sensor_device_t *mg811 = mg811_create(long_name, 3U);

    TEST_ASSERT_NOT_NULL(acs712);
    TEST_ASSERT_NOT_NULL(fsr);
    TEST_ASSERT_NOT_NULL(mg811);
    TEST_ASSERT_EQUAL_UINT8('\0', acs712->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', fsr->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', mg811->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(acs712->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(fsr->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(mg811->info.name));

    destroy_sensor(acs712);
    destroy_sensor(fsr);
    destroy_sensor(mg811);
}

static void test_create_rejects_null_name(void)
{
    TEST_ASSERT_NULL(acs712_create(NULL, 1U));
    TEST_ASSERT_NULL(fsr_create(NULL, 2U));
    TEST_ASSERT_NULL(mg811_create(NULL, 3U));
}

static void test_public_ops_reject_invalid_arguments_without_adc_side_effects(void)
{
    sensor_device_t *acs712 = acs712_create("acs-guards", 1U);
    sensor_device_t *fsr = fsr_create("fsr-guards", 2U);
    sensor_device_t *mg811 = mg811_create("mg-guards", 3U);
    sensor_data_t sentinel;

    TEST_ASSERT_NOT_NULL(acs712);
    TEST_ASSERT_NOT_NULL(fsr);
    TEST_ASSERT_NOT_NULL(mg811);
    memset(&sentinel, 0xA5, sizeof(sentinel));

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, acs712->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, fsr->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, mg811->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, acs712->ops->read(NULL, &sentinel));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, fsr->ops->read(NULL, &sentinel));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, mg811->ops->read(NULL, &sentinel));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, acs712->ops->read(acs712, NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, fsr->ops->read(fsr, NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, mg811->ops->read(mg811, NULL));
    TEST_ASSERT_EQUAL_UINT(0U, g_adc_read_count);

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, acs712->ops->init(&(sensor_device_t){0}));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, fsr->ops->read(&(sensor_device_t){0}, &sentinel));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, mg811->ops->read(&(sensor_device_t){0}, &sentinel));
    TEST_ASSERT_EQUAL_UINT(0U, g_adc_read_count);
    TEST_ASSERT_EQUAL_UINT8(0xA5U, sentinel.type);
    TEST_ASSERT_EQUAL_UINT32(0xA5A5A5A5U, sentinel.timestamp);

    destroy_sensor(acs712);
    destroy_sensor(fsr);
    destroy_sensor(mg811);
}

static void test_zero_raw_reads_update_outputs_and_timestamps(void)
{
    sensor_device_t *acs712 = acs712_create("acs-preserve", 1U);
    sensor_device_t *fsr = fsr_create("fsr-preserve", 2U);
    sensor_device_t *mg811 = mg811_create("mg-preserve", 3U);
    sensor_data_t acs_data = {
        .type = SENSOR_TYPE_CURRENT,
        .value.val_float = 123.0f,
        .timestamp = 11U,
    };
    sensor_data_t fsr_data = {
        .type = SENSOR_TYPE_PRESSURE,
        .value.val_float = 456.0f,
        .timestamp = 22U,
    };
    sensor_data_t mg_data = {
        .type = SENSOR_TYPE_GAS,
        .value.val_float = 789.0f,
        .timestamp = 33U,
    };

    TEST_ASSERT_NOT_NULL(acs712);
    TEST_ASSERT_NOT_NULL(fsr);
    TEST_ASSERT_NOT_NULL(mg811);

    g_adc_value = 0U;
    g_tick = 0U;
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, acs712->ops->read(acs712, &acs_data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, fsr->ops->read(fsr, &fsr_data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, mg811->ops->read(mg811, &mg_data));

    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_CURRENT, acs_data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -25.0f, acs_data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(0U, acs_data.timestamp);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_PRESSURE, fsr_data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, fsr_data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(0U, fsr_data.timestamp);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GAS, mg_data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mg_data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(0U, mg_data.timestamp);
    TEST_ASSERT_EQUAL_UINT(3U, g_adc_read_count);

    destroy_sensor(acs712);
    destroy_sensor(fsr);
    destroy_sensor(mg811);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_acs712_create_and_read_converts_adc_to_current);
    RUN_TEST(test_fsr_create_and_read_converts_adc_to_pressure);
    RUN_TEST(test_mg811_create_and_read_converts_adc_to_co2_ppm_like_value);
    RUN_TEST(test_adc_boundaries_are_converted_for_each_analog_sensor);
    RUN_TEST(test_long_names_are_truncated_with_terminator);
    RUN_TEST(test_create_rejects_null_name);
    RUN_TEST(test_public_ops_reject_invalid_arguments_without_adc_side_effects);
    RUN_TEST(test_zero_raw_reads_update_outputs_and_timestamps);
    return UNITY_END();
}
