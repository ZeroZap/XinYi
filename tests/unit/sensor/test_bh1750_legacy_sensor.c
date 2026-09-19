#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_bh1750.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint8_t data[2];
    size_t len;
    xy_error_t result;
} read_call_t;

typedef struct {
    uint8_t data;
    xy_error_t result;
} write_call_t;

static read_call_t reads[8];
static write_call_t writes[16];
static size_t read_index, read_count, write_index, write_count;
static uint32_t tick, delay_total;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *bus, uint16_t addr, uint32_t timeout)
{
    memset(dev, 0, sizeof(*dev));
    dev->i2c_handle = bus;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    dev->base.initialized = true;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    (void)dev;
    TEST_ASSERT_EQUAL_UINT(1U, len);
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(writes), write_index);
    writes[write_index].data = data[0];
    return writes[write_index++].result;
}

xy_error_t xy_i2c_device_read(xy_i2c_device_t *dev, uint8_t *data, size_t len)
{
    (void)dev;
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(reads), read_index);
    TEST_ASSERT_EQUAL_UINT(reads[read_index].len, len);
    if (reads[read_index].result == XY_DEVICE_OK) memcpy(data, reads[read_index].data, len);
    return reads[read_index++].result;
}

void xy_hal_delay_ms(uint32_t ms) { delay_total += ms; tick += ms; }
uint32_t xy_hal_sys_get_tick_count(void) { return tick; }
int xy_printf(const char *fmt, ...) { (void)fmt; return 0; }
uint32_t get_tick_ms(void) { return tick; }
void delay_ms(uint32_t ms) { xy_hal_delay_ms(ms); }

void setUp(void)
{
    memset(reads, 0, sizeof(reads));
    memset(writes, 0, sizeof(writes));
    read_index = write_index = 0U;
    read_count = write_count = 0U;
    tick = 424242U;
    delay_total = 0U;
}
void tearDown(void) {}

static void queue_read(uint16_t raw, xy_error_t result)
{
    reads[read_count].data[0] = (uint8_t)(raw >> 8);
    reads[read_count].data[1] = (uint8_t)raw;
    reads[read_count].len = 2U;
    reads[read_count++].result = result;
}

static sensor_device_t *create_sensor(void)
{
    static int bus;
    sensor_device_t *sensor = bh1750_create("bh1750", &bus);
    TEST_ASSERT_NOT_NULL(sensor);
    return sensor;
}

static void destroy_sensor(sensor_device_t *sensor)
{
    if (sensor != NULL) {
        SENSOR_FREE(sensor->priv_data);
        SENSOR_FREE(sensor);
    }
}

static void init_ok(sensor_device_t *sensor)
{
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_POWER_ON, writes[0].data);
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_RESET, writes[1].data);
    TEST_ASSERT_EQUAL_UINT32(20U, delay_total);
}

static void test_create_and_delegate_init(void)
{
    sensor_device_t *sensor = create_sensor();
    TEST_ASSERT_EQUAL_STRING("ROHM", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("BH1750", sensor->info.model);
    TEST_ASSERT_EQUAL_UINT8(BH1750_ADDR, ((bh1750_priv_t *)sensor->priv_data)->i2c_addr);
    init_ok(sensor);
    TEST_ASSERT_TRUE(((bh1750_priv_t *)sensor->priv_data)->device.initialized);
    destroy_sensor(sensor);
}

static void test_read_delegates_and_preserves_legacy_lux_contract(void)
{
    sensor_device_t *sensor = create_sensor();
    sensor_data_t data = {0};
    init_ok(sensor);
    queue_read(480U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_POWER_ON, writes[2].data);
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_ONCE_H, writes[3].data);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_LIGHT, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_LUX, data.unit);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 400.0f, data.value.val_float);
    destroy_sensor(sensor);
}

static void test_read_failure_preserves_output(void)
{
    sensor_device_t *sensor = create_sensor();
    sensor_data_t data, snapshot;
    init_ok(sensor);
    memset(&data, 0xA5, sizeof(data)); snapshot = data;
    queue_read(0U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));
    destroy_sensor(sensor);
}

static void test_init_failure_propagates_and_clears_canonical_lifecycle(void)
{
    sensor_device_t *sensor = create_sensor();
    writes[0].result = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->init(sensor));
    TEST_ASSERT_FALSE(((bh1750_priv_t *)sensor->priv_data)->device.initialized);
    TEST_ASSERT_FALSE(((bh1750_priv_t *)sensor->priv_data)->device.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_UINT(1U, write_index);
    destroy_sensor(sensor);
}

static void test_deinit_delegates_power_down_and_is_fail_closed(void)
{
    sensor_device_t *sensor = create_sensor();
    init_ok(sensor);
    writes[write_index].result = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->deinit(sensor));
    TEST_ASSERT_TRUE(((bh1750_priv_t *)sensor->priv_data)->device.initialized);
    writes[write_index].result = XY_DEVICE_OK;
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));
    TEST_ASSERT_FALSE(((bh1750_priv_t *)sensor->priv_data)->device.initialized);
    destroy_sensor(sensor);
}

static void test_public_guards_have_no_bus_side_effects(void)
{
    sensor_device_t *sensor = create_sensor();
    sensor_data_t data;
    TEST_ASSERT_NULL(bh1750_create(NULL, sensor->bus));
    TEST_ASSERT_NULL(bh1750_create("bad", NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(NULL));
    TEST_ASSERT_EQUAL_UINT(0U, write_index);
    TEST_ASSERT_EQUAL_UINT(0U, read_index);
    destroy_sensor(sensor);
}

static void test_long_name_is_terminated(void)
{
    static int bus;
    char name[SENSOR_NAME_MAX_LEN * 2U];
    memset(name, 'B', sizeof(name)); name[sizeof(name) - 1U] = '\0';
    sensor_device_t *sensor = bh1750_create(name, &bus);
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_UINT8('\0', sensor->info.name[SENSOR_NAME_MAX_LEN - 1U]);
    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_create_and_delegate_init);
    RUN_TEST(test_read_delegates_and_preserves_legacy_lux_contract);
    RUN_TEST(test_read_failure_preserves_output);
    RUN_TEST(test_init_failure_propagates_and_clears_canonical_lifecycle);
    RUN_TEST(test_deinit_delegates_power_down_and_is_fail_closed);
    RUN_TEST(test_public_guards_have_no_bus_side_effects);
    RUN_TEST(test_long_name_is_terminated);
    return UNITY_END();
}
