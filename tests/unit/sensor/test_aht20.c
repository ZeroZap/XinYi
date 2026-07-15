#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_aht20.h"
#include "xy_device.h"

static uint8_t g_read_queue[64][7];
static size_t g_read_len_queue[64];
static int g_read_ret_queue[64];
static size_t g_read_count;
static size_t g_read_index;
static uint8_t g_write_queue[8][4];
static size_t g_write_len_queue[8];
static int g_write_ret_queue[8];
static size_t g_write_count;
static size_t g_write_index;
static uint32_t g_tick;
static uint32_t g_delay_total;
static uint8_t g_last_addr;

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
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_write_queue) / sizeof(g_write_queue[0]), g_write_count);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_write_queue[0]), len);

    memcpy(g_write_queue[g_write_count], data, len);
    g_write_len_queue[g_write_count] = len;
    g_write_count++;
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

static void queue_status(uint8_t status)
{
    queue_read(&status, 1U, XY_DEVICE_OK);
}

static void queue_measurement(uint32_t humidity_raw, uint32_t temperature_raw)
{
    uint8_t data[7] = {
        0x00,
        (uint8_t)(humidity_raw >> 12),
        (uint8_t)(humidity_raw >> 4),
        (uint8_t)(((humidity_raw & 0x0FU) << 4) | ((temperature_raw >> 16) & 0x0FU)),
        (uint8_t)(temperature_raw >> 8),
        (uint8_t)temperature_raw,
        0x00,
    };
    queue_read(data, sizeof(data), XY_DEVICE_OK);
}

void setUp(void)
{
    memset(g_read_queue, 0, sizeof(g_read_queue));
    memset(g_read_len_queue, 0, sizeof(g_read_len_queue));
    memset(g_read_ret_queue, 0, sizeof(g_read_ret_queue));
    memset(g_write_queue, 0, sizeof(g_write_queue));
    memset(g_write_len_queue, 0, sizeof(g_write_len_queue));
    for (size_t i = 0; i < sizeof(g_write_ret_queue) / sizeof(g_write_ret_queue[0]); ++i) {
        g_write_ret_queue[i] = XY_DEVICE_OK;
    }
    g_read_count = 0;
    g_read_index = 0;
    g_write_count = 0;
    g_write_index = 0;
    g_tick = 1000;
    g_delay_total = 0;
    g_last_addr = 0;
}

void tearDown(void)
{
}

static void test_init_rejects_invalid_inputs_and_records_calibration_status(void)
{
    xy_aht20_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_AHT20_INVALID_PARAM, xy_aht20_init(NULL, &fake_bus));
    TEST_ASSERT_EQUAL_INT(XY_AHT20_INVALID_PARAM, xy_aht20_init(&dev, NULL));

    queue_status(0x00); /* init busy check */
    queue_status(0x08); /* calibration status */
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_init(&dev, &fake_bus));
    TEST_ASSERT_EQUAL_UINT8(AHT20_ADDR, g_last_addr);
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_TRUE(dev.calibrated);
    TEST_ASSERT_EQUAL_UINT(1U, g_write_count);
    TEST_ASSERT_EQUAL_UINT8(AHT20_CMD_INIT, g_write_queue[0][0]);
    TEST_ASSERT_EQUAL_UINT8(0x08, g_write_queue[0][1]);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_write_queue[0][2]);
}

static void test_init_reports_busy_after_timeout(void)
{
    xy_aht20_t dev;
    int fake_bus;

    for (unsigned i = 0; i < 51U; ++i) {
        queue_status(0x80);
    }

    TEST_ASSERT_EQUAL_INT(XY_AHT20_BUSY, xy_aht20_init(&dev, &fake_bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(500U, g_delay_total);
}

static void test_init_reports_write_and_status_read_failures_and_uncalibrated_status(void)
{
    xy_aht20_t dev;
    int fake_bus;

    g_write_ret_queue[0] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_AHT20_ERROR, xy_aht20_init(&dev, &fake_bus));
    TEST_ASSERT_FALSE(dev.initialized);

    setUp();
    queue_read(NULL, 1U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_aht20_init(&dev, &fake_bus));
    TEST_ASSERT_FALSE(dev.initialized);

    setUp();
    queue_status(0x00); /* init busy check */
    queue_status(0x00); /* calibration status: not calibrated */
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_init(&dev, &fake_bus));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_FALSE(dev.calibrated);
}

static void test_deinit_rejects_null_and_clears_initialized_flag(void)
{
    xy_aht20_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_AHT20_INVALID_PARAM, xy_aht20_deinit(NULL));

    queue_status(0x08);
    queue_status(0x08);
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_init(&dev, &fake_bus));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
}

static void test_read_converts_humidity_temperature_and_timestamp(void)
{
    xy_aht20_t dev;
    int fake_bus;

    queue_status(0x08);
    queue_status(0x08);
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_init(&dev, &fake_bus));

    queue_status(0x00);                  /* pre-trigger idle check */
    queue_status(0x00);                  /* post-trigger idle check */
    queue_measurement(0x80000U, 0x80000U); /* 50.00%RH, 50.00C */

    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_read(&dev));
    TEST_ASSERT_EQUAL_UINT(2U, g_write_count);
    TEST_ASSERT_EQUAL_UINT8(AHT20_CMD_TRIGGER, g_write_queue[1][0]);
    TEST_ASSERT_EQUAL_UINT8(0x33, g_write_queue[1][1]);
    TEST_ASSERT_EQUAL_UINT16(5000U, dev.data.humidity);
    TEST_ASSERT_EQUAL_INT16(5000, dev.data.temperature);
    TEST_ASSERT_EQUAL_UINT32(g_tick, dev.data.timestamp);
}

static void test_read_rejects_invalid_uninitialized_and_propagates_failures_without_overwrite(void)
{
    xy_aht20_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_AHT20_INVALID_PARAM, xy_aht20_read(NULL));
    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_AHT20_INVALID_PARAM, xy_aht20_read(&dev));

    queue_status(0x08);
    queue_status(0x08);
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_init(&dev, &fake_bus));
    dev.data.temperature = 123;
    dev.data.humidity = 456;
    dev.data.timestamp = 0xCAFEU;

    queue_status(0x00);
    g_write_ret_queue[g_write_index] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_aht20_read(&dev));
    TEST_ASSERT_EQUAL_INT16(123, dev.data.temperature);
    TEST_ASSERT_EQUAL_UINT16(456U, dev.data.humidity);
    TEST_ASSERT_EQUAL_UINT32(0xCAFEU, dev.data.timestamp);

    setUp();
    queue_status(0x08);
    queue_status(0x08);
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_init(&dev, &fake_bus));
    dev.data.temperature = 123;
    dev.data.humidity = 456;
    dev.data.timestamp = 0xCAFEU;
    queue_status(0x00);
    queue_status(0x00);
    queue_read(NULL, 7U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_aht20_read(&dev));
    TEST_ASSERT_EQUAL_INT16(123, dev.data.temperature);
    TEST_ASSERT_EQUAL_UINT16(456U, dev.data.humidity);
    TEST_ASSERT_EQUAL_UINT32(0xCAFEU, dev.data.timestamp);
}

static void test_read_post_trigger_busy_preserves_cached_measurement(void)
{
    xy_aht20_t dev;
    int fake_bus;

    queue_status(0x08);
    queue_status(0x08);
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_init(&dev, &fake_bus));
    dev.data.temperature = 321;
    dev.data.humidity = 654U;
    dev.data.timestamp = 0x12345678U;

    queue_status(0x00);
    for (unsigned i = 0; i < 51U; ++i) {
        queue_status(0x80);
    }
    TEST_ASSERT_EQUAL_INT(XY_AHT20_BUSY, xy_aht20_read(&dev));
    TEST_ASSERT_EQUAL_INT16(321, dev.data.temperature);
    TEST_ASSERT_EQUAL_UINT16(654U, dev.data.humidity);
    TEST_ASSERT_EQUAL_UINT32(0x12345678U, dev.data.timestamp);
    TEST_ASSERT_EQUAL_UINT(1U, g_read_count - g_read_index);
}

static void test_get_helpers_validate_inputs_and_update_output_only_on_success(void)
{
    xy_aht20_t dev;
    int16_t temperature = -1;
    uint16_t humidity = 0xBEEF;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_AHT20_INVALID_PARAM, xy_aht20_get_temperature(NULL, &temperature));
    TEST_ASSERT_EQUAL_INT(XY_AHT20_INVALID_PARAM, xy_aht20_get_temperature(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_AHT20_INVALID_PARAM, xy_aht20_get_humidity(NULL, &humidity));
    TEST_ASSERT_EQUAL_INT(XY_AHT20_INVALID_PARAM, xy_aht20_get_humidity(&dev, NULL));

    queue_status(0x08);
    queue_status(0x08);
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_init(&dev, &fake_bus));

    queue_status(0x00);
    queue_status(0x00);
    queue_measurement(0x40000U, 0x40000U); /* 25.00%RH, 0.00C */
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_get_temperature(&dev, &temperature));
    TEST_ASSERT_EQUAL_INT16(0, temperature);

    queue_status(0x00);
    queue_status(0x00);
    queue_measurement(0x60000U, 0x60000U); /* 37.50%RH, 25.00C */
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_get_humidity(&dev, &humidity));
    TEST_ASSERT_EQUAL_UINT16(3750U, humidity);

    for (unsigned i = 0; i < 51U; ++i) {
        queue_status(0x80);
    }
    humidity = 0xBEEF;
    TEST_ASSERT_EQUAL_INT(XY_AHT20_BUSY, xy_aht20_get_humidity(&dev, &humidity));
    TEST_ASSERT_EQUAL_UINT16(0xBEEF, humidity);
}

static void test_init_final_status_read_failure_still_initializes_uncalibrated(void)
{
    xy_aht20_t dev;
    int fake_bus;

    queue_status(0x00);
    queue_read(NULL, 1U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_init(&dev, &fake_bus));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_FALSE(dev.calibrated);
}

static void test_reset_rejects_null_and_sends_reset_command(void)
{
    xy_aht20_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_AHT20_INVALID_PARAM, xy_aht20_reset(NULL));

    queue_status(0x08);
    queue_status(0x08);
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_init(&dev, &fake_bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht20_reset(&dev));
    TEST_ASSERT_EQUAL_UINT(2U, g_write_count);
    TEST_ASSERT_EQUAL_UINT8(AHT20_CMD_RESET, g_write_queue[1][0]);
}

static void test_read_pre_trigger_busy_preserves_cached_measurement_and_no_trigger(void)
{
    xy_aht20_t dev;
    int fake_bus;

    queue_status(0x08);
    queue_status(0x08);
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_init(&dev, &fake_bus));
    dev.data.temperature = -123;
    dev.data.humidity = 456U;
    dev.data.timestamp = 0xABCDEFU;

    for (unsigned i = 0; i < 51U; ++i) {
        queue_status(0x80);
    }
    TEST_ASSERT_EQUAL_INT(XY_AHT20_BUSY, xy_aht20_read(&dev));
    TEST_ASSERT_EQUAL_INT16(-123, dev.data.temperature);
    TEST_ASSERT_EQUAL_UINT16(456U, dev.data.humidity);
    TEST_ASSERT_EQUAL_UINT32(0xABCDEFU, dev.data.timestamp);
    TEST_ASSERT_EQUAL_UINT(1U, g_write_count); /* no trigger write after pre-check timeout */
}

static void test_reset_write_failure_propagates_and_preserves_initialized(void)
{
    xy_aht20_t dev;
    int fake_bus;

    queue_status(0x08);
    queue_status(0x08);
    TEST_ASSERT_EQUAL_INT(XY_AHT20_OK, xy_aht20_init(&dev, &fake_bus));
    g_write_ret_queue[g_write_index] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_aht20_reset(&dev));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(AHT20_CMD_RESET, g_write_queue[1][0]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_invalid_inputs_and_records_calibration_status);
    RUN_TEST(test_init_reports_busy_after_timeout);
    RUN_TEST(test_init_reports_write_and_status_read_failures_and_uncalibrated_status);
    RUN_TEST(test_deinit_rejects_null_and_clears_initialized_flag);
    RUN_TEST(test_read_converts_humidity_temperature_and_timestamp);
    RUN_TEST(test_read_rejects_invalid_uninitialized_and_propagates_failures_without_overwrite);
    RUN_TEST(test_read_post_trigger_busy_preserves_cached_measurement);
    RUN_TEST(test_get_helpers_validate_inputs_and_update_output_only_on_success);
    RUN_TEST(test_init_final_status_read_failure_still_initializes_uncalibrated);
    RUN_TEST(test_reset_rejects_null_and_sends_reset_command);
    RUN_TEST(test_read_pre_trigger_busy_preserves_cached_measurement_and_no_trigger);
    RUN_TEST(test_reset_write_failure_propagates_and_preserves_initialized);
    return UNITY_END();
}
