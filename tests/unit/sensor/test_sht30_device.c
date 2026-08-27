#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_sht30.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint8_t data[6];
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
static uint32_t g_delay_ms;

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
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_LESS_THAN_UINT(g_write_count, g_write_index);
    i2c_op_t *op = &g_writes[g_write_index++];
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    TEST_ASSERT_EQUAL_MEMORY(op->data, data, len);
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
        memcpy(data, op->data, len);
    }
    return op->ret;
}

void xy_hal_delay_ms(uint32_t ms)
{
    g_delay_ms += ms;
}

static uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFFU;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static void queue_write(const uint8_t *data, size_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_writes), g_write_count);
    i2c_op_t *op = &g_writes[g_write_count++];
    memcpy(op->data, data, len);
    op->len = len;
    op->ret = ret;
}

static void queue_measurement(uint16_t temperature, uint16_t humidity, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_count);
    i2c_op_t *op = &g_reads[g_read_count++];
    op->data[0] = (uint8_t)(temperature >> 8);
    op->data[1] = (uint8_t)temperature;
    op->data[2] = crc8(op->data, 2U);
    op->data[3] = (uint8_t)(humidity >> 8);
    op->data[4] = (uint8_t)humidity;
    op->data[5] = crc8(&op->data[3], 2U);
    op->len = 6U;
    op->ret = ret;
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
    g_delay_ms = 0U;
}

void tearDown(void)
{
}

static void test_init_propagates_helper_and_reset_failures(void)
{
    xy_sht30_t sensor;
    int bus;
    const uint8_t reset[] = {0x30U, 0xA2U};

    memset(&sensor, 0xA5, sizeof(sensor));
    g_init_ret = XY_DEVICE_INVALID_PARAM;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sht30_init(&sensor, &bus));
    TEST_ASSERT_FALSE(sensor.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_UINT(0U, g_write_index);

    g_init_ret = XY_DEVICE_OK;
    queue_write(reset, sizeof(reset), XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_sht30_init(&sensor, &bus));
    TEST_ASSERT_FALSE(sensor.i2c_dev.base.initialized);
}

static void test_read_checks_crc_and_preserves_cached_values(void)
{
    xy_sht30_t sensor;
    int bus;
    const uint8_t reset[] = {0x30U, 0xA2U};
    const uint8_t measure[] = {0x2CU, 0x06U};

    queue_write(reset, sizeof(reset), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sht30_init(&sensor, &bus));
    sensor.temperature = 1234;
    sensor.humidity = 5678U;

    queue_write(measure, sizeof(measure), XY_DEVICE_OK);
    queue_measurement(0x8000U, 0x4000U, XY_DEVICE_OK);
    g_reads[0].data[2] ^= 0x01U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_sht30_read(&sensor));
    TEST_ASSERT_EQUAL_INT16(1234, sensor.temperature);
    TEST_ASSERT_EQUAL_UINT16(5678U, sensor.humidity);
}

static void test_read_converts_valid_measurement(void)
{
    xy_sht30_t sensor;
    int bus;
    const uint8_t reset[] = {0x30U, 0xA2U};
    const uint8_t measure[] = {0x2CU, 0x06U};

    queue_write(reset, sizeof(reset), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sht30_init(&sensor, &bus));
    queue_write(measure, sizeof(measure), XY_DEVICE_OK);
    queue_measurement(0x8000U, 0x4000U, XY_DEVICE_OK);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sht30_read(&sensor));
    TEST_ASSERT_EQUAL_INT16(4250, sensor.temperature);
    TEST_ASSERT_EQUAL_UINT16(2500U, sensor.humidity);
    TEST_ASSERT_EQUAL_UINT32(15U, g_delay_ms);
}

static void test_read_rejects_uninitialized_device_without_io(void)
{
    xy_sht30_t sensor;
    memset(&sensor, 0, sizeof(sensor));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sht30_read(&sensor));
    TEST_ASSERT_EQUAL_UINT(0U, g_write_index);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_index);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_propagates_helper_and_reset_failures);
    RUN_TEST(test_read_checks_crc_and_preserves_cached_values);
    RUN_TEST(test_read_converts_valid_measurement);
    RUN_TEST(test_read_rejects_uninitialized_device_without_io);
    return UNITY_END();
}
