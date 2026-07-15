#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_device.h"
#include "xy_sht40.h"

static uint8_t g_read_queue[16][6];
static size_t g_read_len_queue[16];
static int g_read_ret_queue[16];
static size_t g_read_count;
static size_t g_read_index;
static uint8_t g_write_queue[8];
static int g_write_ret_queue[8];
static size_t g_write_count;
static size_t g_write_index;
static uint32_t g_tick;
static uint32_t g_delay_total;
static uint8_t g_last_addr;

static uint8_t sht40_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle, uint16_t addr, uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(i2c_handle);
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1;
    dev->i2c_handle = i2c_handle;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    g_last_addr = (uint8_t)addr;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read(xy_i2c_device_t *dev, uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_read_queue) / sizeof(g_read_queue[0]), g_read_index);
    TEST_ASSERT_EQUAL_UINT(g_read_len_queue[g_read_index], len);

    int ret = g_read_ret_queue[g_read_index];
    if (ret == XY_DEVICE_OK) {
        memcpy(data, g_read_queue[g_read_index], len);
    }
    g_read_index++;
    return ret;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT(1U, len);
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_write_queue), g_write_count);
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_write_ret_queue) / sizeof(g_write_ret_queue[0]), g_write_index);

    g_write_queue[g_write_count++] = data[0];
    return g_write_ret_queue[g_write_index++];
}

uint32_t xy_os_tick_get(void)
{
    return g_tick;
}

void xy_os_delay(uint32_t ms)
{
    g_delay_total += ms;
    g_tick += ms;
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

static void queue_read(const uint8_t *data, size_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_read_queue) / sizeof(g_read_queue[0]), g_read_count);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_read_queue[0]), len);
    if (data != NULL) {
        memcpy(g_read_queue[g_read_count], data, len);
    }
    g_read_len_queue[g_read_count] = len;
    g_read_ret_queue[g_read_count] = ret;
    g_read_count++;
}

static void queue_pair_payload(uint16_t first, uint16_t second)
{
    uint8_t data[6] = {
        (uint8_t)(first >> 8),
        (uint8_t)first,
        0U,
        (uint8_t)(second >> 8),
        (uint8_t)second,
        0U,
    };
    data[2] = sht40_crc8(data, 2U);
    data[5] = sht40_crc8(&data[3], 2U);
    queue_read(data, sizeof(data), XY_DEVICE_OK);
}

void setUp(void)
{
    memset(g_read_queue, 0, sizeof(g_read_queue));
    memset(g_read_len_queue, 0, sizeof(g_read_len_queue));
    memset(g_read_ret_queue, 0, sizeof(g_read_ret_queue));
    memset(g_write_queue, 0, sizeof(g_write_queue));
    for (size_t i = 0; i < sizeof(g_write_ret_queue) / sizeof(g_write_ret_queue[0]); ++i) {
        g_write_ret_queue[i] = XY_DEVICE_OK;
    }
    g_read_count = 0;
    g_read_index = 0;
    g_write_count = 0;
    g_write_index = 0;
    g_tick = 2000;
    g_delay_total = 0;
    g_last_addr = 0;
}

void tearDown(void)
{
}

static void test_init_rejects_invalid_inputs_and_reads_serial(void)
{
    xy_sht40_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_SHT40_INVALID_PARAM, xy_sht40_init(NULL, &fake_bus));
    TEST_ASSERT_EQUAL_INT(XY_SHT40_INVALID_PARAM, xy_sht40_init(&dev, NULL));

    queue_pair_payload(0x1234U, 0xABCDU);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_init(&dev, &fake_bus));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(SHT40_ADDR, g_last_addr);
    TEST_ASSERT_EQUAL_UINT8(SHT40_CMD_READ_SERIAL, g_write_queue[0]);
    TEST_ASSERT_EQUAL_UINT32(0x1234U, dev.data.serial[0]);
    TEST_ASSERT_EQUAL_UINT32(0xABCDU, dev.data.serial[1]);
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_total);
}

static void test_init_rejects_bad_serial_crc_without_initializing(void)
{
    xy_sht40_t dev;
    int fake_bus;
    uint8_t serial[6] = {0x12, 0x34, 0x00, 0xAB, 0xCD, 0x00};

    serial[2] = (uint8_t)(sht40_crc8(serial, 2U) ^ 0x55U);
    serial[5] = sht40_crc8(&serial[3], 2U);
    queue_read(serial, sizeof(serial), XY_DEVICE_OK);

    TEST_ASSERT_EQUAL_INT(XY_SHT40_CRC_ERROR, xy_sht40_init(&dev, &fake_bus));
    TEST_ASSERT_FALSE(dev.initialized);
}

static void test_read_uses_precision_command_and_converts_measurement(void)
{
    xy_sht40_t dev;
    int fake_bus;

    queue_pair_payload(0x1234U, 0xABCDU);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_init(&dev, &fake_bus));
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_set_precision(&dev, XY_SHT40_MEDIUM_PRECISION));

    queue_pair_payload(0x8000U, 0x8000U);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_read(&dev));
    TEST_ASSERT_EQUAL_UINT(2U, g_write_count);
    TEST_ASSERT_EQUAL_UINT8(SHT40_CMD_MEASURE_MPM, g_write_queue[1]);
    TEST_ASSERT_EQUAL_INT16(4250, dev.data.temperature);
    TEST_ASSERT_EQUAL_UINT16(5650U, dev.data.humidity);
    TEST_ASSERT_EQUAL_UINT32(g_tick, dev.data.timestamp);
    TEST_ASSERT_EQUAL_UINT32(35U, g_delay_total);
}

static void test_read_bad_crc_does_not_overwrite_cached_measurement(void)
{
    xy_sht40_t dev;
    int fake_bus;
    uint8_t bad_measurement[6] = {0x80, 0x00, 0x00, 0x80, 0x00, 0x00};

    queue_pair_payload(0x1234U, 0xABCDU);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_init(&dev, &fake_bus));

    queue_pair_payload(0x8000U, 0x8000U);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_read(&dev));
    TEST_ASSERT_EQUAL_INT16(4250, dev.data.temperature);
    TEST_ASSERT_EQUAL_UINT16(5650U, dev.data.humidity);

    bad_measurement[2] = (uint8_t)(sht40_crc8(bad_measurement, 2U) ^ 0xAAU);
    bad_measurement[5] = sht40_crc8(&bad_measurement[3], 2U);
    queue_read(bad_measurement, sizeof(bad_measurement), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_CRC_ERROR, xy_sht40_read(&dev));
    TEST_ASSERT_EQUAL_INT16(4250, dev.data.temperature);
    TEST_ASSERT_EQUAL_UINT16(5650U, dev.data.humidity);

    bad_measurement[2] = sht40_crc8(bad_measurement, 2U);
    bad_measurement[5] = (uint8_t)(sht40_crc8(&bad_measurement[3], 2U) ^ 0x55U);
    queue_read(bad_measurement, sizeof(bad_measurement), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_CRC_ERROR, xy_sht40_read(&dev));
    TEST_ASSERT_EQUAL_INT16(4250, dev.data.temperature);
    TEST_ASSERT_EQUAL_UINT16(5650U, dev.data.humidity);
}

static void test_read_i2c_read_failure_preserves_cached_measurement(void)
{
    xy_sht40_t dev;
    int fake_bus;

    queue_pair_payload(0x1234U, 0xABCDU);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_init(&dev, &fake_bus));

    dev.data.temperature = 123;
    dev.data.humidity = 456U;
    dev.data.timestamp = 0xBEEFU;
    queue_read(NULL, 6U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_sht40_read(&dev));
    TEST_ASSERT_EQUAL_INT16(123, dev.data.temperature);
    TEST_ASSERT_EQUAL_UINT16(456U, dev.data.humidity);
    TEST_ASSERT_EQUAL_UINT32(0xBEEFU, dev.data.timestamp);
}

static void test_init_reports_i2c_write_and_read_failures(void)
{
    xy_sht40_t dev;
    int fake_bus;

    g_write_ret_queue[0] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_SHT40_NOT_FOUND, xy_sht40_init(&dev, &fake_bus));
    TEST_ASSERT_FALSE(dev.initialized);

    setUp();
    queue_read(NULL, 6U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_sht40_init(&dev, &fake_bus));
    TEST_ASSERT_FALSE(dev.initialized);
}

static void test_deinit_rejects_null_and_clears_initialized_flag(void)
{
    xy_sht40_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_SHT40_INVALID_PARAM, xy_sht40_deinit(NULL));

    queue_pair_payload(0x1234U, 0xABCDU);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_init(&dev, &fake_bus));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
}

static void test_read_rejects_invalid_or_uninitialized_and_propagates_write_failure(void)
{
    xy_sht40_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_SHT40_INVALID_PARAM, xy_sht40_read(NULL));
    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_SHT40_INVALID_PARAM, xy_sht40_read(&dev));

    queue_pair_payload(0x1234U, 0xABCDU);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_init(&dev, &fake_bus));
    g_write_ret_queue[g_write_index] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_sht40_read(&dev));
}

static void test_low_precision_read_uses_low_power_command_and_delay(void)
{
    xy_sht40_t dev;
    int fake_bus;

    queue_pair_payload(0x1234U, 0xABCDU);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_init(&dev, &fake_bus));
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_set_precision(&dev, XY_SHT40_LOW_PRECISION));

    queue_pair_payload(0xFFFFU, 0x0000U);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_read(&dev));
    TEST_ASSERT_EQUAL_UINT8(SHT40_CMD_MEASURE_LPM, g_write_queue[1]);
    TEST_ASSERT_EQUAL_UINT32(20U, g_delay_total); /* 10ms serial + 10ms low precision */
    TEST_ASSERT_INT16_WITHIN(1, 12999, dev.data.temperature);
    TEST_ASSERT_EQUAL_UINT16(64936U, dev.data.humidity); /* unsigned wrap documents current behavior */
}

static void test_getters_and_precision_validate_inputs(void)
{
    xy_sht40_t dev;
    uint32_t serial[2] = {0, 0};
    int16_t temperature = -1;
    uint16_t humidity = 0xBEEF;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_SHT40_INVALID_PARAM, xy_sht40_get_temperature(NULL, &temperature));
    TEST_ASSERT_EQUAL_INT(XY_SHT40_INVALID_PARAM, xy_sht40_get_temperature(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_SHT40_INVALID_PARAM, xy_sht40_get_humidity(NULL, &humidity));
    TEST_ASSERT_EQUAL_INT(XY_SHT40_INVALID_PARAM, xy_sht40_get_humidity(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_SHT40_INVALID_PARAM, xy_sht40_get_serial(NULL, serial));
    TEST_ASSERT_EQUAL_INT(XY_SHT40_INVALID_PARAM, xy_sht40_get_serial(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_SHT40_INVALID_PARAM, xy_sht40_set_precision(NULL, XY_SHT40_HIGH_PRECISION));
    TEST_ASSERT_EQUAL_INT(XY_SHT40_INVALID_PARAM,
                          xy_sht40_set_precision(&dev, (xy_sht40_precision_t)99));

    queue_pair_payload(0x1234U, 0xABCDU);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_init(&dev, &fake_bus));
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_get_serial(&dev, serial));
    TEST_ASSERT_EQUAL_UINT32(0x1234U, serial[0]);
    TEST_ASSERT_EQUAL_UINT32(0xABCDU, serial[1]);

    queue_pair_payload(0x4000U, 0x4000U);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_get_temperature(&dev, &temperature));
    TEST_ASSERT_EQUAL_INT16(-125, temperature);

    queue_pair_payload(0x6000U, 0x6000U);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_get_humidity(&dev, &humidity));
    TEST_ASSERT_EQUAL_UINT16(4087U, humidity);

    queue_read(NULL, 6U, XY_DEVICE_ERROR);
    humidity = 0xBEEF;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_sht40_get_humidity(&dev, &humidity));
    TEST_ASSERT_EQUAL_UINT16(0xBEEF, humidity);
}

static void test_temperature_getter_preserves_output_on_write_and_read_failures(void)
{
    xy_sht40_t dev;
    int16_t temperature = -2222;
    int fake_bus;

    queue_pair_payload(0x1234U, 0xABCDU);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_init(&dev, &fake_bus));

    g_write_ret_queue[g_write_index] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_sht40_get_temperature(&dev, &temperature));
    TEST_ASSERT_EQUAL_INT16(-2222, temperature);

    queue_read(NULL, 6U, XY_DEVICE_ERROR);
    temperature = 3333;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_sht40_get_temperature(&dev, &temperature));
    TEST_ASSERT_EQUAL_INT16(3333, temperature);
}

static void test_default_high_precision_read_uses_hpm_command_and_delay(void)
{
    xy_sht40_t dev;
    int fake_bus;

    queue_pair_payload(0x1234U, 0xABCDU);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_init(&dev, &fake_bus));

    queue_pair_payload(0x8000U, 0x8000U);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_read(&dev));
    TEST_ASSERT_EQUAL_UINT8(SHT40_CMD_MEASURE_HPM, g_write_queue[1]);
    TEST_ASSERT_EQUAL_UINT32(55U, g_delay_total); /* 10ms serial + 45ms high precision */
}

static void test_get_serial_copies_cached_serial_without_i2c_access(void)
{
    xy_sht40_t dev;
    uint32_t serial[2] = {0xDEADU, 0xBEEFU};
    int fake_bus;

    queue_pair_payload(0x1234U, 0xABCDU);
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_init(&dev, &fake_bus));
    TEST_ASSERT_EQUAL_INT(XY_SHT40_OK, xy_sht40_get_serial(&dev, serial));
    TEST_ASSERT_EQUAL_UINT32(0x1234U, serial[0]);
    TEST_ASSERT_EQUAL_UINT32(0xABCDU, serial[1]);
    TEST_ASSERT_EQUAL_UINT(1U, g_write_count);
    TEST_ASSERT_EQUAL_UINT(1U, g_read_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_invalid_inputs_and_reads_serial);
    RUN_TEST(test_init_rejects_bad_serial_crc_without_initializing);
    RUN_TEST(test_init_reports_i2c_write_and_read_failures);
    RUN_TEST(test_read_uses_precision_command_and_converts_measurement);
    RUN_TEST(test_read_bad_crc_does_not_overwrite_cached_measurement);
    RUN_TEST(test_read_i2c_read_failure_preserves_cached_measurement);
    RUN_TEST(test_deinit_rejects_null_and_clears_initialized_flag);
    RUN_TEST(test_read_rejects_invalid_or_uninitialized_and_propagates_write_failure);
    RUN_TEST(test_low_precision_read_uses_low_power_command_and_delay);
    RUN_TEST(test_getters_and_precision_validate_inputs);
    RUN_TEST(test_temperature_getter_preserves_output_on_write_and_read_failures);
    RUN_TEST(test_default_high_precision_read_uses_hpm_command_and_delay);
    RUN_TEST(test_get_serial_copies_cached_serial_without_i2c_access);
    return UNITY_END();
}
