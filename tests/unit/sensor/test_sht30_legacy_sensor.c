#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_sht30.h"
#include "xy_sht30.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint8_t bytes[8];
    size_t len;
    int ret;
} i2c_op_t;

static i2c_op_t g_writes[8];
static i2c_op_t g_reads[8];
static size_t g_write_count;
static size_t g_write_index;
static size_t g_read_count;
static size_t g_read_index;
static int g_init_ret;
static uint16_t g_init_addr;
static uint32_t g_delay_ms;
static uint32_t g_tick;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle, uint16_t addr,
                              uint32_t timeout)
{
    if (g_init_ret != XY_DEVICE_OK) {
        return g_init_ret;
    }
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = true;
    dev->i2c_handle = i2c_handle;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    g_init_addr = addr;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_LESS_THAN_UINT(g_write_count, g_write_index);
    i2c_op_t *op = &g_writes[g_write_index++];
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    TEST_ASSERT_EQUAL_MEMORY(op->bytes, data, len);
    return op->ret;
}

xy_error_t xy_i2c_device_read(xy_i2c_device_t *dev, uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_LESS_THAN_UINT(g_read_count, g_read_index);
    i2c_op_t *op = &g_reads[g_read_index++];
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    if (op->ret == XY_DEVICE_OK) {
        memcpy(data, op->bytes, len);
    }
    return op->ret;
}

void xy_hal_delay_ms(uint32_t ms)
{
    g_delay_ms += ms;
    g_tick += ms;
}

void delay_ms(uint32_t ms)
{
    g_tick += ms;
}

uint32_t get_tick_ms(void)
{
    return g_tick;
}

static uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFFU;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static void queue_write(const uint8_t *bytes, size_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_writes), g_write_count);
    i2c_op_t *op = &g_writes[g_write_count++];
    memcpy(op->bytes, bytes, len);
    op->len = len;
    op->ret = ret;
}

static void queue_measurement(uint16_t temperature, uint16_t humidity, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_count);
    i2c_op_t *op = &g_reads[g_read_count++];
    op->bytes[0] = (uint8_t)(temperature >> 8);
    op->bytes[1] = (uint8_t)temperature;
    op->bytes[2] = crc8(op->bytes, 2U);
    op->bytes[3] = (uint8_t)(humidity >> 8);
    op->bytes[4] = (uint8_t)humidity;
    op->bytes[5] = crc8(&op->bytes[3], 2U);
    op->len = 6U;
    op->ret = ret;
}

static void assert_queues_drained(void)
{
    TEST_ASSERT_EQUAL_UINT(g_write_count, g_write_index);
    TEST_ASSERT_EQUAL_UINT(g_read_count, g_read_index);
}

static void destroy_sensor(sensor_device_t *sensor)
{
    if (sensor != NULL) {
        SENSOR_FREE(sensor->priv_data);
        SENSOR_FREE(sensor);
    }
}

void setUp(void)
{
    memset(g_writes, 0, sizeof(g_writes));
    memset(g_reads, 0, sizeof(g_reads));
    g_write_count = 0U;
    g_write_index = 0U;
    g_read_count = 0U;
    g_read_index = 0U;
    g_init_ret = XY_DEVICE_OK;
    g_init_addr = 0U;
    g_delay_ms = 0U;
    g_tick = 24680U;
}

void tearDown(void)
{
}

static void test_legacy_wrapper_delegates_init_and_humidity_read(void)
{
    int fake_bus;
    const uint8_t reset_cmd[2] = {0x30U, 0xA2U};
    const uint8_t measure_cmd[2] = {0x2CU, 0x06U};
    sensor_data_t data = {0};
    sensor_device_t *sensor = sht30_create("sht30-main", &fake_bus, 0U);

    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_EQUAL_STRING("sht30-main", sensor->info.name);
    TEST_ASSERT_EQUAL_STRING("Sensirion", sensor->info.vendor);
    TEST_ASSERT_EQUAL_STRING("SHT30", sensor->info.model);
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_RELATIVE_HUMIDITY, sensor->info.type);
    TEST_ASSERT_EQUAL_PTR(&fake_bus, sensor->bus);

    queue_write(reset_cmd, sizeof(reset_cmd), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT16(SHT30_ADDR_DEFAULT, g_init_addr);

    queue_write(measure_cmd, sizeof(measure_cmd), XY_DEVICE_OK);
    queue_measurement(0x6666U, 0x8000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_TYPE_RELATIVE_HUMIDITY, data.type);
    TEST_ASSERT_EQUAL_INT(SENSOR_UNIT_PERCENT, data.unit);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, data.value.val_float);
    TEST_ASSERT_EQUAL_UINT32(24695U, data.timestamp);
    TEST_ASSERT_EQUAL_UINT32(15U, g_delay_ms);

    assert_queues_drained();
    destroy_sensor(sensor);
}

static void test_legacy_wrapper_maps_errors_and_preserves_output(void)
{
    int fake_bus;
    const uint8_t reset_cmd[2] = {0x30U, 0xA2U};
    const uint8_t measure_cmd[2] = {0x2CU, 0x06U};
    sensor_data_t data = {.type = SENSOR_TYPE_TEMPERATURE,
                          .unit = SENSOR_UNIT_CELSIUS,
                          .value.val_float = -12.5f,
                          .timestamp = 123U,
                          .accuracy = 7U};
    sensor_data_t snapshot = data;
    sensor_device_t *sensor = sht30_create("sht30-alt", &fake_bus, SHT30_ADDR_ALT);

    TEST_ASSERT_NULL(sht30_create(NULL, &fake_bus, SHT30_ADDR_DEFAULT));
    TEST_ASSERT_NOT_NULL(sensor);

    queue_write(reset_cmd, sizeof(reset_cmd), XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->init(sensor));

    queue_write(reset_cmd, sizeof(reset_cmd), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(SENSOR_EOK, sensor->ops->init(sensor));
    TEST_ASSERT_EQUAL_UINT16(SHT30_ADDR_ALT, g_init_addr);

    queue_write(measure_cmd, sizeof(measure_cmd), XY_DEVICE_OK);
    queue_measurement(0x8000U, 0x4000U, XY_DEVICE_OK);
    g_reads[0].bytes[5] ^= 0x01U;
    TEST_ASSERT_EQUAL_INT(SENSOR_EIO, sensor->ops->read(sensor, &data));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &data, sizeof(data));

    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->init(NULL));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(NULL, &data));
    TEST_ASSERT_EQUAL_INT(SENSOR_EINVAL, sensor->ops->read(sensor, NULL));
    assert_queues_drained();

    destroy_sensor(sensor);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_legacy_wrapper_delegates_init_and_humidity_read);
    RUN_TEST(test_legacy_wrapper_maps_errors_and_preserves_output);
    return UNITY_END();
}
