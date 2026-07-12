#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_device.h"
#include "xy_tsl2561.h"

static uint8_t g_read_reg_cmd_queue[32];
static uint8_t g_read_reg_data_queue[32][2];
static size_t g_read_reg_len_queue[32];
static int g_read_reg_ret_queue[32];
static size_t g_read_reg_count;
static size_t g_read_reg_index;

static uint8_t g_write_data_queue[32][2];
static size_t g_write_len_queue[32];
static int g_write_ret_queue[32];
static size_t g_write_count;
static size_t g_write_index;

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

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_read_reg_cmd_queue), g_read_reg_index);
    TEST_ASSERT_EQUAL_UINT8(g_read_reg_cmd_queue[g_read_reg_index], reg);
    TEST_ASSERT_EQUAL_UINT(g_read_reg_len_queue[g_read_reg_index], len);

    int ret = g_read_reg_ret_queue[g_read_reg_index];
    if (ret == XY_DEVICE_OK) {
        memcpy(data, g_read_reg_data_queue[g_read_reg_index], len);
    }
    g_read_reg_index++;
    return ret;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT(2U, len);
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_write_data_queue) / sizeof(g_write_data_queue[0]), g_write_index);

    memcpy(g_write_data_queue[g_write_count], data, len);
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

static void queue_read_reg(uint8_t cmd, const uint8_t *data, size_t len, int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_read_reg_cmd_queue), g_read_reg_count);
    g_read_reg_cmd_queue[g_read_reg_count] = cmd;
    g_read_reg_len_queue[g_read_reg_count] = len;
    g_read_reg_ret_queue[g_read_reg_count] = ret;
    if (data != NULL && len > 0U) {
        memcpy(g_read_reg_data_queue[g_read_reg_count], data, len);
    }
    g_read_reg_count++;
}

static void queue_read_reg_u8(uint8_t cmd, uint8_t value, int ret)
{
    queue_read_reg(cmd, &value, 1U, ret);
}

static void queue_read_reg_u16(uint8_t cmd, uint16_t value, int ret)
{
    uint8_t data[2] = {(uint8_t)value, (uint8_t)(value >> 8)};
    queue_read_reg(cmd, data, 2U, ret);
}

void setUp(void)
{
    memset(g_read_reg_cmd_queue, 0, sizeof(g_read_reg_cmd_queue));
    memset(g_read_reg_data_queue, 0, sizeof(g_read_reg_data_queue));
    memset(g_read_reg_len_queue, 0, sizeof(g_read_reg_len_queue));
    memset(g_read_reg_ret_queue, 0, sizeof(g_read_reg_ret_queue));
    memset(g_write_data_queue, 0, sizeof(g_write_data_queue));
    memset(g_write_len_queue, 0, sizeof(g_write_len_queue));
    for (size_t i = 0; i < sizeof(g_write_ret_queue) / sizeof(g_write_ret_queue[0]); ++i) {
        g_write_ret_queue[i] = XY_DEVICE_OK;
    }
    g_read_reg_count = 0;
    g_read_reg_index = 0;
    g_write_count = 0;
    g_write_index = 0;
    g_last_addr = 0;
    g_tick = 5000;
    g_delay_total = 0;
}

void tearDown(void)
{
}

static void queue_init_success_reads(void)
{
    queue_read_reg_u8(TSL2561_CMD_BIT | TSL2561_REG_ID, 0x0AU, XY_DEVICE_OK);
    queue_read_reg_u8(TSL2561_CMD_BIT | TSL2561_REG_TIMING, 0x12U, XY_DEVICE_OK);
    queue_read_reg_u8(TSL2561_CMD_BIT | TSL2561_REG_TIMING, 0x10U, XY_DEVICE_OK);
}

static void init_ok(xy_tsl2561_t *dev)
{
    int fake_bus;
    queue_init_success_reads();
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_OK, xy_tsl2561_init(dev, &fake_bus, TSL2561_ADDR_FLOAT));
}

static void test_init_rejects_invalid_inputs_and_writes_default_config(void)
{
    xy_tsl2561_t dev;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_init(NULL, &fake_bus, TSL2561_ADDR_FLOAT));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_init(&dev, NULL, TSL2561_ADDR_FLOAT));

    init_ok(&dev);
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(TSL2561_ADDR_FLOAT, g_last_addr);
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_GAIN_1X, dev.gain);
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INTEGRATION_402MS, dev.integration);
    TEST_ASSERT_EQUAL_UINT(3U, g_write_count);
    TEST_ASSERT_EQUAL_UINT8(TSL2561_CMD_BIT | TSL2561_REG_CONTROL, g_write_data_queue[0][0]);
    TEST_ASSERT_EQUAL_UINT8(0x03U, g_write_data_queue[0][1]);
    TEST_ASSERT_EQUAL_UINT8(TSL2561_CMD_BIT | TSL2561_REG_TIMING, g_write_data_queue[1][0]);
    TEST_ASSERT_EQUAL_UINT8(0x02U, g_write_data_queue[1][1]);
    TEST_ASSERT_EQUAL_UINT8(TSL2561_CMD_BIT | TSL2561_REG_TIMING, g_write_data_queue[2][0]);
    TEST_ASSERT_EQUAL_UINT8(0x02U, g_write_data_queue[2][1]);
}

static void test_init_reports_not_found_on_bad_id_or_read_failure(void)
{
    xy_tsl2561_t dev;
    int fake_bus;

    queue_read_reg_u8(TSL2561_CMD_BIT | TSL2561_REG_ID, 0x00U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_NOT_FOUND, xy_tsl2561_init(&dev, &fake_bus, TSL2561_ADDR_LOW));

    setUp();
    queue_read_reg_u8(TSL2561_CMD_BIT | TSL2561_REG_ID, 0x00U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_NOT_FOUND, xy_tsl2561_init(&dev, &fake_bus, TSL2561_ADDR_LOW));
}

static void test_read_updates_channels_lux_and_timestamp(void)
{
    xy_tsl2561_t dev;

    init_ok(&dev);
    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA0_L, 2000U, XY_DEVICE_OK);
    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA1_L, 500U, XY_DEVICE_OK);

    TEST_ASSERT_EQUAL_INT(XY_TSL2561_OK, xy_tsl2561_read(&dev));
    TEST_ASSERT_EQUAL_UINT8(TSL2561_CMD_BIT | TSL2561_REG_CONTROL, g_write_data_queue[3][0]);
    TEST_ASSERT_EQUAL_UINT8(0x03U, g_write_data_queue[3][1]);
    TEST_ASSERT_EQUAL_UINT16(2000U, dev.data.broadband);
    TEST_ASSERT_EQUAL_UINT16(500U, dev.data.ir);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 16.66f, dev.data.lux);
    TEST_ASSERT_EQUAL_UINT32(g_tick, dev.data.timestamp);
    TEST_ASSERT_EQUAL_UINT32(420U, g_delay_total);
}

static void test_read_errors_do_not_overwrite_data(void)
{
    xy_tsl2561_t dev;

    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_read(NULL));
    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_read(&dev));

    init_ok(&dev);
    dev.data.broadband = 11U;
    dev.data.ir = 22U;
    dev.data.lux = 3.0f;
    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA0_L, 0U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_tsl2561_read(&dev));
    TEST_ASSERT_EQUAL_UINT16(11U, dev.data.broadband);
    TEST_ASSERT_EQUAL_UINT16(22U, dev.data.ir);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, dev.data.lux);

    setUp();
    init_ok(&dev);
    dev.data.broadband = 11U;
    dev.data.ir = 22U;
    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA0_L, 2000U, XY_DEVICE_OK);
    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA1_L, 0U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_tsl2561_read(&dev));
    TEST_ASSERT_EQUAL_UINT16(11U, dev.data.broadband);
    TEST_ASSERT_EQUAL_UINT16(22U, dev.data.ir);
}

static void test_getters_update_outputs_only_on_success(void)
{
    xy_tsl2561_t dev;
    uint16_t broadband = 0xFFFFU;
    uint16_t ir = 0xFFFFU;
    float lux = -1.0f;

    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_get_broadband(NULL, &broadband));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_get_broadband(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_get_ir(NULL, &ir));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_get_ir(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_get_lux(NULL, &lux));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_get_lux(&dev, NULL));

    init_ok(&dev);
    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA0_L, 1000U, XY_DEVICE_OK);
    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA1_L, 100U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_OK, xy_tsl2561_get_broadband(&dev, &broadband));
    TEST_ASSERT_EQUAL_UINT16(1000U, broadband);

    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA0_L, 1000U, XY_DEVICE_OK);
    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA1_L, 100U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_OK, xy_tsl2561_get_ir(&dev, &ir));
    TEST_ASSERT_EQUAL_UINT16(100U, ir);

    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA0_L, 1000U, XY_DEVICE_OK);
    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA1_L, 100U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_OK, xy_tsl2561_get_lux(&dev, &lux));
    TEST_ASSERT_GREATER_THAN_FLOAT(0.0f, lux);

    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA0_L, 0U, XY_DEVICE_ERROR);
    broadband = 0xFFFFU;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_tsl2561_get_broadband(&dev, &broadband));
    TEST_ASSERT_EQUAL_UINT16(0xFFFFU, broadband);
}

static void test_read_integration_delay_and_zero_broadband_lux_branches(void)
{
    xy_tsl2561_t dev;

    init_ok(&dev);
    queue_read_reg_u8(TSL2561_CMD_BIT | TSL2561_REG_TIMING, 0x02U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_tsl2561_set_integration(&dev, XY_TSL2561_INTEGRATION_13MS));
    g_delay_total = 0;
    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA0_L, 0U, XY_DEVICE_OK);
    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA1_L, 0U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_OK, xy_tsl2561_read(&dev));
    TEST_ASSERT_EQUAL_UINT32(20U, g_delay_total);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dev.data.lux);

    setUp();
    init_ok(&dev);
    queue_read_reg_u8(TSL2561_CMD_BIT | TSL2561_REG_TIMING, 0x02U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_tsl2561_set_integration(&dev, XY_TSL2561_INTEGRATION_101MS));
    g_delay_total = 0;
    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA0_L, 1500U, XY_DEVICE_OK);
    queue_read_reg_u16(TSL2561_CMD_BIT | TSL2561_WORD_BIT | TSL2561_REG_DATA1_L, 1000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_OK, xy_tsl2561_read(&dev));
    TEST_ASSERT_EQUAL_UINT32(120U, g_delay_total);
}

static void test_enable_disable_propagate_i2c_write_failures(void)
{
    xy_tsl2561_t dev;

    init_ok(&dev);
    g_write_ret_queue[g_write_index] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_tsl2561_enable(&dev));

    g_write_ret_queue[g_write_index] = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_tsl2561_disable(&dev));
}

static void test_gain_integration_enable_disable_and_deinit_contracts(void)
{
    xy_tsl2561_t dev;

    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_set_gain(NULL, XY_TSL2561_GAIN_1X));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_set_integration(NULL, XY_TSL2561_INTEGRATION_13MS));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_enable(NULL));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_disable(NULL));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_deinit(NULL));

    init_ok(&dev);
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM, xy_tsl2561_set_gain(&dev, (xy_tsl2561_gain_t)99));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INVALID_PARAM,
                          xy_tsl2561_set_integration(&dev, (xy_tsl2561_integration_t)99));

    queue_read_reg_u8(TSL2561_CMD_BIT | TSL2561_REG_TIMING, 0x02U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_tsl2561_set_gain(&dev, XY_TSL2561_GAIN_16X));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_GAIN_16X, dev.gain);
    TEST_ASSERT_EQUAL_UINT8(TSL2561_CMD_BIT | TSL2561_REG_TIMING, g_write_data_queue[3][0]);
    TEST_ASSERT_EQUAL_UINT8(0x12U, g_write_data_queue[3][1]);

    queue_read_reg_u8(TSL2561_CMD_BIT | TSL2561_REG_TIMING, 0x12U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_tsl2561_set_integration(&dev, XY_TSL2561_INTEGRATION_13MS));
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_INTEGRATION_13MS, dev.integration);
    TEST_ASSERT_EQUAL_UINT8(0x10U, g_write_data_queue[4][1]);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_tsl2561_disable(&dev));
    TEST_ASSERT_EQUAL_UINT8(0x00U, g_write_data_queue[5][1]);
    TEST_ASSERT_EQUAL_INT(XY_TSL2561_OK, xy_tsl2561_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(0x00U, g_write_data_queue[6][1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_invalid_inputs_and_writes_default_config);
    RUN_TEST(test_init_reports_not_found_on_bad_id_or_read_failure);
    RUN_TEST(test_read_updates_channels_lux_and_timestamp);
    RUN_TEST(test_read_errors_do_not_overwrite_data);
    RUN_TEST(test_getters_update_outputs_only_on_success);
    RUN_TEST(test_read_integration_delay_and_zero_broadband_lux_branches);
    RUN_TEST(test_enable_disable_propagate_i2c_write_failures);
    RUN_TEST(test_gain_integration_enable_disable_and_deinit_contracts);
    return UNITY_END();
}
