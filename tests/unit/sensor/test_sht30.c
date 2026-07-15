#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_sht30.h"
#include "xy_device.h"
#include "xy_os_delay.h"

static uint8_t g_read_queue[8][6];
static size_t g_read_len_queue[8];
static int g_read_ret_queue[8];
static size_t g_read_count;
static size_t g_read_index;
static uint8_t g_write_queue[12][2];
static size_t g_write_len_queue[12];
static int g_write_ret_queue[12];
static size_t g_write_count;
static size_t g_write_index;
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

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_write_queue) / sizeof(g_write_queue[0]), g_write_index);
    TEST_ASSERT_EQUAL_UINT(g_write_len_queue[g_write_index], len);
    memcpy(g_write_queue[g_write_index], data, len);
    int ret = g_write_ret_queue[g_write_index];
    g_write_index++;
    return ret;
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

xy_os_status_t xy_os_delay(uint32_t ms)
{
    g_delay_total += ms;
    return 0;
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

static uint8_t sht30_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static void queue_write(size_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_write_queue) / sizeof(g_write_queue[0]), g_write_count);
    g_write_len_queue[g_write_count] = len;
    g_write_ret_queue[g_write_count] = ret;
    g_write_count++;
}

static void queue_measurement(uint16_t temp_raw, uint16_t hum_raw, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_read_queue) / sizeof(g_read_queue[0]), g_read_count);
    uint8_t *buf = g_read_queue[g_read_count];
    buf[0] = (uint8_t)(temp_raw >> 8);
    buf[1] = (uint8_t)temp_raw;
    buf[2] = sht30_crc8(buf, 2U);
    buf[3] = (uint8_t)(hum_raw >> 8);
    buf[4] = (uint8_t)hum_raw;
    buf[5] = sht30_crc8(&buf[3], 2U);
    g_read_len_queue[g_read_count] = 6U;
    g_read_ret_queue[g_read_count] = ret;
    g_read_count++;
}

void setUp(void)
{
    memset(g_read_queue, 0, sizeof(g_read_queue));
    memset(g_read_len_queue, 0, sizeof(g_read_len_queue));
    memset(g_read_ret_queue, 0, sizeof(g_read_ret_queue));
    memset(g_write_queue, 0, sizeof(g_write_queue));
    memset(g_write_len_queue, 0, sizeof(g_write_len_queue));
    memset(g_write_ret_queue, 0, sizeof(g_write_ret_queue));
    g_read_count = 0;
    g_read_index = 0;
    g_write_count = 0;
    g_write_index = 0;
    g_delay_total = 0;
    g_last_addr = 0;
}

void tearDown(void)
{
}

static void test_init_rejects_invalid_inputs_and_sends_soft_reset(void)
{
    xy_sht30_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_SHT30_INVALID_PARAM, xy_sht30_init(NULL, &fake_bus, SHT30_ADDR_DEFAULT));
    TEST_ASSERT_EQUAL_INT(XY_SHT30_INVALID_PARAM, xy_sht30_init(&dev, NULL, SHT30_ADDR_DEFAULT));

    queue_write(1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_init(&dev, &fake_bus, SHT30_ADDR_ALT));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(SHT30_ADDR_ALT, g_last_addr);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(SHT30_CMD_SOFT_RESET >> 8), g_write_queue[0][0]);
    TEST_ASSERT_EQUAL_UINT32(15U, g_delay_total);
}

static void test_init_reports_i2c_reset_failure(void)
{
    xy_sht30_t dev;
    int fake_bus;

    queue_write(1U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_ERROR, xy_sht30_init(&dev, &fake_bus, SHT30_ADDR_DEFAULT));
}

static void test_read_converts_measurement_and_checks_crc(void)
{
    xy_sht30_t dev;
    int fake_bus;

    queue_write(1U, XY_DEVICE_OK); /* init reset */
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_init(&dev, &fake_bus, SHT30_ADDR_DEFAULT));

    queue_write(1U, XY_DEVICE_OK); /* measure */
    queue_measurement(0x8000U, 0x4000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_read(&dev));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(SHT30_CMD_MEASURE_H >> 8), g_write_queue[1][0]);
    TEST_ASSERT_EQUAL_INT16(4250, dev.temperature);
    TEST_ASSERT_EQUAL_UINT16(2500U, dev.humidity);
    TEST_ASSERT_EQUAL_UINT32(35U, g_delay_total);

    queue_write(1U, XY_DEVICE_OK);
    queue_measurement(0x6666U, 0x8000U, XY_DEVICE_OK);
    g_read_queue[1][2] ^= 0x01U;
    TEST_ASSERT_EQUAL_INT(XY_SHT30_CRC_ERROR, xy_sht30_read(&dev));
}

static void test_read_reports_failures_and_preserves_cached_measurement(void)
{
    xy_sht30_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_SHT30_INVALID_PARAM, xy_sht30_read(NULL));
    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_SHT30_INVALID_PARAM, xy_sht30_read(&dev));

    queue_write(1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_init(&dev, &fake_bus, SHT30_ADDR_DEFAULT));
    dev.temperature = 1234;
    dev.humidity = 5678U;

    queue_write(1U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_ERROR, xy_sht30_read(&dev));
    TEST_ASSERT_EQUAL_INT16(1234, dev.temperature);
    TEST_ASSERT_EQUAL_UINT16(5678U, dev.humidity);
    TEST_ASSERT_EQUAL_UINT(g_read_count, g_read_index);

    queue_write(1U, XY_DEVICE_OK);
    queue_measurement(0x0000U, 0x0000U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_ERROR, xy_sht30_read(&dev));
    TEST_ASSERT_EQUAL_INT16(1234, dev.temperature);
    TEST_ASSERT_EQUAL_UINT16(5678U, dev.humidity);

    queue_write(1U, XY_DEVICE_OK);
    queue_measurement(0x6666U, 0x8000U, XY_DEVICE_OK);
    g_read_queue[g_read_count - 1U][5] ^= 0x01U;
    TEST_ASSERT_EQUAL_INT(XY_SHT30_CRC_ERROR, xy_sht30_read(&dev));
    TEST_ASSERT_EQUAL_INT16(1234, dev.temperature);
    TEST_ASSERT_EQUAL_UINT16(5678U, dev.humidity);
}

static void test_read_converts_raw_minimum_and_maximum_bounds(void)
{
    xy_sht30_t dev;
    int fake_bus;

    queue_write(1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_init(&dev, &fake_bus, SHT30_ADDR_DEFAULT));

    queue_write(1U, XY_DEVICE_OK);
    queue_measurement(0x0000U, 0x0000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_read(&dev));
    TEST_ASSERT_EQUAL_INT16(-4500, dev.temperature);
    TEST_ASSERT_EQUAL_UINT16(0U, dev.humidity);

    queue_write(1U, XY_DEVICE_OK);
    queue_measurement(0xFFFFU, 0xFFFFU, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_read(&dev));
    TEST_ASSERT_EQUAL_INT16(13000, dev.temperature);
    TEST_ASSERT_EQUAL_UINT16(10000U, dev.humidity);
}

static void test_helpers_validate_outputs_and_propagate_control_failures(void)
{
    xy_sht30_t dev;
    int16_t temperature = -1;
    uint16_t humidity = 0xBEEFU;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_SHT30_INVALID_PARAM, xy_sht30_read_temperature(NULL, &temperature));
    TEST_ASSERT_EQUAL_INT(XY_SHT30_INVALID_PARAM, xy_sht30_read_temperature(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_SHT30_INVALID_PARAM, xy_sht30_read_humidity(NULL, &humidity));
    TEST_ASSERT_EQUAL_INT(XY_SHT30_INVALID_PARAM, xy_sht30_read_humidity(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_SHT30_INVALID_PARAM, xy_sht30_soft_reset(NULL));
    TEST_ASSERT_EQUAL_INT(XY_SHT30_INVALID_PARAM, xy_sht30_heater_on(NULL));
    TEST_ASSERT_EQUAL_INT(XY_SHT30_INVALID_PARAM, xy_sht30_heater_off(NULL));
    TEST_ASSERT_EQUAL_INT(XY_SHT30_INVALID_PARAM, xy_sht30_deinit(NULL));

    queue_write(1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_init(&dev, &fake_bus, SHT30_ADDR_DEFAULT));

    queue_write(1U, XY_DEVICE_OK);
    queue_measurement(0x8000U, 0x8000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_read_temperature(&dev, &temperature));
    TEST_ASSERT_EQUAL_INT16(4250, temperature);

    queue_write(1U, XY_DEVICE_OK);
    queue_measurement(0x8000U, 0x8000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_read_humidity(&dev, &humidity));
    TEST_ASSERT_EQUAL_UINT16(5000U, humidity);

    temperature = -2222;
    queue_write(1U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_ERROR, xy_sht30_read_temperature(&dev, &temperature));
    TEST_ASSERT_EQUAL_INT16(-2222, temperature);

    humidity = 0xBEEFU;
    queue_write(1U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_ERROR, xy_sht30_read_humidity(&dev, &humidity));
    TEST_ASSERT_EQUAL_UINT16(0xBEEFU, humidity);

    queue_write(1U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_ERROR, xy_sht30_soft_reset(&dev));

    queue_write(1U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_sht30_heater_on(&dev));

    queue_write(1U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_sht30_heater_off(&dev));

    queue_write(1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_soft_reset(&dev));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(SHT30_CMD_SOFT_RESET >> 8), g_write_queue[8][0]);

    queue_write(1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sht30_heater_on(&dev));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(SHT30_CMD_HEATER_ON >> 8), g_write_queue[9][0]);

    queue_write(1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sht30_heater_off(&dev));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(SHT30_CMD_HEATER_OFF >> 8), g_write_queue[10][0]);

    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
}


static void test_sht30_control_success_paths_and_deinit_uninitialized(void)
{
    xy_sht30_t dev;
    int fake_bus;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);

    queue_write(1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_init(&dev, &fake_bus, SHT30_ADDR_DEFAULT));

    g_delay_total = 0;
    queue_write(1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_SHT30_OK, xy_sht30_soft_reset(&dev));
    TEST_ASSERT_EQUAL_UINT32(15U, g_delay_total);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(SHT30_CMD_SOFT_RESET >> 8), g_write_queue[1][0]);

    queue_write(1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sht30_heater_on(&dev));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(SHT30_CMD_HEATER_ON >> 8), g_write_queue[2][0]);

    queue_write(1U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_sht30_heater_off(&dev));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(SHT30_CMD_HEATER_OFF >> 8), g_write_queue[3][0]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_invalid_inputs_and_sends_soft_reset);
    RUN_TEST(test_init_reports_i2c_reset_failure);
    RUN_TEST(test_read_converts_measurement_and_checks_crc);
    RUN_TEST(test_read_reports_failures_and_preserves_cached_measurement);
    RUN_TEST(test_read_converts_raw_minimum_and_maximum_bounds);
    RUN_TEST(test_helpers_validate_outputs_and_propagate_control_failures);
    RUN_TEST(test_sht30_control_success_paths_and_deinit_uninitialized);
    return UNITY_END();
}
