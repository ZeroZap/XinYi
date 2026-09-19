#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "sensor_aht20.h"

#define READ_QUEUE_CAPACITY 64U

static uint8_t g_reads[READ_QUEUE_CAPACITY][7];
static size_t g_read_lens[READ_QUEUE_CAPACITY];
static int g_read_results[READ_QUEUE_CAPACITY];
static size_t g_read_count;
static size_t g_read_index;
static uint8_t g_writes[4][3];
static size_t g_write_lens[4];
static size_t g_write_count;
static uint32_t g_tick;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t addr, uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(handle);
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = true;
    dev->i2c_handle = handle;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read(xy_i2c_device_t *dev, uint8_t *data, size_t len)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_LESS_THAN_UINT(g_read_count, g_read_index);
    TEST_ASSERT_EQUAL_UINT(g_read_lens[g_read_index], len);
    int result = g_read_results[g_read_index];
    if (result == XY_DEVICE_OK) {
        memcpy(data, g_reads[g_read_index], len);
    }
    g_read_index++;
    return result;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_LESS_THAN_UINT(4U, g_write_count);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(3U, len);
    memcpy(g_writes[g_write_count], data, len);
    g_write_lens[g_write_count++] = len;
    return XY_DEVICE_OK;
}

uint32_t xy_os_tick_get(void)
{
    return g_tick;
}

void xy_os_delay(uint32_t ms)
{
    g_tick += ms;
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

static void queue_read(const uint8_t *data, size_t len, int result)
{
    TEST_ASSERT_LESS_THAN_UINT(READ_QUEUE_CAPACITY, g_read_count);
    if (data != NULL) {
        memcpy(g_reads[g_read_count], data, len);
    }
    g_read_lens[g_read_count] = len;
    g_read_results[g_read_count++] = result;
}

static void queue_status(uint8_t status)
{
    queue_read(&status, 1U, XY_DEVICE_OK);
}

static void queue_measurement(uint32_t humidity_raw, uint32_t temperature_raw)
{
    uint8_t data[7] = {0U,
                       (uint8_t)(humidity_raw >> 12),
                       (uint8_t)(humidity_raw >> 4),
                       (uint8_t)(((humidity_raw & 0x0FU) << 4) |
                                 ((temperature_raw >> 16) & 0x0FU)),
                       (uint8_t)(temperature_raw >> 8),
                       (uint8_t)temperature_raw,
                       0U};
    queue_read(data, sizeof(data), XY_DEVICE_OK);
}

void setUp(void)
{
    memset(g_reads, 0, sizeof(g_reads));
    memset(g_read_lens, 0, sizeof(g_read_lens));
    memset(g_read_results, 0, sizeof(g_read_results));
    memset(g_writes, 0, sizeof(g_writes));
    memset(g_write_lens, 0, sizeof(g_write_lens));
    g_read_count = 0U;
    g_read_index = 0U;
    g_write_count = 0U;
    g_tick = 1000U;
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

static sensor_device_t *create_initialized(bool humidity)
{
    static int fake_bus;
    sensor_device_t *sensor = humidity ? aht20_create_humidity("aht20-humidity", &fake_bus)
                                       : aht20_create_temperature("aht20-temp", &fake_bus);
    TEST_ASSERT_NOT_NULL(sensor);
    queue_status(0U);
    queue_status(0x08U);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    return sensor;
}

static void test_factories_reject_invalid_inputs_and_preserve_metadata(void)
{
    int fake_bus;
    TEST_ASSERT_NULL(aht20_create_temperature(NULL, &fake_bus));
    TEST_ASSERT_NULL(aht20_create_temperature("temp", NULL));
    TEST_ASSERT_NULL(aht20_create_humidity(NULL, &fake_bus));
    TEST_ASSERT_NULL(aht20_create_humidity("humidity", NULL));

    sensor_device_t *temp = aht20_create_temperature("temp", &fake_bus);
    sensor_device_t *humidity = aht20_create_humidity("humidity", &fake_bus);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_TEMPERATURE, temp->info.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_HUMIDITY, humidity->info.type);
    TEST_ASSERT_EQUAL_STRING("AHT20", temp->info.model);
    destroy_sensor(temp);
    destroy_sensor(humidity);
}

static void test_wrapper_delegates_lifecycle_to_canonical_owner(void)
{
    sensor_device_t *sensor = create_initialized(false);
    aht20_priv_t *priv = (aht20_priv_t *)sensor->priv_data;

    TEST_ASSERT_TRUE(priv->device.initialized);
    TEST_ASSERT_EQUAL_UINT8(AHT20_CMD_INIT, g_writes[0][0]);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->deinit(sensor));
    TEST_ASSERT_FALSE(priv->device.initialized);
    TEST_ASSERT_FALSE(priv->device.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->deinit(NULL));
    destroy_sensor(sensor);
}

static void test_temperature_wrapper_converts_and_preserves_output_on_failure(void)
{
    sensor_device_t *sensor = create_initialized(false);
    sensor_data_t data;
    sensor_data_t before;

    queue_status(0U);
    queue_status(0U);
    queue_measurement(0x80000U, 0x80000U);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_TEMPERATURE, data.type);
#if SENSOR_USE_FLOAT
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, data.value.val_float);
#else
    TEST_ASSERT_EQUAL_INT32(5000, data.value.val_int32);
#endif

    memcpy(&before, &data, sizeof(data));
    queue_read(NULL, 1U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(SENSOR_ETIMEOUT, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&before, &data, sizeof(data));
    destroy_sensor(sensor);
}

static void test_humidity_wrapper_converts_and_preserves_output_on_failure(void)
{
    sensor_device_t *sensor = create_initialized(true);
    sensor_data_t data;
    sensor_data_t before;

    queue_status(0U);
    queue_status(0U);
    queue_measurement(0x40000U, 0x40000U);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_HUMIDITY, data.type);
#if SENSOR_USE_FLOAT
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, data.value.val_float);
#else
    TEST_ASSERT_EQUAL_INT32(2500, data.value.val_int32);
#endif

    memcpy(&before, &data, sizeof(data));
    for (unsigned int i = 0U; i < 51U; ++i) {
        queue_status(0x80U);
    }
    TEST_ASSERT_EQUAL_INT(SENSOR_EBUSY, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&before, &data, sizeof(data));
    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_factories_reject_invalid_inputs_and_preserve_metadata);
    RUN_TEST(test_wrapper_delegates_lifecycle_to_canonical_owner);
    RUN_TEST(test_temperature_wrapper_converts_and_preserves_output_on_failure);
    RUN_TEST(test_humidity_wrapper_converts_and_preserves_output_on_failure);
    return UNITY_END();
}
