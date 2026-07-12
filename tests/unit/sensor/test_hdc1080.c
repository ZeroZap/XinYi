#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_device.h"
#include "xy_hdc1080.h"

static uint8_t g_read_queue[16][4];
static size_t g_read_len_queue[16];
static int g_read_ret_queue[16];
static size_t g_read_count;
static size_t g_read_index;

static uint8_t g_write_reg_queue[16];
static uint8_t g_write_data_queue[16][2];
static size_t g_write_len_queue[16];
static int g_write_ret_queue[16];
static size_t g_write_count;
static size_t g_write_index;

static uint8_t g_cmd_queue[16];
static int g_cmd_ret_queue[16];
static size_t g_cmd_count;
static size_t g_cmd_index;

static uint8_t g_last_addr;
static uint32_t g_tick;
static uint32_t g_delay_total;

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

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT(2U, len);
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_write_reg_queue), g_write_index);

    g_write_reg_queue[g_write_count] = reg;
    memcpy(g_write_data_queue[g_write_count], data, len);
    g_write_len_queue[g_write_count] = len;
    g_write_count++;
    return g_write_ret_queue[g_write_index++];
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT(1U, len);
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_cmd_queue), g_cmd_index);

    g_cmd_queue[g_cmd_count++] = data[0];
    return g_cmd_ret_queue[g_cmd_index++];
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

static void queue_read_raw(uint16_t temp_raw, uint16_t humi_raw, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_read_queue) / sizeof(g_read_queue[0]), g_read_count);
    g_read_queue[g_read_count][0] = (uint8_t)(temp_raw >> 8);
    g_read_queue[g_read_count][1] = (uint8_t)temp_raw;
    g_read_queue[g_read_count][2] = (uint8_t)(humi_raw >> 8);
    g_read_queue[g_read_count][3] = (uint8_t)humi_raw;
    g_read_len_queue[g_read_count] = 4U;
    g_read_ret_queue[g_read_count] = ret;
    g_read_count++;
}

void setUp(void)
{
    memset(g_read_queue, 0, sizeof(g_read_queue));
    memset(g_read_len_queue, 0, sizeof(g_read_len_queue));
    memset(g_read_ret_queue, 0, sizeof(g_read_ret_queue));
    memset(g_write_reg_queue, 0, sizeof(g_write_reg_queue));
    memset(g_write_data_queue, 0, sizeof(g_write_data_queue));
    memset(g_write_len_queue, 0, sizeof(g_write_len_queue));
    memset(g_cmd_queue, 0, sizeof(g_cmd_queue));
    for (size_t i = 0; i < sizeof(g_write_ret_queue) / sizeof(g_write_ret_queue[0]); ++i) {
        g_write_ret_queue[i] = XY_DEVICE_OK;
    }
    for (size_t i = 0; i < sizeof(g_cmd_ret_queue) / sizeof(g_cmd_ret_queue[0]); ++i) {
        g_cmd_ret_queue[i] = XY_DEVICE_OK;
    }
    g_read_count = 0;
    g_read_index = 0;
    g_write_count = 0;
    g_write_index = 0;
    g_cmd_count = 0;
    g_cmd_index = 0;
    g_last_addr = 0;
    g_tick = 1000;
    g_delay_total = 0;
}

void tearDown(void)
{
}

static void init_ok(xy_hdc1080_t *dev)
{
    int fake_bus;
    TEST_ASSERT_EQUAL_INT(XY_HDC1080_OK, xy_hdc1080_init(dev, &fake_bus, HDC1080_ADDR));
}

static void test_init_rejects_invalid_inputs_and_writes_reset_then_config(void)
{
    xy_hdc1080_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_HDC1080_INVALID_PARAM, xy_hdc1080_init(NULL, &fake_bus, HDC1080_ADDR));
    TEST_ASSERT_EQUAL_INT(XY_HDC1080_INVALID_PARAM, xy_hdc1080_init(&dev, NULL, HDC1080_ADDR));

    TEST_ASSERT_EQUAL_INT(XY_HDC1080_OK, xy_hdc1080_init(&dev, &fake_bus, HDC1080_ADDR));
    TEST_ASSERT_EQUAL_UINT8(HDC1080_ADDR, g_last_addr);
    TEST_ASSERT_EQUAL_UINT8(1U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT(2U, g_write_count);
    TEST_ASSERT_EQUAL_UINT8(HDC1080_REG_CONFIG, g_write_reg_queue[0]);
    TEST_ASSERT_EQUAL_UINT8(0x80, g_write_data_queue[0][0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_write_data_queue[0][1]);
    TEST_ASSERT_EQUAL_UINT8(HDC1080_REG_CONFIG, g_write_reg_queue[1]);
    TEST_ASSERT_EQUAL_UINT8(0x10, g_write_data_queue[1][0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_write_data_queue[1][1]);
    TEST_ASSERT_EQUAL_UINT32(15U, g_delay_total);
}

static void test_init_maps_write_failures_to_error(void)
{
    xy_hdc1080_t dev;
    int fake_bus;

    g_write_ret_queue[0] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_HDC1080_ERROR, xy_hdc1080_init(&dev, &fake_bus, HDC1080_ADDR));

    setUp();
    g_write_ret_queue[1] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_HDC1080_ERROR, xy_hdc1080_init(&dev, &fake_bus, HDC1080_ADDR));
}

static void test_read_converts_temperature_and_humidity(void)
{
    xy_hdc1080_t dev;

    init_ok(&dev);
    queue_read_raw(0x8000U, 0x4000U, XY_DEVICE_OK);

    TEST_ASSERT_EQUAL_INT(XY_HDC1080_OK, xy_hdc1080_read(&dev));
    TEST_ASSERT_EQUAL_UINT(1U, g_cmd_count);
    TEST_ASSERT_EQUAL_UINT8(HDC1080_REG_TEMP, g_cmd_queue[0]);
    TEST_ASSERT_INT16_WITHIN(1, 4250, dev.temperature);
    TEST_ASSERT_UINT16_WITHIN(1, 2500U, dev.humidity);
    TEST_ASSERT_EQUAL_UINT32(25U, g_delay_total);
}

static void test_read_returns_errors_without_overwriting_existing_values(void)
{
    xy_hdc1080_t dev;

    TEST_ASSERT_EQUAL_INT(XY_HDC1080_INVALID_PARAM, xy_hdc1080_read(NULL));
    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_HDC1080_INVALID_PARAM, xy_hdc1080_read(&dev));

    init_ok(&dev);
    dev.temperature = 123;
    dev.humidity = 456;
    g_cmd_ret_queue[0] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_hdc1080_read(&dev));
    TEST_ASSERT_EQUAL_INT16(123, dev.temperature);
    TEST_ASSERT_EQUAL_UINT16(456U, dev.humidity);

    setUp();
    init_ok(&dev);
    dev.temperature = 123;
    dev.humidity = 456;
    queue_read_raw(0U, 0U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_hdc1080_read(&dev));
    TEST_ASSERT_EQUAL_INT16(123, dev.temperature);
    TEST_ASSERT_EQUAL_UINT16(456U, dev.humidity);
}

static void test_read_temperature_and_humidity_update_outputs_only_on_success(void)
{
    xy_hdc1080_t dev;
    int16_t temp = -1;
    uint16_t humi = 0xFFFFU;

    TEST_ASSERT_EQUAL_INT(XY_HDC1080_INVALID_PARAM, xy_hdc1080_read_temperature(NULL, &temp));
    TEST_ASSERT_EQUAL_INT(XY_HDC1080_INVALID_PARAM, xy_hdc1080_read_temperature(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_HDC1080_INVALID_PARAM, xy_hdc1080_read_humidity(NULL, &humi));
    TEST_ASSERT_EQUAL_INT(XY_HDC1080_INVALID_PARAM, xy_hdc1080_read_humidity(&dev, NULL));

    init_ok(&dev);
    queue_read_raw(0x8000U, 0x8000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_HDC1080_OK, xy_hdc1080_read_temperature(&dev, &temp));
    TEST_ASSERT_INT16_WITHIN(1, 4250, temp);

    queue_read_raw(0x0000U, 0x4000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_HDC1080_OK, xy_hdc1080_read_humidity(&dev, &humi));
    TEST_ASSERT_UINT16_WITHIN(1, 2500U, humi);

    g_cmd_ret_queue[g_cmd_index] = XY_DEVICE_ERROR;
    temp = -1;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_hdc1080_read_temperature(&dev, &temp));
    TEST_ASSERT_EQUAL_INT16(-1, temp);
}

static void test_deinit_and_heater_commands(void)
{
    xy_hdc1080_t dev;

    TEST_ASSERT_EQUAL_INT(XY_HDC1080_INVALID_PARAM, xy_hdc1080_deinit(NULL));

    init_ok(&dev);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_hdc1080_heater_on(&dev));
    TEST_ASSERT_EQUAL_UINT8(HDC1080_REG_CONFIG, g_write_reg_queue[2]);
    TEST_ASSERT_EQUAL_UINT8(0x20, g_write_data_queue[2][0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_write_data_queue[2][1]);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_hdc1080_heater_off(&dev));
    TEST_ASSERT_EQUAL_UINT8(HDC1080_REG_CONFIG, g_write_reg_queue[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_write_data_queue[3][0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_write_data_queue[3][1]);

    TEST_ASSERT_EQUAL_INT(XY_HDC1080_OK, xy_hdc1080_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_invalid_inputs_and_writes_reset_then_config);
    RUN_TEST(test_init_maps_write_failures_to_error);
    RUN_TEST(test_read_converts_temperature_and_humidity);
    RUN_TEST(test_read_returns_errors_without_overwriting_existing_values);
    RUN_TEST(test_read_temperature_and_humidity_update_outputs_only_on_success);
    RUN_TEST(test_deinit_and_heater_commands);
    return UNITY_END();
}
