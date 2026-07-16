#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_bh1750.h"
#include "xy_device.h"

static uint8_t g_read_queue[16][2];
static size_t g_read_len_queue[16];
static int g_read_ret_queue[16];
static size_t g_read_count;
static size_t g_read_index;
static uint8_t g_write_queue[16];
static int g_write_ret_queue[16];
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
    TEST_ASSERT_EQUAL_UINT(1U, len);
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_write_queue), g_write_index);

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

static void queue_read_raw(uint16_t raw, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_read_queue) / sizeof(g_read_queue[0]), g_read_count);
    g_read_queue[g_read_count][0] = (uint8_t)(raw >> 8);
    g_read_queue[g_read_count][1] = (uint8_t)raw;
    g_read_len_queue[g_read_count] = 2U;
    g_read_ret_queue[g_read_count] = ret;
    g_read_count++;
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
    g_tick = 3000;
    g_delay_total = 0;
    g_last_addr = 0;
}

void tearDown(void)
{
}

static void init_ok(xy_bh1750_t *dev)
{
    int fake_bus;
    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_init(dev, &fake_bus, BH1750_ADDR_LOW));
}

static void test_init_rejects_invalid_inputs_and_sends_power_on_reset(void)
{
    xy_bh1750_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_BH1750_INVALID_PARAM, xy_bh1750_init(NULL, &fake_bus, BH1750_ADDR_LOW));
    TEST_ASSERT_EQUAL_INT(XY_BH1750_INVALID_PARAM, xy_bh1750_init(&dev, NULL, BH1750_ADDR_LOW));

    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_init(&dev, &fake_bus, BH1750_ADDR_HIGH));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(BH1750_ADDR_HIGH, g_last_addr);
    TEST_ASSERT_EQUAL_INT(XY_BH1750_HIGH_RES, dev.resolution);
    TEST_ASSERT_EQUAL_INT(XY_BH1750_ONE_TIME, dev.mode);
    TEST_ASSERT_EQUAL_UINT(2U, g_write_count);
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_POWER_ON, g_write_queue[0]);
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_RESET, g_write_queue[1]);
    TEST_ASSERT_EQUAL_UINT32(20U, g_delay_total);
}

static void test_init_reports_not_found_when_power_on_fails(void)
{
    xy_bh1750_t dev;
    int fake_bus;

    g_write_ret_queue[0] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_BH1750_NOT_FOUND, xy_bh1750_init(&dev, &fake_bus, BH1750_ADDR_LOW));
    TEST_ASSERT_FALSE(dev.initialized);
}

static void test_read_high_resolution_one_time_converts_raw_lux(void)
{
    xy_bh1750_t dev;

    init_ok(&dev);
    queue_read_raw(1234U, XY_DEVICE_OK);

    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_read(&dev));
    TEST_ASSERT_EQUAL_UINT(4U, g_write_count);
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_POWER_ON, g_write_queue[2]);
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_ONCE_H, g_write_queue[3]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1234.0f, dev.data.illuminance);
    TEST_ASSERT_EQUAL_UINT32(g_tick, dev.data.timestamp);
    TEST_ASSERT_EQUAL_UINT32(210U, g_delay_total);
}

static void test_read_resolution_and_mode_select_command_and_scale(void)
{
    xy_bh1750_t dev;

    init_ok(&dev);
    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_set_resolution(&dev, XY_BH1750_HIGH_RES2));
    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_set_mode(&dev, XY_BH1750_CONTINUOUS));
    queue_read_raw(100U, XY_DEVICE_OK);

    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_read(&dev));
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_CONT_H2, g_write_queue[3]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, dev.data.illuminance);

    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_set_resolution(&dev, XY_BH1750_LOW_RES));
    queue_read_raw(80U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_read(&dev));
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_CONT_L, g_write_queue[5]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, dev.data.illuminance);
}

static void test_read_failures_preserve_cached_data_and_stop_early(void)
{
    xy_bh1750_t dev;

    init_ok(&dev);
    dev.data.illuminance = 12.5f;
    dev.data.timestamp = 0x1234U;
    g_write_ret_queue[g_write_index] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bh1750_read(&dev));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.5f, dev.data.illuminance);
    TEST_ASSERT_EQUAL_UINT32(0x1234U, dev.data.timestamp);
    TEST_ASSERT_EQUAL_UINT(3U, g_write_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_index);

    setUp();
    init_ok(&dev);
    dev.data.illuminance = 12.5f;
    dev.data.timestamp = 0x1234U;
    g_write_ret_queue[g_write_index + 1U] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bh1750_read(&dev));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.5f, dev.data.illuminance);
    TEST_ASSERT_EQUAL_UINT32(0x1234U, dev.data.timestamp);
    TEST_ASSERT_EQUAL_UINT(4U, g_write_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_index);
}

static void test_read_data_failure_preserves_cache_after_measurement_wait(void)
{
    xy_bh1750_t dev;

    init_ok(&dev);
    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_set_resolution(&dev, XY_BH1750_LOW_RES));
    dev.data.illuminance = 88.0f;
    dev.data.timestamp = 0xBEEFU;
    queue_read_raw(0U, XY_DEVICE_ERROR);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bh1750_read(&dev));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 88.0f, dev.data.illuminance);
    TEST_ASSERT_EQUAL_UINT32(0xBEEFU, dev.data.timestamp);
    TEST_ASSERT_EQUAL_UINT(1U, g_read_index);
    TEST_ASSERT_EQUAL_UINT32(46U, g_delay_total);
}

static void test_get_illuminance_validates_inputs_and_preserves_output_on_failure(void)
{
    xy_bh1750_t dev;
    float lux = -1.0f;

    TEST_ASSERT_EQUAL_INT(XY_BH1750_INVALID_PARAM, xy_bh1750_get_illuminance(NULL, &lux));
    TEST_ASSERT_EQUAL_INT(XY_BH1750_INVALID_PARAM, xy_bh1750_get_illuminance(&dev, NULL));

    init_ok(&dev);
    queue_read_raw(64U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_get_illuminance(&dev, &lux));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 64.0f, lux);

    queue_read_raw(0U, XY_DEVICE_ERROR);
    lux = -1.0f;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bh1750_get_illuminance(&dev, &lux));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, lux);
}

static void test_init_reset_failure_leaves_device_uninitialized(void)
{
    xy_bh1750_t dev;
    int fake_bus;

    g_write_ret_queue[1] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bh1750_init(&dev, &fake_bus, BH1750_ADDR_LOW));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT(2U, g_write_count);
}

static void test_power_and_reset_propagate_write_failures(void)
{
    xy_bh1750_t dev;

    init_ok(&dev);
    g_write_ret_queue[g_write_index] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bh1750_power_down(&dev));

    g_write_ret_queue[g_write_index] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bh1750_power_on(&dev));

    g_write_ret_queue[g_write_index] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_bh1750_reset(&dev));
}

static void test_configuration_power_and_reset_validate_inputs(void)
{
    xy_bh1750_t dev;

    TEST_ASSERT_EQUAL_INT(XY_BH1750_INVALID_PARAM, xy_bh1750_deinit(NULL));
    TEST_ASSERT_EQUAL_INT(XY_BH1750_INVALID_PARAM, xy_bh1750_power_on(NULL));
    TEST_ASSERT_EQUAL_INT(XY_BH1750_INVALID_PARAM, xy_bh1750_power_down(NULL));
    TEST_ASSERT_EQUAL_INT(XY_BH1750_INVALID_PARAM, xy_bh1750_reset(NULL));
    TEST_ASSERT_EQUAL_INT(XY_BH1750_INVALID_PARAM,
                          xy_bh1750_set_resolution(&dev, (xy_bh1750_res_t)99));
    TEST_ASSERT_EQUAL_INT(XY_BH1750_INVALID_PARAM, xy_bh1750_set_mode(&dev, (xy_bh1750_mode_t)99));

    init_ok(&dev);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bh1750_power_down(&dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bh1750_power_on(&dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_bh1750_reset(&dev));
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_POWER_DOWN, g_write_queue[2]);
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_POWER_ON, g_write_queue[3]);
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_RESET, g_write_queue[4]);

    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_POWER_DOWN, g_write_queue[5]);
}

static void test_deinit_clears_initialized_even_when_power_down_fails(void)
{
    xy_bh1750_t dev;

    init_ok(&dev);
    g_write_ret_queue[g_write_index] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_POWER_DOWN, g_write_queue[2]);
}

static void test_setters_update_cached_mode_and_resolution_without_bus_io(void)
{
    xy_bh1750_t dev;

    init_ok(&dev);
    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_set_resolution(&dev, XY_BH1750_LOW_RES));
    TEST_ASSERT_EQUAL_INT(XY_BH1750_LOW_RES, dev.resolution);
    TEST_ASSERT_EQUAL_UINT(2U, g_write_count);

    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_set_mode(&dev, XY_BH1750_CONTINUOUS));
    TEST_ASSERT_EQUAL_INT(XY_BH1750_CONTINUOUS, dev.mode);
    TEST_ASSERT_EQUAL_UINT(2U, g_write_count);
}

static void test_read_default_fallback_uses_continuous_high_command_and_delay(void)
{
    xy_bh1750_t dev;

    init_ok(&dev);
    dev.resolution = (xy_bh1750_res_t)99;
    dev.mode = XY_BH1750_ONE_TIME;
    queue_read_raw(42U, XY_DEVICE_OK);

    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_read(&dev));
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_CONT_H, g_write_queue[3]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 42.0f, dev.data.illuminance);
    TEST_ASSERT_EQUAL_UINT32(210U, g_delay_total);
}

static void test_read_one_time_high2_and_low_resolution_boundaries(void)
{
    xy_bh1750_t dev;

    init_ok(&dev);
    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_set_resolution(&dev, XY_BH1750_HIGH_RES2));
    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_set_mode(&dev, XY_BH1750_ONE_TIME));
    queue_read_raw(0xFFFFU, XY_DEVICE_OK);

    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_read(&dev));
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_ONCE_H2, g_write_queue[3]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 32767.5f, dev.data.illuminance);
    TEST_ASSERT_EQUAL_UINT32(210U, g_delay_total);

    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_set_resolution(&dev, XY_BH1750_LOW_RES));
    queue_read_raw(0xFFFFU, XY_DEVICE_OK);

    TEST_ASSERT_EQUAL_INT(XY_BH1750_OK, xy_bh1750_read(&dev));
    TEST_ASSERT_EQUAL_UINT8(BH1750_CMD_ONCE_L, g_write_queue[5]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 8191.875f, dev.data.illuminance);
    TEST_ASSERT_EQUAL_UINT32(236U, g_delay_total);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_invalid_inputs_and_sends_power_on_reset);
    RUN_TEST(test_init_reports_not_found_when_power_on_fails);
    RUN_TEST(test_read_high_resolution_one_time_converts_raw_lux);
    RUN_TEST(test_read_resolution_and_mode_select_command_and_scale);
    RUN_TEST(test_read_failures_preserve_cached_data_and_stop_early);
    RUN_TEST(test_read_data_failure_preserves_cache_after_measurement_wait);
    RUN_TEST(test_get_illuminance_validates_inputs_and_preserves_output_on_failure);
    RUN_TEST(test_init_reset_failure_leaves_device_uninitialized);
    RUN_TEST(test_power_and_reset_propagate_write_failures);
    RUN_TEST(test_configuration_power_and_reset_validate_inputs);
    RUN_TEST(test_deinit_clears_initialized_even_when_power_down_fails);
    RUN_TEST(test_setters_update_cached_mode_and_resolution_without_bus_io);
    RUN_TEST(test_read_default_fallback_uses_continuous_high_command_and_delay);
    RUN_TEST(test_read_one_time_high2_and_low_resolution_boundaries);
    return UNITY_END();
}
