#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_aeat8800.h"
#include "sensor_mlx90393.h"

static uint32_t g_tick;

uint32_t get_tick_ms(void)
{
    return g_tick;
}

void setUp(void)
{
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

static void test_mlx90393_create_sets_identity_and_default_read_contract(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = mlx90393_create("mlx90393-main", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("mlx90393-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Melexis", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("MLX90393", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ANGLE, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_NOT_NULL(sensor->priv_data);
    TEST_ASSERT_EQUAL_UINT8(MLX90393_ADDR, ((mlx90393_priv_t *)sensor->priv_data)->i2c_addr);

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));

    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ANGLE, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    destroy_sensor(sensor);
}

static void test_aeat8800_create_sets_identity_and_default_read_contract(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = aeat8800_create("aeat8800-main", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("aeat8800-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Bourns", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("AEAT-8800", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ANGLE, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_NOT_NULL(sensor->priv_data);

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));

    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_ANGLE, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    destroy_sensor(sensor);
}

static void test_long_names_are_truncated_with_terminator(void)
{
    int fake_bus;
    char long_name[SENSOR_NAME_MAX_LEN * 2U];
    memset(long_name, 'M', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = '\0';

    sensor_device_t *mlx90393 = mlx90393_create(long_name, &fake_bus);
    sensor_device_t *aeat8800 = aeat8800_create(long_name, &fake_bus);

    TEST_ASSERT_NOT_NULL(mlx90393);
    TEST_ASSERT_NOT_NULL(aeat8800);
    TEST_ASSERT_EQUAL_UINT8('\0', mlx90393->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT8('\0', aeat8800->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(mlx90393->info.name));
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(aeat8800->info.name));

    destroy_sensor(mlx90393);
    destroy_sensor(aeat8800);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mlx90393_create_sets_identity_and_default_read_contract);
    RUN_TEST(test_aeat8800_create_sets_identity_and_default_read_contract);
    RUN_TEST(test_long_names_are_truncated_with_terminator);
    return UNITY_END();
}
