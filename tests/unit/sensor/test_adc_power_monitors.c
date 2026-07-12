#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_device.h"
#include "xy_hal_sys.h"
#include "xy_ltc2945.h"
#include "xy_ads1115.h"
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

static void queue_write(uint8_t reg, uint8_t value, xy_error_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    g_ops[g_op_count].kind = OP_WRITE;
    g_ops[g_op_count].data[0] = reg;
    g_ops[g_op_count].data[1] = value;
    g_ops[g_op_count].len = 2U;
    g_ops[g_op_count].ret = ret;
    g_op_count++;
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
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
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
    TEST_ASSERT_TRUE(dev->base.initialized);
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
    TEST_ASSERT_TRUE(dev->base.initialized);
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
}

void tearDown(void)
{
}

static xy_ltc2945_config_t ltc_config(void)
{
    xy_ltc2945_config_t cfg = {
        .shunt_resistor_mohm = 10.0f,
        .auto_convert = true,
        .alert_gpio_config = 0xA5U,
    };
    return cfg;
}

static void init_ltc_ok(xy_ltc2945_t *ltc, int *bus)
{
    xy_ltc2945_config_t cfg = ltc_config();
    queue_read8(LTC2945_REG_STATUS, 0x55U, XY_DEVICE_OK);
    queue_write(LTC2945_REG_CONTROL, 0x08U, XY_DEVICE_OK);
    queue_write(LTC2945_REG_CTRL_GPIO, 0xA5U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_OK, xy_ltc2945_init(ltc, bus, LTC2945_ADDR_ADDR0, &cfg));
}

static void test_ltc2945_init_read_controls_and_invalid_paths(void)
{
    xy_ltc2945_t ltc;
    xy_ltc2945_config_t cfg = ltc_config();
    float value;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_init(NULL, &bus, LTC2945_ADDR_ADDR0, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_init(&ltc, NULL, LTC2945_ADDR_ADDR0, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_init(&ltc, &bus, LTC2945_ADDR_ADDR0, NULL));

    init_ltc_ok(&ltc, &bus);
    TEST_ASSERT_TRUE(ltc.initialized);
    TEST_ASSERT_EQUAL_UINT16(LTC2945_ADDR_ADDR0, g_last_addr);
    TEST_ASSERT_EQUAL_UINT32(400U, g_last_timeout);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.0000625f, ltc.power_lsb);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.0025f, ltc.charge_lsb);

    queue_read16(LTC2945_REG_VIN_MSB, 0x1000U, XY_DEVICE_OK);
    queue_read16(LTC2945_REG_VSENSE_MSB, 0x0100U, XY_DEVICE_OK);
    queue_read24(LTC2945_REG_POWER_MSB, 0x000064U, XY_DEVICE_OK);
    queue_read24(LTC2945_REG_CHARGE_MSB, 0x00000AU, XY_DEVICE_OK);
    queue_read24(LTC2945_REG_ENERGY_MSB, 0x000014U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_OK, xy_ltc2945_read(&ltc));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 6.4f, ltc.data.voltage_v);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.04f, ltc.data.current_a);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.00625f, ltc.data.power_w);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.025f, ltc.data.charge_c);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.00125f, ltc.data.energy_j);
    TEST_ASSERT_EQUAL_UINT32(222333U, ltc.data.timestamp);

    queue_write(LTC2945_REG_CONTROL, 0x08U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_OK, xy_ltc2945_reset_counters(&ltc));
    queue_write(LTC2945_REG_ALERT, 0x01U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ltc2945_enable_alert(&ltc, true));

    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_get_voltage(NULL, &value));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_get_current(&ltc, NULL));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_get_power(NULL, &value));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_get_charge(&ltc, NULL));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_get_energy(NULL, &value));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_reset_counters(NULL));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_enable_alert(NULL, false));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_OK, xy_ltc2945_deinit(&ltc));
    TEST_ASSERT_FALSE(ltc.initialized);
}

static void test_ltc2945_not_found_and_uninitialized_read(void)
{
    xy_ltc2945_t ltc = {0};
    xy_ltc2945_config_t cfg = ltc_config();
    int bus;

    queue_read8(LTC2945_REG_STATUS, 0x00U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_NOT_FOUND, xy_ltc2945_init(&ltc, &bus, LTC2945_ADDR_ADDR1, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_read(NULL));
    TEST_ASSERT_EQUAL_INT(XY_LTC2945_INVALID_PARAM, xy_ltc2945_read(&ltc));
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
}

static void test_ads1115_not_found_and_io_failure_paths(void)
{
    xy_ads1115_t ads = {0};
    int16_t raw = 0;
    int bus;

    queue_read16(ADS1115_REG_CONFIG, 0x0000U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_NOT_FOUND, xy_ads1115_init(&ads, &bus, ADS1115_ADDR_VDD));
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_INVALID_PARAM, xy_ads1115_read_single(&ads, 0U, &raw));

    setUp();
    init_ads_ok(&ads, &bus);
    queue_write_reg_config(ADS1115_CONFIG_OS_SINGLE | ADS1115_CONFIG_MUX_SINGLE_0 |
                           (ADS1115_PGA_2_048V << 9) | ADS1115_CONFIG_MODE_SINGLE |
                           (ADS1115_DR_128SPS << 5) | ADS1115_CONFIG_COMP_DISABLE,
                           XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_ads1115_read_single(&ads, 0U, &raw));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ltc2945_init_read_controls_and_invalid_paths);
    RUN_TEST(test_ltc2945_not_found_and_uninitialized_read);
    RUN_TEST(test_ads1115_single_diff_voltage_config_and_invalid_paths);
    RUN_TEST(test_ads1115_not_found_and_io_failure_paths);
    return UNITY_END();
}
