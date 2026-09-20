#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_aht10.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint8_t data[8];
    size_t len;
    xy_error_t result;
} io_step_t;

static io_step_t g_reads[8];
static io_step_t g_writes[8];
static size_t g_read_count;
static size_t g_read_index;
static size_t g_write_count;
static size_t g_write_index;
static uint32_t g_tick;
static uint32_t g_delay_ms;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(handle);
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1U;
    dev->i2c_handle = handle;
    dev->dev_addr = address;
    dev->timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    io_step_t *step;

    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_SIZE(g_writes), g_write_index);
    step = &g_writes[g_write_index++];
    TEST_ASSERT_EQUAL_UINT(step->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(step->data, data, len);
    return step->result;
}

xy_error_t xy_i2c_device_read(xy_i2c_device_t *dev, uint8_t *data, size_t len)
{
    io_step_t *step;

    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_SIZE(g_reads), g_read_index);
    step = &g_reads[g_read_index++];
    TEST_ASSERT_EQUAL_UINT(step->len, len);
    if (step->result == XY_DEVICE_OK) {
        memcpy(data, step->data, len);
    }
    return step->result;
}

void xy_hal_delay_ms(uint32_t ms)
{
    g_delay_ms += ms;
    g_tick += ms;
}

uint32_t get_tick_ms(void)
{
    return g_tick;
}

void delay_ms(uint32_t ms)
{
    xy_hal_delay_ms(ms);
}

static void queue_write(const uint8_t *data, size_t len, xy_error_t result)
{
    io_step_t *step = &g_writes[g_write_count++];
    memcpy(step->data, data, len);
    step->len = len;
    step->result = result;
}

static void queue_read(const uint8_t *data, size_t len, xy_error_t result)
{
    io_step_t *step = &g_reads[g_read_count++];
    if (data != NULL) {
        memcpy(step->data, data, len);
    }
    step->len = len;
    step->result = result;
}

void setUp(void)
{
    memset(g_reads, 0, sizeof(g_reads));
    memset(g_writes, 0, sizeof(g_writes));
    g_read_count = 0U;
    g_read_index = 0U;
    g_write_count = 0U;
    g_write_index = 0U;
    g_tick = 1000U;
    g_delay_ms = 0U;
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

static void test_factory_and_lifecycle_delegate_to_canonical_owner(void)
{
    static const uint8_t init_command[] = {0xE1U, 0x08U, 0x00U};
    sensor_device_t *sensor;
    aht10_priv_t *priv;
    int bus;

    TEST_ASSERT_NULL(aht10_create(NULL, &bus, 0U));
    TEST_ASSERT_NULL(aht10_create("aht10", NULL, 0U));
    TEST_ASSERT_NULL(aht10_create("aht10", &bus, 0x39U));

    sensor = aht10_create("aht10-main", &bus, 0U);
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("AHT10", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_RELATIVE_HUMIDITY, sensor->info.type);
    priv = (aht10_priv_t *)sensor->priv_data;
    TEST_ASSERT_EQUAL_UINT8(AHT10_ADDR_DEFAULT, priv->i2c_addr);

    queue_write(init_command, sizeof(init_command), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_TRUE(priv->device.initialized);
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_ms);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));
    TEST_ASSERT_FALSE(priv->device.initialized);
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(sensor));

    destroy_sensor(sensor);
}

static void test_read_preserves_legacy_humidity_contract(void)
{
    static const uint8_t init_command[] = {0xE1U, 0x08U, 0x00U};
    static const uint8_t measure_command[] = {0xACU, 0x33U, 0x00U};
    static const uint8_t frame[] = {0x00U, 0x80U, 0x00U, 0x08U, 0x00U, 0x00U};
    sensor_data_t data = {0};
    sensor_device_t *sensor;
    int bus;

    sensor = aht10_create("aht10", &bus, 0U);
    TEST_ASSERT_NOT_NULL(sensor);
    queue_write(init_command, sizeof(init_command), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    queue_write(measure_command, sizeof(measure_command), XY_DEVICE_OK);
    queue_read(frame, sizeof(frame), XY_DEVICE_OK);

    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_RELATIVE_HUMIDITY, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_PERCENT, data.unit);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(g_tick, data.timestamp);
    TEST_ASSERT_EQUAL_UINT8(90U, data.accuracy);

    destroy_sensor(sensor);
}

static void test_transport_and_busy_errors_preserve_legacy_output(void)
{
    static const uint8_t init_command[] = {0xE1U, 0x08U, 0x00U};
    static const uint8_t measure_command[] = {0xACU, 0x33U, 0x00U};
    static const uint8_t busy_frame[] = {0x80U, 0U, 0U, 0U, 0U, 0U};
    sensor_data_t data = {
        .type = SENSOR_TYPE_TEMPERATURE,
        .unit = SENSOR_UNIT_CELSIUS,
        .value.val_float = 12.5f,
        .timestamp = 7U,
        .accuracy = 1U,
    };
    sensor_data_t before = data;
    sensor_device_t *sensor;
    int bus;

    sensor = aht10_create("aht10", &bus, 0U);
    TEST_ASSERT_NOT_NULL(sensor);
    queue_write(init_command, sizeof(init_command), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));

    queue_write(measure_command, sizeof(measure_command), XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&before, &data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT(0U, g_read_index);

    queue_write(measure_command, sizeof(measure_command), XY_DEVICE_OK);
    queue_read(NULL, 6U, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&before, &data, sizeof(data));

    queue_write(measure_command, sizeof(measure_command), XY_DEVICE_OK);
    queue_read(busy_frame, sizeof(busy_frame), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EBUSY, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&before, &data, sizeof(data));

    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_factory_and_lifecycle_delegate_to_canonical_owner);
    RUN_TEST(test_read_preserves_legacy_humidity_contract);
    RUN_TEST(test_transport_and_busy_errors_preserve_legacy_output);
    return UNITY_END();
}
