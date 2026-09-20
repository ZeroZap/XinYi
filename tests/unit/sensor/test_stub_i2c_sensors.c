#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_max30102.h"

static uint32_t g_tick;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

void setUp(void)
{
    g_tick = 31415U;
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

static void assert_stub_identity(sensor_device_t *sensor, const char *name, const char *vendor,
                                 const char *model, sensor_type_t type, void *bus)
{
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING(name, sensor->info.name);
    TEST_ASSERT_EQUAL_STRING(vendor, sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING(model, sensor->info.model);
    TEST_ASSERT_EQUAL_INT(type, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_EQUAL_PTR(bus, sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->deinit);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_NOT_NULL(sensor->priv_data);
}

static void assert_output_unchanged(const sensor_data_t *actual, const sensor_data_t *expected)
{
    TEST_ASSERT_EQUAL_INT(expected->type, actual->type);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, expected->value.val_float, actual->value.val_float);
    TEST_ASSERT_EQUAL_UINT32(expected->timestamp, actual->timestamp);
}





static void test_max30102_create_sets_identity_and_default_read_contract(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = max30102_create("max30102-main", &fake_bus);

    assert_stub_identity(sensor, "max30102-main", "Maxim", "MAX30102", SENSOR_TYPE_HEART_RATE,
                         &fake_bus);
    TEST_ASSERT_EQUAL_UINT8(MAX30102_ADDR, ((max30102_priv_t *)sensor->priv_data)->i2c_addr);

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_HEART_RATE, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 72.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    destroy_sensor(sensor);
}

static void test_long_names_are_truncated_with_terminator(void)
{
    int fake_bus;
    char long_name[SENSOR_NAME_MAX_LEN * 2U];
    memset(long_name, 'I', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = '\0';

    sensor_device_t *max30102 = max30102_create(long_name, &fake_bus);

    TEST_ASSERT_NOT_NULL(max30102);
    TEST_ASSERT_EQUAL_UINT8('\0', max30102->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(max30102->info.name));

    destroy_sensor(max30102);
}

static void test_create_rejects_null_names(void)
{
    int fake_bus;

    TEST_ASSERT_NULL(max30102_create(NULL, &fake_bus));
}

static void test_i2c_factories_and_ops_reject_missing_bus(void)
{
    int fake_bus;
    sensor_device_t *sensors[] = {
        max30102_create("max30102-bus", &fake_bus),
    };
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_float = -4.0f, .timestamp = 91U};
    sensor_data_t snapshot = data;
    size_t i;

    TEST_ASSERT_NULL(max30102_create("max30102-null-bus", NULL));

    for (i = 0U; i < sizeof(sensors) / sizeof(sensors[0]); ++i) {
        TEST_ASSERT_NOT_NULL(sensors[i]);
        sensors[i]->bus = NULL;
        TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensors[i]->ops->init(sensors[i]));
        TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensors[i]->ops->deinit(sensors[i]));
        TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensors[i]->ops->read(sensors[i], &data));
        assert_output_unchanged(&data, &snapshot);
        destroy_sensor(sensors[i]);
    }
}

static void test_public_ops_reject_null_inputs_and_preserve_output(void)
{
    int fake_bus;
    sensor_device_t *max30102 = max30102_create("max30102-guard", &fake_bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_float = -12.5f, .timestamp = 777U};
    sensor_data_t snapshot = data;

    TEST_ASSERT_NOT_NULL(max30102);

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, max30102->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, max30102->ops->deinit(NULL));

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, max30102->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, max30102->ops->read(max30102, NULL));
    assert_output_unchanged(&data, &snapshot);

    destroy_sensor(max30102);
}

static void test_missing_private_data_is_rejected_and_preserves_output(void)
{
    int fake_bus;
    sensor_device_t *max30102 = max30102_create("max30102-no-priv", &fake_bus);
    sensor_data_t data = {.type = SENSOR_TYPE_CUSTOM, .value.val_float = 41.0f, .timestamp = 55U};
    sensor_data_t snapshot = data;

    TEST_ASSERT_NOT_NULL(max30102);

    SENSOR_FREE(max30102->priv_data);
    max30102->priv_data = NULL;

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, max30102->ops->deinit(max30102));

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, max30102->ops->init(max30102));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, max30102->ops->deinit(max30102));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, max30102->ops->read(max30102, &data));
    assert_output_unchanged(&data, &snapshot);

    destroy_sensor(max30102);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_max30102_create_sets_identity_and_default_read_contract);
    RUN_TEST(test_long_names_are_truncated_with_terminator);
    RUN_TEST(test_create_rejects_null_names);
    RUN_TEST(test_i2c_factories_and_ops_reject_missing_bus);
    RUN_TEST(test_public_ops_reject_null_inputs_and_preserve_output);
    RUN_TEST(test_missing_private_data_is_rejected_and_preserves_output);
    return UNITY_END();
}
