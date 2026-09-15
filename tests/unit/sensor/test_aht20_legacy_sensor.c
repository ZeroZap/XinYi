#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "sensor_aht20.h"

static unsigned int g_write_count;
static unsigned int g_read_count;
static uint32_t g_delay_total_ms;
static uint8_t g_status;
static int g_reset_write_result;
static int g_init_write_result;
static int g_status_read_result;
static int g_trigger_write_result;
static int g_measurement_read_result;

uint32_t get_tick_ms(void)
{
    return 1234U;
}

void delay_ms(uint32_t ms)
{
    g_delay_total_ms += ms;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)addr;
    (void)reg;
    (void)data;
    (void)len;
    return SENSOR_EIO;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)addr;
    (void)reg;
    (void)data;
    (void)len;
    return SENSOR_EIO;
}

int hal_i2c_write(void *bus, uint8_t addr, uint8_t *data, uint16_t len)
{
    (void)bus;
    TEST_ASSERT_EQUAL_UINT8(AHT20_ADDR_DEFAULT, addr);
    if (len == 1U) {
        TEST_ASSERT_EQUAL_UINT8(AHT20_CMD_SOFT_RESET, data[0]);
        return g_reset_write_result;
    }
    TEST_ASSERT_EQUAL_UINT16(3U, len);
    if (data[0] == AHT20_CMD_INIT) {
        TEST_ASSERT_EQUAL_UINT8(0x08U, data[1]);
        TEST_ASSERT_EQUAL_UINT8(0x00U, data[2]);
        return g_init_write_result;
    }
    TEST_ASSERT_EQUAL_UINT8(AHT20_CMD_TRIGGER, data[0]);
    TEST_ASSERT_EQUAL_UINT8(0x33U, data[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00U, data[2]);
    ++g_write_count;
    return g_trigger_write_result;
}

int hal_i2c_read(void *bus, uint8_t addr, uint8_t *data, uint16_t len)
{
    (void)bus;
    TEST_ASSERT_EQUAL_UINT8(AHT20_ADDR_DEFAULT, addr);
    if (len == 1U) {
        return g_status_read_result;
    }
    TEST_ASSERT_EQUAL_UINT16(7U, len);
    if (g_measurement_read_result != SENSOR_EOK) {
        ++g_read_count;
        return g_measurement_read_result;
    }
    memset(data, 0, len);
    data[0] = g_status;
    ++g_read_count;
    return SENSOR_EOK;
}

void setUp(void)
{
    g_write_count = 0U;
    g_read_count = 0U;
    g_delay_total_ms = 0U;
    g_status = 0x80U;
    g_reset_write_result = SENSOR_EOK;
    g_init_write_result = SENSOR_EOK;
    g_status_read_result = SENSOR_EOK;
    g_trigger_write_result = SENSOR_EOK;
    g_measurement_read_result = SENSOR_EOK;
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

static void assert_busy_preserves_output(sensor_device_t *sensor)
{
    sensor_data_t data;
    sensor_data_t before;

    memset(&data, 0xA5, sizeof(data));
    memcpy(&before, &data, sizeof(before));

    TEST_ASSERT_EQUAL_INT(SENSOR_EBUSY, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&before, &data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT(1U, g_write_count);
    TEST_ASSERT_EQUAL_UINT(1U, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(80U, g_delay_total_ms);
}

static void test_aht20_deinit_rejects_null_context(void)
{
    int fake_bus;
    sensor_device_t *sensor = aht20_create_temperature("aht20-temp", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(NULL));
    destroy_sensor(sensor);
}

static void test_aht20_init_propagates_soft_reset_failure_without_delay(void)
{
    int fake_bus;
    sensor_device_t *sensor = aht20_create_temperature("aht20-init", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    g_reset_write_result = SENSOR_ETIMEOUT;
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(0U, g_delay_total_ms);
    TEST_ASSERT_FALSE(((aht20_priv_t *)sensor->priv_data)->initialized);

    destroy_sensor(sensor);
}

static void test_aht20_init_propagates_command_and_status_transport_errors(void)
{
    int fake_bus;
    sensor_device_t *sensor = aht20_create_temperature("aht20-init-transport", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    g_init_write_result = SENSOR_ETIMEOUT;
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(20U, g_delay_total_ms);
    TEST_ASSERT_FALSE(((aht20_priv_t *)sensor->priv_data)->initialized);

    setUp();
    g_status_read_result = SENSOR_ETIMEOUT;
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT32(30U, g_delay_total_ms);
    TEST_ASSERT_FALSE(((aht20_priv_t *)sensor->priv_data)->initialized);

    destroy_sensor(sensor);
}

static void test_aht20_read_propagates_transport_errors_and_preserves_output(void)
{
    int fake_bus;
    sensor_data_t data;
    sensor_data_t before;
    sensor_device_t *sensor = aht20_create_temperature("aht20-read-transport", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    memset(&data, 0xA5, sizeof(data));
    memcpy(&before, &data, sizeof(before));

    g_trigger_write_result = SENSOR_ETIMEOUT;
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&before, &data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT32(0U, g_delay_total_ms);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_count);

    g_trigger_write_result = SENSOR_EOK;
    g_measurement_read_result = SENSOR_ETIMEOUT;
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&before, &data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT32(80U, g_delay_total_ms);
    TEST_ASSERT_EQUAL_UINT(1U, g_read_count);

    destroy_sensor(sensor);
}

static void test_aht20_temperature_read_propagates_busy(void)
{
    int fake_bus;
    sensor_device_t *sensor = aht20_create_temperature("aht20-temp", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    assert_busy_preserves_output(sensor);
    destroy_sensor(sensor);
}

static void test_aht20_humidity_read_propagates_busy(void)
{
    int fake_bus;
    sensor_device_t *sensor = aht20_create_humidity("aht20-humidity", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    assert_busy_preserves_output(sensor);
    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_aht20_deinit_rejects_null_context);
    RUN_TEST(test_aht20_init_propagates_soft_reset_failure_without_delay);
    RUN_TEST(test_aht20_init_propagates_command_and_status_transport_errors);
    RUN_TEST(test_aht20_read_propagates_transport_errors_and_preserves_output);
    RUN_TEST(test_aht20_temperature_read_propagates_busy);
    RUN_TEST(test_aht20_humidity_read_propagates_busy);
    return UNITY_END();
}
