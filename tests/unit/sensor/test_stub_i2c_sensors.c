#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_ens160.h"
#include "sensor_im69d.h"
#include "sensor_max30102.h"
#include "sensor_sgp30.h"

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
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_NOT_NULL(sensor->priv_data);
}

static void test_sgp30_create_sets_identity_and_default_read_contract(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = sgp30_create("sgp30-main", &fake_bus);

    assert_stub_identity(sensor, "sgp30-main", "Sensirion", "SGP30", SENSOR_TYPE_GAS,
                         &fake_bus);
    TEST_ASSERT_EQUAL_UINT8(SGP30_ADDR, ((sgp30_priv_t *)sensor->priv_data)->i2c_addr);

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GAS, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 100.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    destroy_sensor(sensor);
}

static void test_ens160_create_sets_identity_and_default_read_contract(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = ens160_create("ens160-main", &fake_bus);

    assert_stub_identity(sensor, "ens160-main", "Sciosense", "ENS160", SENSOR_TYPE_GAS,
                         &fake_bus);
    TEST_ASSERT_EQUAL_UINT8(ENS160_ADDR, ((ens160_priv_t *)sensor->priv_data)->i2c_addr);

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_GAS, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 100.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    destroy_sensor(sensor);
}

static void test_im69d_create_sets_identity_and_default_read_contract(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = im69d_create("im69d-main", &fake_bus);

    assert_stub_identity(sensor, "im69d-main", "Infineon", "IM69D", SENSOR_TYPE_SOUND,
                         &fake_bus);
    TEST_ASSERT_EQUAL_UINT8(IM69D_ADDR, ((im69d_priv_t *)sensor->priv_data)->i2c_addr);

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_SOUND, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    destroy_sensor(sensor);
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

    sensor_device_t *sgp30 = sgp30_create(long_name, &fake_bus);
    sensor_device_t *ens160 = ens160_create(long_name, &fake_bus);
    sensor_device_t *im69d = im69d_create(long_name, &fake_bus);
    sensor_device_t *max30102 = max30102_create(long_name, &fake_bus);

    TEST_ASSERT_NOT_NULL(sgp30);
    TEST_ASSERT_NOT_NULL(ens160);
    TEST_ASSERT_NOT_NULL(im69d);
    TEST_ASSERT_NOT_NULL(max30102);
    TEST_ASSERT_EQUAL_UINT8('\0', sgp30->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', ens160->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', im69d->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', max30102->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(sgp30->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(ens160->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(im69d->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(max30102->info.name));

    destroy_sensor(sgp30);
    destroy_sensor(ens160);
    destroy_sensor(im69d);
    destroy_sensor(max30102);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sgp30_create_sets_identity_and_default_read_contract);
    RUN_TEST(test_ens160_create_sets_identity_and_default_read_contract);
    RUN_TEST(test_im69d_create_sets_identity_and_default_read_contract);
    RUN_TEST(test_max30102_create_sets_identity_and_default_read_contract);
    RUN_TEST(test_long_names_are_truncated_with_terminator);
    return UNITY_END();
}
