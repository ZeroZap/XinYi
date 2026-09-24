#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_device.h"
#include "xy_hal_sys.h"
#include "xy_ltc2945.h"
#include "xy_ads1115.h"
#include "xy_ina219.h"
#include "xy_os.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef enum {
    OP_READ_REG,
    OP_WRITE,
    OP_WRITE_REG,
} op_kind_t;

typedef struct {
    op_kind_t kind;
    uint8_t reg;
    uint8_t data[3];
    size_t len;
    xy_error_t ret;
} i2c_op_t;

static i2c_op_t g_ops[96];
static uint8_t g_seen_write_data[96][3];
static size_t g_op_count;
static size_t g_op_index;
static size_t g_seen_write_count;
static uint16_t g_last_addr;
static uint32_t g_last_timeout;
static uint32_t g_delay_total;
static xy_error_t g_i2c_init_ret;

static void queue_read(uint8_t reg, const uint8_t *data, size_t len, xy_error_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    g_ops[g_op_count].kind = OP_READ_REG;
    g_ops[g_op_count].reg = reg;
    g_ops[g_op_count].len = len;
    g_ops[g_op_count].ret = ret;
    if (data != NULL) {
        memcpy(g_ops[g_op_count].data, data, len);
    }
    g_op_count++;
}

static void queue_read8(uint8_t reg, uint8_t value, xy_error_t ret)
{
    queue_read(reg, &value, 1U, ret);
}

static void queue_read16(uint8_t reg, uint16_t value, xy_error_t ret)
{
    uint8_t data[2] = {(uint8_t)(value >> 8), (uint8_t)value};
    queue_read(reg, data, 2U, ret);
}

static void queue_read24(uint8_t reg, uint32_t value, xy_error_t ret)
{
    uint8_t data[3] = {(uint8_t)(value >> 16), (uint8_t)(value >> 8), (uint8_t)value};
    queue_read(reg, data, 3U, ret);
}

static void queue_write_reg_config(uint16_t config, xy_error_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    g_ops[g_op_count].kind = OP_WRITE_REG;
    g_ops[g_op_count].reg = ADS1115_REG_CONFIG;
    g_ops[g_op_count].data[0] = ADS1115_REG_CONFIG;
    g_ops[g_op_count].data[1] = (uint8_t)(config >> 8);
    g_ops[g_op_count].data[2] = (uint8_t)config;
    g_ops[g_op_count].len = 3U;
    g_ops[g_op_count].ret = ret;
    g_op_count++;
}

static void queue_write_u8(uint8_t reg, uint8_t value, xy_error_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    g_ops[g_op_count].kind = OP_WRITE_REG;
    g_ops[g_op_count].reg = reg;
    g_ops[g_op_count].data[0] = value;
    g_ops[g_op_count].len = 1U;
    g_ops[g_op_count].ret = ret;
    g_op_count++;
}

static void queue_write_reg16(uint8_t reg, uint16_t value, xy_error_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    g_ops[g_op_count].kind = OP_WRITE_REG;
    g_ops[g_op_count].reg = reg;
    g_ops[g_op_count].data[0] = (uint8_t)(value >> 8);
    g_ops[g_op_count].data[1] = (uint8_t)value;
    g_ops[g_op_count].len = 2U;
    g_ops[g_op_count].ret = ret;
    g_op_count++;
}

static i2c_op_t *next_op(op_kind_t kind)
{
    TEST_ASSERT_LESS_THAN_UINT(g_op_count, g_op_index);
    TEST_ASSERT_EQUAL_INT(kind, g_ops[g_op_index].kind);
    return &g_ops[g_op_index++];
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
    g_last_addr = addr;
    g_last_timeout = timeout;
    if (g_i2c_init_ret != XY_DEVICE_OK) {
        return g_i2c_init_ret;
    }
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_EQUAL(0, dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    TEST_ASSERT_NOT_NULL(data);

    i2c_op_t *op = next_op(OP_READ_REG);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    if (op->ret == XY_DEVICE_OK) {
        memcpy(data, op->data, len);
    }
    return op->ret;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_EQUAL(0, dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    TEST_ASSERT_NOT_NULL(data);

    i2c_op_t *op = next_op(OP_WRITE);
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    memcpy(g_seen_write_data[g_seen_write_count++], data, len);
    return op->ret;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_EQUAL(0, dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    TEST_ASSERT_NOT_NULL(data);

    i2c_op_t *op = next_op(OP_WRITE_REG);
    TEST_ASSERT_EQUAL_UINT8(op->reg, reg);
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(op->data, data, len);
    memcpy(g_seen_write_data[g_seen_write_count++], data, len);
    return op->ret;
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

uint32_t xy_hal_sys_get_tick_count(void)
{
    return 222333U;
}

xy_os_status_t xy_os_delay(uint32_t ticks)
{
    g_delay_total += ticks;
    return XY_OS_OK;
}

void setUp(void)
{
    memset(g_ops, 0, sizeof(g_ops));
    memset(g_seen_write_data, 0, sizeof(g_seen_write_data));
    g_op_count = 0;
    g_op_index = 0;
    g_seen_write_count = 0;
    g_last_addr = 0;
    g_last_timeout = 0;
    g_delay_total = 0;
    g_i2c_init_ret = XY_DEVICE_OK;
}

void tearDown(void)
{
}

static xy_ltc2945_config_t ltc_config(void)
{
    xy_ltc2945_config_t cfg = {
        .shunt_resistance_uohm = 10000U,
        .control_register = XY_LTC2945_CONTROL_CONTINUOUS_SENSE_PLUS,
        .alert_register = 0xA5U,
    };
    return cfg;
}

static void init_ltc_ok(xy_ltc2945_t *ltc, int *bus)
{
    xy_ltc2945_config_t cfg = ltc_config();
    queue_read8(XY_LTC2945_REG_STATUS, 0x55U, XY_DEVICE_OK);
    queue_write_u8(XY_LTC2945_REG_CONTROL, cfg.control_register, XY_DEVICE_OK);
    queue_write_u8(XY_LTC2945_REG_ALERT, cfg.alert_register, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_OK,
                          xy_ltc2945_init(ltc, bus, XY_LTC2945_ADDR_DEFAULT, &cfg));
}

static void test_ltc2945_datasheet_registers_scaling_and_lifecycle(void)
{
    xy_ltc2945_t ltc;
    xy_ltc2945_sample_t sample = {0};
    xy_ltc2945_config_t cfg = ltc_config();
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM,
                          xy_ltc2945_init(NULL, &bus, XY_LTC2945_ADDR_DEFAULT, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM,
                          xy_ltc2945_init(&ltc, NULL, XY_LTC2945_ADDR_DEFAULT, &cfg));
    cfg.shunt_resistance_uohm = 0U;
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM,
                          xy_ltc2945_init(&ltc, &bus, XY_LTC2945_ADDR_DEFAULT, &cfg));
    cfg = ltc_config();
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM,
                          xy_ltc2945_init(&ltc, &bus, 0x66U, &cfg));

    init_ltc_ok(&ltc, &bus);
    TEST_ASSERT_TRUE(ltc.initialized);
    TEST_ASSERT_EQUAL_UINT16(XY_LTC2945_ADDR_DEFAULT, g_last_addr);
    TEST_ASSERT_EQUAL_UINT32(1000U, g_last_timeout);

    queue_read16(XY_LTC2945_REG_VIN_MSB, 0x1000U, XY_DEVICE_OK);
    queue_read16(XY_LTC2945_REG_SENSE_MSB, 0x0100U, XY_DEVICE_OK);
    queue_read24(XY_LTC2945_REG_POWER_MSB, 0x000064U, XY_DEVICE_OK);
    queue_read8(XY_LTC2945_REG_STATUS, 0x12U, XY_DEVICE_OK);
    queue_read8(XY_LTC2945_REG_FAULT, 0x34U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_OK, xy_ltc2945_read_sample(&ltc, &sample));
    TEST_ASSERT_EQUAL_UINT32(6400U, sample.bus_voltage_mv);
    TEST_ASSERT_EQUAL_UINT32(400U, sample.shunt_voltage_uv);
    TEST_ASSERT_EQUAL_UINT32(40000U, sample.current_ua);
    TEST_ASSERT_EQUAL_UINT32(6250U, sample.power_uw);
    TEST_ASSERT_EQUAL_UINT8(0x12U, sample.status);
    TEST_ASSERT_EQUAL_UINT8(0x34U, sample.fault);
    TEST_ASSERT_EQUAL_UINT32(222333U, sample.timestamp);

    queue_write_u8(XY_LTC2945_REG_ALERT, 0x5AU, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ltc2945_set_alert_mask(&ltc, 0x5AU));
    TEST_ASSERT_EQUAL_UINT8(0x5AU, ltc.config.alert_register);
    queue_write_u8(XY_LTC2945_REG_FAULT, 0x34U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ltc2945_clear_faults(&ltc, 0x34U));

    TEST_ASSERT_EQUAL_INT(XY_LTC2945_OK, xy_ltc2945_deinit(&ltc));
    TEST_ASSERT_FALSE(ltc.initialized);
    TEST_ASSERT_FALSE(ltc.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(ltc.i2c_dev.i2c_handle);
}

static void test_ltc2945_failures_are_atomic_and_stop_at_first_error(void)
{
    xy_ltc2945_t ltc;
    xy_ltc2945_sample_t output = {.bus_voltage_mv = 1U, .current_ua = 2U};
    const xy_ltc2945_sample_t output_before = output;
    int bus;

    init_ltc_ok(&ltc, &bus);
    ltc.sample.bus_voltage_mv = 11U;
    ltc.sample.current_ua = 22U;
    const xy_ltc2945_sample_t cache_before = ltc.sample;

    queue_read16(XY_LTC2945_REG_VIN_MSB, 0x1000U, XY_DEVICE_OK);
    queue_read16(XY_LTC2945_REG_SENSE_MSB, 0U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_ltc2945_read_sample(&ltc, &output));
    TEST_ASSERT_EQUAL_MEMORY(&output_before, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&cache_before, &ltc.sample, sizeof(ltc.sample));
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);

    ltc.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_read_sample(&ltc, &output));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_set_alert_mask(&ltc, 1U));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_clear_faults(&ltc, 1U));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_deinit(&ltc));
    TEST_ASSERT_TRUE(ltc.initialized);
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);
}

static void test_ltc2945_init_failures_clear_complete_lifecycle(void)
{
    xy_ltc2945_t ltc;
    xy_ltc2945_config_t cfg = ltc_config();
    int bus;

    memset(&ltc, 0xA5, sizeof(ltc));
    g_i2c_init_ret = XY_DEVICE_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR,
                          xy_ltc2945_init(&ltc, &bus, XY_LTC2945_ADDR_DEFAULT, &cfg));
    TEST_ASSERT_EQUAL_MEMORY(&(xy_ltc2945_t){0}, &ltc, sizeof(ltc));
    TEST_ASSERT_EQUAL_UINT(0U, g_op_index);

    setUp();
    queue_read8(XY_LTC2945_REG_STATUS, 0U, XY_DEVICE_OK);
    queue_write_u8(XY_LTC2945_REG_CONTROL, cfg.control_register, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_ltc2945_init(&ltc, &bus, XY_LTC2945_ADDR_DEFAULT, &cfg));
    TEST_ASSERT_EQUAL_MEMORY(&(xy_ltc2945_t){0}, &ltc, sizeof(ltc));
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);
}

static void test_ltc2945_missing_handle_fails_closed(void)
{
    xy_ltc2945_t ltc;
    xy_ltc2945_sample_t output = {.bus_voltage_mv = 1U, .current_ua = 2U};
    const xy_ltc2945_sample_t snapshot = output;
    size_t before;
    int bus;

    init_ltc_ok(&ltc, &bus);
    ltc.sample = snapshot;
    before = g_op_index;
    ltc.i2c_dev.i2c_handle = NULL;

    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_read_sample(&ltc, &output));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_set_alert_mask(&ltc, 1U));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_clear_faults(&ltc, 1U));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_deinit(&ltc));
    TEST_ASSERT_EQUAL_UINT(before, g_op_index);
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &ltc.sample, sizeof(ltc.sample));
    TEST_ASSERT_TRUE(ltc.initialized);
}

static void init_ads_ok(xy_ads1115_t *ads, int *bus)
{
    queue_read16(ADS1115_REG_CONFIG, 0x8583U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_init(ads, bus, ADS1115_ADDR_GND));
}

static void test_ads1115_single_diff_voltage_config_and_invalid_paths(void)
{
    xy_ads1115_t ads;
    int16_t raw;
    int32_t mv;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_init(NULL, &bus, ADS1115_ADDR_GND));
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_init(&ads, NULL, ADS1115_ADDR_GND));

    init_ads_ok(&ads, &bus);
    TEST_ASSERT_TRUE(ads.initialized);
    TEST_ASSERT_EQUAL_UINT16(ADS1115_ADDR_GND, g_last_addr);
    TEST_ASSERT_EQUAL_UINT32(1000U, g_last_timeout);
    TEST_ASSERT_EQUAL_INT(ADS1115_PGA_2_048V, ads.pga);
    TEST_ASSERT_EQUAL_INT(ADS1115_DR_128SPS, ads.dr);

    uint16_t single_cfg = ADS1115_CONFIG_OS_SINGLE | ADS1115_CONFIG_MUX_SINGLE_2 |
                          (ADS1115_PGA_2_048V << 9) | ADS1115_CONFIG_MODE_SINGLE |
                          (ADS1115_DR_128SPS << 5) | ADS1115_CONFIG_COMP_DISABLE;
    queue_write_reg_config(single_cfg, XY_DEVICE_OK);
    queue_read16(ADS1115_REG_CONVERT, 0x1234U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_read_single(&ads, 2U, &raw));
    TEST_ASSERT_EQUAL_INT16(0x1234, raw);
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_total);
    TEST_ASSERT_EQUAL_INT16(raw, ads.last_value);

    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_set_pga(&ads, ADS1115_PGA_4_096V));
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_set_dr(&ads, ADS1115_DR_860SPS));
    uint16_t diff_cfg = ADS1115_CONFIG_OS_SINGLE | ADS1115_CONFIG_MUX_DIFF_0_3 |
                        (ADS1115_PGA_4_096V << 9) | ADS1115_CONFIG_MODE_SINGLE |
                        (ADS1115_DR_860SPS << 5) | ADS1115_CONFIG_COMP_DISABLE;
    queue_write_reg_config(diff_cfg, XY_DEVICE_OK);
    queue_read16(ADS1115_REG_CONVERT, 0xFF00U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_read_diff(&ads, 0U, 3U, &raw));
    TEST_ASSERT_EQUAL_INT16(-256, raw);

    queue_write_reg_config(ADS1115_CONFIG_OS_SINGLE | ADS1115_CONFIG_MUX_SINGLE_1 |
                           (ADS1115_PGA_4_096V << 9) | ADS1115_CONFIG_MODE_SINGLE |
                           (ADS1115_DR_860SPS << 5) | ADS1115_CONFIG_COMP_DISABLE,
                           XY_DEVICE_OK);
    queue_read16(ADS1115_REG_CONVERT, 0x0800U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_read_voltage(&ads, 1U, &mv));
    TEST_ASSERT_EQUAL_INT32(256, mv);

    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_read_single(NULL, 0U, &raw));
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_read_single(&ads, 4U, &raw));
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_read_diff(&ads, 2U, 1U, &raw));
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_read_voltage(&ads, 4U, &mv));
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_set_pga(&ads, (xy_ads1115_pga_t)6));
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_set_dr(NULL, ADS1115_DR_128SPS));
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_deinit(&ads));
    TEST_ASSERT_FALSE(ads.initialized);
    TEST_ASSERT_FALSE(ads.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_deinit(&ads));
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM,
                          xy_ads1115_read_voltage(&ads, 0U, &mv));
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_read_voltage(&ads, 4U, &mv));
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM,
                          xy_ads1115_set_dr(&ads, ADS1115_DR_860SPS));
    TEST_ASSERT_EQUAL_INT(ADS1115_PGA_4_096V, ads.pga);
    TEST_ASSERT_EQUAL_INT(ADS1115_DR_860SPS, ads.dr);
}

static void test_ads1115_not_found_and_io_failure_paths(void)
{
    xy_ads1115_t ads = {0};
    int16_t raw = 0;
    int bus;

    queue_read16(ADS1115_REG_CONFIG, 0x0000U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_ads1115_init(&ads, &bus, ADS1115_ADDR_VDD));
    TEST_ASSERT_FALSE(ads.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_PTR(NULL, ads.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_read_single(&ads, 0U, &raw));

    setUp();
    init_ads_ok(&ads, &bus);
    queue_write_reg_config(ADS1115_CONFIG_OS_SINGLE | ADS1115_CONFIG_MUX_SINGLE_0 |
                           (ADS1115_PGA_2_048V << 9) | ADS1115_CONFIG_MODE_SINGLE |
                           (ADS1115_DR_128SPS << 5) | ADS1115_CONFIG_COMP_DISABLE,
                           XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_ads1115_read_single(&ads, 0U, &raw));

    raw = 0x5555;
    ads.last_value = 0x1234;
    queue_write_reg_config(ADS1115_CONFIG_OS_SINGLE | ADS1115_CONFIG_MUX_SINGLE_0 |
                           (ADS1115_PGA_2_048V << 9) | ADS1115_CONFIG_MODE_SINGLE |
                           (ADS1115_DR_128SPS << 5) | ADS1115_CONFIG_COMP_DISABLE,
                           XY_DEVICE_OK);
    queue_read16(ADS1115_REG_CONVERT, 0xABCDU, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_ads1115_read_single(&ads, 0U, &raw));
    TEST_ASSERT_EQUAL_INT16(0x5555, raw);
    TEST_ASSERT_EQUAL_INT16(0x1234, ads.last_value);
}

static void test_ads1115_propagates_i2c_init_failure_without_bus_io(void)
{
    xy_ads1115_t ads;
    int bus;

    memset(&ads, 0xA5, sizeof(ads));
    g_i2c_init_ret = XY_DEVICE_ERROR;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_ads1115_init(&ads, &bus, ADS1115_ADDR_GND));
    TEST_ASSERT_FALSE(ads.initialized);
    TEST_ASSERT_FALSE(ads.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_PTR(NULL, ads.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT(0U, g_op_index);
}



static void test_ads1115_read_voltage_failure_preserves_output(void)
{
    xy_ads1115_t ads;
    int32_t mv = 123456;
    int bus;

    init_ads_ok(&ads, &bus);
    ads.last_value = 0x2468;
    queue_write_reg_config(ADS1115_CONFIG_OS_SINGLE | ADS1115_CONFIG_MUX_SINGLE_1 |
                           (ADS1115_PGA_2_048V << 9) | ADS1115_CONFIG_MODE_SINGLE |
                           (ADS1115_DR_128SPS << 5) | ADS1115_CONFIG_COMP_DISABLE,
                           XY_DEVICE_OK);
    queue_read16(ADS1115_REG_CONVERT, 0xFFFFU, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_ads1115_read_voltage(&ads, 1U, &mv));
    TEST_ASSERT_EQUAL_INT32(123456, mv);
    TEST_ASSERT_EQUAL_INT16(0x2468, ads.last_value);

    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_read_voltage(&ads, 0U, NULL));
}

static void test_ads1115_diff_mux_variants_and_voltage_ranges(void)
{
    xy_ads1115_t ads;
    int16_t raw = 0;
    int32_t mv = 0;
    int bus;

    init_ads_ok(&ads, &bus);

    queue_write_reg_config(ADS1115_CONFIG_OS_SINGLE | ADS1115_CONFIG_MUX_DIFF_0_1 |
                           (ADS1115_PGA_2_048V << 9) | ADS1115_CONFIG_MODE_SINGLE |
                           (ADS1115_DR_128SPS << 5) | ADS1115_CONFIG_COMP_DISABLE,
                           XY_DEVICE_OK);
    queue_read16(ADS1115_REG_CONVERT, 0x0100U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_read_diff(&ads, 0U, 1U, &raw));
    TEST_ASSERT_EQUAL_INT16(0x0100, raw);

    queue_write_reg_config(ADS1115_CONFIG_OS_SINGLE | ADS1115_CONFIG_MUX_DIFF_1_3 |
                           (ADS1115_PGA_2_048V << 9) | ADS1115_CONFIG_MODE_SINGLE |
                           (ADS1115_DR_128SPS << 5) | ADS1115_CONFIG_COMP_DISABLE,
                           XY_DEVICE_OK);
    queue_read16(ADS1115_REG_CONVERT, 0x0200U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_read_diff(&ads, 1U, 3U, &raw));
    TEST_ASSERT_EQUAL_INT16(0x0200, raw);

    queue_write_reg_config(ADS1115_CONFIG_OS_SINGLE | ADS1115_CONFIG_MUX_DIFF_2_3 |
                           (ADS1115_PGA_2_048V << 9) | ADS1115_CONFIG_MODE_SINGLE |
                           (ADS1115_DR_128SPS << 5) | ADS1115_CONFIG_COMP_DISABLE,
                           XY_DEVICE_OK);
    queue_read16(ADS1115_REG_CONVERT, 0x0300U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_read_diff(&ads, 2U, 3U, &raw));
    TEST_ASSERT_EQUAL_INT16(0x0300, raw);

    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_set_pga(&ads, ADS1115_PGA_6_144V));
    queue_write_reg_config(ADS1115_CONFIG_OS_SINGLE | ADS1115_CONFIG_MUX_SINGLE_3 |
                           (ADS1115_PGA_6_144V << 9) | ADS1115_CONFIG_MODE_SINGLE |
                           (ADS1115_DR_128SPS << 5) | ADS1115_CONFIG_COMP_DISABLE,
                           XY_DEVICE_OK);
    queue_read16(ADS1115_REG_CONVERT, 0x1000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_read_voltage(&ads, 3U, &mv));
    TEST_ASSERT_EQUAL_INT32(768, mv);

    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_set_pga(&ads, ADS1115_PGA_0_256V));
    queue_write_reg_config(ADS1115_CONFIG_OS_SINGLE | ADS1115_CONFIG_MUX_SINGLE_0 |
                           (ADS1115_PGA_0_256V << 9) | ADS1115_CONFIG_MODE_SINGLE |
                           (ADS1115_DR_128SPS << 5) | ADS1115_CONFIG_COMP_DISABLE,
                           XY_DEVICE_OK);
    queue_read16(ADS1115_REG_CONVERT, 0x1000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_read_voltage(&ads, 0U, &mv));
    TEST_ASSERT_EQUAL_INT32(31, mv);
}

static void test_ads1115_read_paths_reject_missing_i2c_context(void)
{
    xy_ads1115_t ads;
    int16_t raw = 1234;
    int bus;

    init_ads_ok(&ads, &bus);
    ads.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM,
                          xy_ads1115_read_single(&ads, 0U, &raw));
    TEST_ASSERT_EQUAL_INT16(1234, raw);
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM,
                          xy_ads1115_read_diff(&ads, 0U, 1U, &raw));
    TEST_ASSERT_EQUAL_INT16(1234, raw);
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);
}

static void test_ads1115_setters_reject_missing_i2c_context_without_cache_change(void)
{
    xy_ads1115_t ads;
    int bus;

    init_ads_ok(&ads, &bus);
    ads.i2c_dev.base.initialized = 0U;
    ads.pga = ADS1115_PGA_2_048V;
    ads.dr = ADS1115_DR_128SPS;

    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM,
                          xy_ads1115_set_pga(&ads, ADS1115_PGA_4_096V));
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM,
                          xy_ads1115_set_dr(&ads, ADS1115_DR_860SPS));
    TEST_ASSERT_EQUAL_INT(ADS1115_PGA_2_048V, ads.pga);
    TEST_ASSERT_EQUAL_INT(ADS1115_DR_128SPS, ads.dr);
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);
}

static void test_ads1115_deinit_rejects_missing_i2c_context_without_lifecycle_change(void)
{
    xy_ads1115_t ads;
    int bus;

    init_ads_ok(&ads, &bus);
    ads.i2c_dev.base.initialized = 0U;

    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_deinit(&ads));
    TEST_ASSERT_TRUE(ads.initialized);
    TEST_ASSERT_FALSE(ads.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);
}


static xy_ina219_config_t ina219_config(void)
{
    xy_ina219_config_t config = {
        .shunt_resistance_uohm = 100000U,
        .current_lsb_ua = 100U,
        .config_register = XY_INA219_CONFIG_DEFAULT,
    };
    return config;
}

static void init_ina219_ok(xy_ina219_t *ina, int *bus)
{
    xy_ina219_config_t config = ina219_config();
    queue_write_reg16(XY_INA219_REG_CONFIG, XY_INA219_CONFIG_RESET, XY_DEVICE_OK);
    queue_write_reg16(XY_INA219_REG_CONFIG, XY_INA219_CONFIG_DEFAULT, XY_DEVICE_OK);
    queue_write_reg16(XY_INA219_REG_CALIBRATION, 4096U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_ina219_init(ina, bus, XY_INA219_ADDR_DEFAULT, &config));
}

static void test_ina219_init_and_measurement_contract(void)
{
    xy_ina219_t ina;
    xy_ina219_sample_t sample = {0};
    xy_ina219_config_t config = ina219_config();
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_ina219_init(NULL, &bus, XY_INA219_ADDR_DEFAULT, &config));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_ina219_init(&ina, NULL, XY_INA219_ADDR_DEFAULT, &config));
    config.shunt_resistance_uohm = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_ina219_init(&ina, &bus, XY_INA219_ADDR_DEFAULT, &config));

    init_ina219_ok(&ina, &bus);
    TEST_ASSERT_TRUE(ina.initialized);
    TEST_ASSERT_EQUAL_UINT16(4096U, ina.calibration_register);
    TEST_ASSERT_EQUAL_UINT16(XY_INA219_ADDR_DEFAULT, g_last_addr);

    queue_read16(XY_INA219_REG_SHUNT_VOLTAGE, 0xFFF6U, XY_DEVICE_OK);
    queue_read16(XY_INA219_REG_BUS_VOLTAGE, 0x5DC0U, XY_DEVICE_OK);
    queue_read16(XY_INA219_REG_CURRENT, 0xFF9CU, XY_DEVICE_OK);
    queue_read16(XY_INA219_REG_POWER, 50U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ina219_read_sample(&ina, &sample));
    TEST_ASSERT_EQUAL_INT32(-100, sample.shunt_voltage_uv);
    TEST_ASSERT_EQUAL_UINT32(12000U, sample.bus_voltage_mv);
    TEST_ASSERT_EQUAL_INT32(-10000, sample.current_ua);
    TEST_ASSERT_EQUAL_UINT32(100000U, sample.power_uw);
    TEST_ASSERT_EQUAL_MEMORY(&sample, &ina.sample, sizeof(sample));

    queue_write_reg16(XY_INA219_REG_CONFIG, 0U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ina219_deinit(&ina));
    TEST_ASSERT_FALSE(ina.initialized);
    TEST_ASSERT_FALSE(ina.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(ina.i2c_dev.i2c_handle);
}

static void test_ina219_failures_preserve_state_and_stop_io(void)
{
    xy_ina219_t ina;
    xy_ina219_sample_t output = {.shunt_voltage_uv = 1,
                                 .bus_voltage_mv = 2U,
                                 .current_ua = 3,
                                 .power_uw = 4U};
    xy_ina219_sample_t output_snapshot = output;
    int bus;

    init_ina219_ok(&ina, &bus);
    ina.sample.shunt_voltage_uv = 11;
    ina.sample.bus_voltage_mv = 22U;
    ina.sample.current_ua = 33;
    ina.sample.power_uw = 44U;
    const xy_ina219_sample_t cache_snapshot = ina.sample;

    queue_read16(XY_INA219_REG_SHUNT_VOLTAGE, 10U, XY_DEVICE_OK);
    queue_read16(XY_INA219_REG_BUS_VOLTAGE, 0U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_ina219_read_sample(&ina, &output));
    TEST_ASSERT_EQUAL_MEMORY(&output_snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_MEMORY(&cache_snapshot, &ina.sample, sizeof(ina.sample));
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);

    ina.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_ina219_read_sample(&ina, &output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ina219_deinit(&ina));
    TEST_ASSERT_TRUE(ina.initialized);
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);
}

static void test_ina219_init_failure_clears_lifecycle(void)
{
    xy_ina219_t ina;
    xy_ina219_config_t config = ina219_config();
    int bus;

    queue_write_reg16(XY_INA219_REG_CONFIG, XY_INA219_CONFIG_RESET, XY_DEVICE_OK);
    queue_write_reg16(XY_INA219_REG_CONFIG, XY_INA219_CONFIG_DEFAULT, XY_DEVICE_OK);
    queue_write_reg16(XY_INA219_REG_CALIBRATION, 4096U, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_ina219_init(&ina, &bus, XY_INA219_ADDR_DEFAULT, &config));
    TEST_ASSERT_FALSE(ina.initialized);
    TEST_ASSERT_FALSE(ina.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);
}

static void test_ina219_public_operations_require_live_i2c_handle_and_deinit_clears_it(void)
{
    xy_ina219_t ina;
    xy_ina219_sample_t output = {.shunt_voltage_uv = 1,
                                 .bus_voltage_mv = 2U,
                                 .current_ua = 3,
                                 .power_uw = 4U};
    const xy_ina219_sample_t output_snapshot = output;
    int bus;

    init_ina219_ok(&ina, &bus);
    ina.i2c_dev.i2c_handle = NULL;
    const size_t operations_before = g_op_count;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ina219_read_sample(&ina, &output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ina219_deinit(&ina));
    TEST_ASSERT_EQUAL_MEMORY(&output_snapshot, &output, sizeof(output));
    TEST_ASSERT_EQUAL_UINT(operations_before, g_op_count);
    TEST_ASSERT_TRUE(ina.initialized);

    ina.i2c_dev.i2c_handle = &bus;
    queue_write_reg16(XY_INA219_REG_CONFIG, 0U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ina219_deinit(&ina));
    TEST_ASSERT_NULL(ina.i2c_dev.i2c_handle);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ltc2945_datasheet_registers_scaling_and_lifecycle);
    RUN_TEST(test_ltc2945_failures_are_atomic_and_stop_at_first_error);
    RUN_TEST(test_ltc2945_init_failures_clear_complete_lifecycle);
    RUN_TEST(test_ltc2945_missing_handle_fails_closed);
    RUN_TEST(test_ads1115_single_diff_voltage_config_and_invalid_paths);
    RUN_TEST(test_ads1115_not_found_and_io_failure_paths);
    RUN_TEST(test_ads1115_propagates_i2c_init_failure_without_bus_io);
    RUN_TEST(test_ads1115_read_voltage_failure_preserves_output);
    RUN_TEST(test_ads1115_diff_mux_variants_and_voltage_ranges);
    RUN_TEST(test_ads1115_read_paths_reject_missing_i2c_context);
    RUN_TEST(test_ads1115_setters_reject_missing_i2c_context_without_cache_change);
    RUN_TEST(test_ads1115_deinit_rejects_missing_i2c_context_without_lifecycle_change);
    RUN_TEST(test_ina219_init_and_measurement_contract);
    RUN_TEST(test_ina219_failures_preserve_state_and_stop_io);
    RUN_TEST(test_ina219_init_failure_clears_lifecycle);
    RUN_TEST(test_ina219_public_operations_require_live_i2c_handle_and_deinit_clears_it);
    return UNITY_END();
}
