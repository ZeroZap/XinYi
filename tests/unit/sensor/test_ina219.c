#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_ina219.h"

static uint8_t g_read_buf[2];
static int g_read_ret;
static void *g_last_bus;
static uint8_t g_last_addr;
static uint8_t g_last_reg;
static uint16_t g_last_len;
static uint32_t g_tick;
static unsigned int g_read_count;
uint32_t get_tick_ms(void)
{
    return g_tick;
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    g_last_bus = bus;
    g_last_addr = addr;
    g_last_reg = reg;
    g_last_len = len;
    g_read_count++;
    if (g_read_ret == SENSOR_EOK) {
        memcpy(data, g_read_buf, len);
    }
    return g_read_ret;
}

void setUp(void)
{
    memset(g_read_buf, 0, sizeof(g_read_buf));
    g_read_ret = SENSOR_EOK;
    g_last_bus = NULL;
    g_last_addr = 0;
    g_last_reg = 0;
    g_last_len = 0;
    g_tick = 123456U;
    g_read_count = 0;
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

static void test_create_populates_identity_ops_and_private_address(void)
{
    int fake_bus;

    sensor_device_t *sensor = ina219_create("ina219-main", &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("ina219-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("TI", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("INA219", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_CURRENT, sensor->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_STATUS_IDLE, sensor->status);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);
    TEST_ASSERT_NOT_NULL(sensor->ops);
    TEST_ASSERT_NOT_NULL(sensor->ops->init);
    TEST_ASSERT_NOT_NULL(sensor->ops->read);
    TEST_ASSERT_NOT_NULL(sensor->priv_data);
    TEST_ASSERT_EQUAL_UINT8(INA219_ADDR, ((ina219_priv_t *)sensor->priv_data)->i2c_addr);

    destroy_sensor(sensor);
}

static void test_create_truncates_long_name_and_keeps_terminator(void)
{
    int fake_bus;
    char long_name[SENSOR_NAME_MAX_LEN * 2U];
    memset(long_name, 'A', sizeof(long_name));
    long_name[sizeof(long_name) - 1U] = '\0';

    sensor_device_t *sensor = ina219_create(long_name, &fake_bus);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_UINT8('\0', sensor->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    TEST_ASSERT_EQUAL_UINT(SENSOR_NAME_MAX_LEN - 1U, strlen(sensor->info.name));

    destroy_sensor(sensor);
}

static void test_read_converts_signed_shunt_register_to_current_and_timestamp(void)
{
    int fake_bus;
    sensor_data_t data = {0};
    sensor_device_t *sensor = ina219_create("ina219", &fake_bus);
    TEST_ASSERT_NOT_NULL(sensor);
    g_read_buf[0] = 0x01U;
    g_read_buf[1] = 0xF4U; /* +500 * 0.1 = 50.0 */

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));

    TEST_ASSERT_EQUAL_UINT(1U, g_read_count);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, g_last_bus);
    TEST_ASSERT_EQUAL_UINT8(INA219_ADDR, g_last_addr);
    TEST_ASSERT_EQUAL_UINT8(0x01U, g_last_reg);
    TEST_ASSERT_EQUAL_UINT16(2U, g_last_len);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_CURRENT, data.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);

    g_read_buf[0] = 0xFFU;
    g_read_buf[1] = 0x9CU; /* -100 * 0.1 = -10.0 */
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -10.0f, data.value.val_float);

    destroy_sensor(sensor);
}

static void test_init_returns_ok_without_touching_bus(void)
{
    int fake_bus;
    sensor_device_t *sensor = ina219_create("ina219", &fake_bus);
    TEST_ASSERT_NOT_NULL(sensor);

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT(0U, g_read_count);

    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_create_populates_identity_ops_and_private_address);
    RUN_TEST(test_create_truncates_long_name_and_keeps_terminator);
    RUN_TEST(test_read_converts_signed_shunt_register_to_current_and_timestamp);
    RUN_TEST(test_init_returns_ok_without_touching_bus);
    return UNITY_END();
}
