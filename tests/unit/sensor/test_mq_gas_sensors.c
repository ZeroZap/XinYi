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

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));

    TEST_ASSERT_EQUAL_UINT(1U, g_adc_read_count);
    TEST_ASSERT_EQUAL_UINT8(pin, g_adc_pin);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GAS, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
}

static void assert_init_does_not_sample_adc(sensor_device_t *sensor)
{
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT(0U, g_adc_read_count);
}

static void test_mq3_create_and_read_converts_adc_divide_by_10(void)
{
    sensor_device_t *sensor = mq3_create("mq3-main", 3U);
    assert_common_gas_identity(sensor, "mq3-main", "MQ-3", 3U);

    assert_init_does_not_sample_adc(sensor);
    assert_adc_read(sensor, 3U, 321U, 32.1f);

    destroy_sensor(sensor);
}

static void test_mq3_adc_zero_and_full_scale_boundaries(void)
{
    sensor_device_t *sensor = mq3_create("mq3-bounds", 4U);
    TEST_ASSERT_NOT_NULL(sensor);

    assert_init_does_not_sample_adc(sensor);
    assert_adc_read(sensor, 4U, 0U, 0.0f);

    g_adc_read_count = 0;
    g_tick = 24680U;
    assert_adc_read(sensor, 4U, 4095U, 409.5f);

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

    assert_init_does_not_sample_adc(sensor);
    assert_adc_read(sensor, 7U, 456U, 45.6f);

    destroy_sensor(sensor);
}

static void test_mq7_adc_zero_and_full_scale_boundaries(void)
{
    sensor_device_t *sensor = mq7_create("mq7-bounds", 8U);
    TEST_ASSERT_NOT_NULL(sensor);

    assert_init_does_not_sample_adc(sensor);
    assert_adc_read(sensor, 8U, 0U, 0.0f);

    g_adc_read_count = 0;
    g_tick = 97531U;
    assert_adc_read(sensor, 8U, 4095U, 409.5f);

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

    assert_init_does_not_sample_adc(sensor);
    assert_adc_read(sensor, 9U, 400U, 50.0f);

    destroy_sensor(sensor);
}

static void test_mq135_adc_zero_and_full_scale_boundaries(void)
{
    sensor_device_t *sensor = mq135_create("mq135-bounds", 10U);
    TEST_ASSERT_NOT_NULL(sensor);

    assert_init_does_not_sample_adc(sensor);
    assert_adc_read(sensor, 10U, 0U, 0.0f);

    g_adc_read_count = 0;
    g_tick = 86420U;
    assert_adc_read(sensor, 10U, 4095U, 511.875f);

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

static void test_create_rejects_null_name(void)
{
    TEST_ASSERT_NULL(mq3_create(NULL, 1U));
    TEST_ASSERT_NULL(mq7_create(NULL, 2U));
    TEST_ASSERT_NULL(mq135_create(NULL, 3U));
}

static void assert_invalid_public_ops_do_not_sample_adc(sensor_device_t *sensor)
{
    sensor_data_t data;

    memset(&data, 0x5A, sizeof(data));
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, sensor->ops->read(sensor, NULL));
    TEST_ASSERT_EQUAL_UINT(0U, g_adc_read_count);
    TEST_ASSERT_EQUAL_UINT8(0x5AU, data.type);
    TEST_ASSERT_EQUAL_UINT32(0x5A5A5A5AU, data.timestamp);
}

static void test_public_ops_reject_null_inputs_without_side_effects(void)
{
    sensor_device_t *mq3 = mq3_create("mq3-guards", 1U);
    sensor_device_t *mq7 = mq7_create("mq7-guards", 2U);
    sensor_device_t *mq135 = mq135_create("mq135-guards", 3U);

    TEST_ASSERT_NOT_NULL(mq3);
    TEST_ASSERT_NOT_NULL(mq7);
    TEST_ASSERT_NOT_NULL(mq135);

    assert_invalid_public_ops_do_not_sample_adc(mq3);
    assert_invalid_public_ops_do_not_sample_adc(mq7);
    assert_invalid_public_ops_do_not_sample_adc(mq135);

    destroy_sensor(mq3);
    destroy_sensor(mq7);
    destroy_sensor(mq135);
}

static void test_missing_private_data_is_rejected_without_adc_side_effects(void)
{
    sensor_device_t *mq3 = mq3_create("mq3-no-priv", 1U);
    sensor_device_t *mq7 = mq7_create("mq7-no-priv", 2U);
    sensor_device_t *mq135 = mq135_create("mq135-no-priv", 3U);
    sensor_data_t data;

    TEST_ASSERT_NOT_NULL(mq3);
    TEST_ASSERT_NOT_NULL(mq7);
    TEST_ASSERT_NOT_NULL(mq135);

    memset(&data, 0xA5, sizeof(data));
    SENSOR_FREE(mq3->priv_data);
    mq3->priv_data = NULL;
    SENSOR_FREE(mq7->priv_data);
    mq7->priv_data = NULL;
    SENSOR_FREE(mq135->priv_data);
    mq135->priv_data = NULL;

    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, mq3->ops->init(mq3));
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, mq3->ops->read(mq3, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, mq7->ops->init(mq7));
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, mq7->ops->read(mq7, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, mq135->ops->init(mq135));
    TEST_ASSERT_EQUAL_INT(SENSOR_ERROR, mq135->ops->read(mq135, &data));
    TEST_ASSERT_EQUAL_UINT(0U, g_adc_read_count);
    TEST_ASSERT_EQUAL_UINT8(0xA5U, data.type);
    TEST_ASSERT_EQUAL_UINT32(0xA5A5A5A5U, data.timestamp);

    destroy_sensor(mq3);
    destroy_sensor(mq7);
    destroy_sensor(mq135);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mq3_create_and_read_converts_adc_divide_by_10);
    RUN_TEST(test_mq3_adc_zero_and_full_scale_boundaries);
    RUN_TEST(test_mq7_create_and_read_converts_adc_divide_by_10);
    RUN_TEST(test_mq7_adc_zero_and_full_scale_boundaries);
    RUN_TEST(test_mq135_create_and_read_converts_adc_divide_by_8);
    RUN_TEST(test_mq135_adc_zero_and_full_scale_boundaries);
    RUN_TEST(test_long_names_are_truncated_with_terminator);
    RUN_TEST(test_create_rejects_null_name);
    RUN_TEST(test_public_ops_reject_null_inputs_without_side_effects);
    RUN_TEST(test_missing_private_data_is_rejected_without_adc_side_effects);
    return UNITY_END();
}
