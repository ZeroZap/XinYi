#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_device.h"
#include "xy_ina226.h"
#include "xy_max17043.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint8_t reg;
    uint8_t data[2];
    xy_error_t ret;
} read_op_t;

typedef struct {
    uint8_t bytes[3];
    size_t len;
    xy_error_t ret;
} write_op_t;

static read_op_t g_reads[64];
static write_op_t g_writes[64];
static uint8_t g_seen_writes[64][3];
static size_t g_read_count;
static size_t g_read_index;
static size_t g_write_count;
static size_t g_write_index;
static size_t g_seen_write_count;
static uint16_t g_last_addr;
static uint32_t g_last_timeout;
static uint32_t g_delay_total;

static void queue_read16(uint8_t reg, uint16_t value, xy_error_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_count);
    g_reads[g_read_count].reg = reg;
    g_reads[g_read_count].data[0] = (uint8_t)(value >> 8);
    g_reads[g_read_count].data[1] = (uint8_t)value;
    g_reads[g_read_count].ret = ret;
    g_read_count++;
}

static void queue_write16(uint8_t reg, uint16_t value, xy_error_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_writes), g_write_count);
    g_writes[g_write_count].bytes[0] = reg;
    g_writes[g_write_count].bytes[1] = (uint8_t)(value >> 8);
    g_writes[g_write_count].bytes[2] = (uint8_t)value;
    g_writes[g_write_count].len = 3U;
    g_writes[g_write_count].ret = ret;
    g_write_count++;
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
    TEST_ASSERT_EQUAL_UINT(2U, len);
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_reads), g_read_index);
    TEST_ASSERT_EQUAL_UINT8(g_reads[g_read_index].reg, reg);

    xy_error_t ret = g_reads[g_read_index].ret;
    if (ret == XY_DEVICE_OK) {
        memcpy(data, g_reads[g_read_index].data, len);
    }
    g_read_index++;
    return ret;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_writes), g_write_index);
    TEST_ASSERT_EQUAL_UINT(g_writes[g_write_index].len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(g_writes[g_write_index].bytes, data, len);

    memcpy(g_seen_writes[g_seen_write_count++], data, len);
    return g_writes[g_write_index++].ret;
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

uint32_t xy_hal_sys_get_tick_count(void)
{
    return 123456U;
}

void xy_hal_delay_ms(uint32_t ms)
{
    g_delay_total += ms;
}

uint32_t xy_os_tick_get(void)
{
    return 654321U;
}

void setUp(void)
{
    memset(g_reads, 0, sizeof(g_reads));
    memset(g_writes, 0, sizeof(g_writes));
    memset(g_seen_writes, 0, sizeof(g_seen_writes));
    g_read_count = 0;
    g_read_index = 0;
    g_write_count = 0;
    g_write_index = 0;
    g_seen_write_count = 0;
    g_last_addr = 0;
    g_last_timeout = 0;
    g_delay_total = 0;
}

void tearDown(void)
{
}

static xy_max17043_config_t max_config(void)
{
    xy_max17043_config_t cfg = {
        .capacity_mah = 3000.0f,
        .alert_voltage_mv = 3200.0f,
        .enable_hibernate = true,
    };
    return cfg;
}

static void init_max_ok(xy_max17043_t *gauge, int *bus)
{
    xy_max17043_config_t cfg = max_config();
    queue_read16(MAX17043_REG_VER, 0x0012U, XY_DEVICE_OK);
    queue_write16(MAX17043_REG_VALRT, 160U, XY_DEVICE_OK);
    queue_write16(MAX17043_REG_HIBRT, 0x4000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_OK, xy_max17043_init(gauge, bus, &cfg));
}

static void test_max17043_init_read_controls_and_reset(void)
{
    xy_max17043_t gauge;
    xy_max17043_config_t cfg = max_config();
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_init(NULL, &bus, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_init(&gauge, NULL, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_init(&gauge, &bus, NULL));

    init_max_ok(&gauge, &bus);
    TEST_ASSERT_TRUE(gauge.initialized);
    TEST_ASSERT_EQUAL_UINT16(MAX17043_ADDR, g_last_addr);
    TEST_ASSERT_EQUAL_UINT32(400U, g_last_timeout);
    TEST_ASSERT_EQUAL_UINT8(0x12U, gauge.data.version);

    queue_read16(MAX17043_REG_VCELL, 0xC800U, XY_DEVICE_OK);
    queue_read16(MAX17043_REG_SOC, 0x4B80U, XY_DEVICE_OK);
    queue_read16(MAX17043_REG_CRATE, 0xFF9CU, XY_DEVICE_OK);
    queue_read16(MAX17043_REG_STATUS, 0x0012U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_OK, xy_max17043_read(&gauge));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 250.0f, gauge.data.voltage_mv);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 75.5f, gauge.data.percentage);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.208f, gauge.data.crate);
    TEST_ASSERT_TRUE(gauge.data.low_battery);
    TEST_ASSERT_TRUE(gauge.data.reset_triggered);
    TEST_ASSERT_EQUAL_UINT32(123456U, gauge.data.timestamp);

    TEST_ASSERT_EQUAL_INT(XY_MAX17043_OK, xy_max17043_set_capacity(&gauge, 2500.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2500.0f, gauge.config.capacity_mah);
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_set_capacity(&gauge, 0.0f));

    queue_write16(MAX17043_REG_HIBRT, 0x0000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_max17043_enable_hibernate(&gauge, false));
    queue_write16(MAX17043_REG_UNLOCK, 0x0090U, XY_DEVICE_OK);
    queue_write16(MAX17043_REG_COMMAND, 0x0002U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_OK, xy_max17043_reset(&gauge));
    TEST_ASSERT_EQUAL_UINT32(100U, g_delay_total);
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_OK, xy_max17043_deinit(&gauge));
    TEST_ASSERT_FALSE(gauge.initialized);
}

static void test_max17043_read_partial_failures_keep_ok_and_preserve_failed_fields(void)
{
    xy_max17043_t gauge;
    int bus;

    init_max_ok(&gauge, &bus);
    gauge.data.voltage_mv = 1.0f;
    gauge.data.percentage = 2.0f;
    gauge.data.crate = 3.0f;
    gauge.data.low_battery = true;
    gauge.data.reset_triggered = true;
    gauge.data.timestamp = 4U;

    queue_read16(MAX17043_REG_VCELL, 0xC800U, XY_DEVICE_ERROR);
    queue_read16(MAX17043_REG_SOC, 0x4B80U, XY_DEVICE_ERROR);
    queue_read16(MAX17043_REG_CRATE, 0xFF9CU, XY_DEVICE_ERROR);
    queue_read16(MAX17043_REG_STATUS, 0x0000U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_OK, xy_max17043_read(&gauge));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, gauge.data.voltage_mv);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, gauge.data.percentage);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, gauge.data.crate);
    TEST_ASSERT_TRUE(gauge.data.low_battery);
    TEST_ASSERT_TRUE(gauge.data.reset_triggered);
    TEST_ASSERT_EQUAL_UINT32(123456U, gauge.data.timestamp);
}

static void test_max17043_not_found_and_getter_invalid_paths(void)
{
    xy_max17043_t gauge = {0};
    xy_max17043_config_t cfg = max_config();
    float value;
    int bus;

    queue_read16(MAX17043_REG_VER, 0x0000U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_NOT_FOUND, xy_max17043_init(&gauge, &bus, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_read(NULL));
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_read(&gauge));
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_get_voltage(NULL, &value));
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_get_voltage(&gauge, NULL));
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_get_percentage(NULL, &value));
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_get_percentage(&gauge, NULL));
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_get_crate(NULL, &value));
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_get_crate(&gauge, NULL));
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_set_capacity(NULL, 1000.0f));
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_enable_hibernate(NULL, true));
    TEST_ASSERT_EQUAL_INT(XY_MAX17043_INVALID_PARAM, xy_max17043_reset(NULL));
}

static xy_ina_config_t ina_config(void)
{
    xy_ina_config_t cfg = {
        .shunt_resistor_mohm = 10.0f,
        .avg_samples = XY_INA_AVG_16,
        .alert_current_ma = 5000U,
    };
    return cfg;
}

static void init_ina_ok(xy_ina_t *ina, int *bus)
{
    xy_ina_config_t cfg = ina_config();
    queue_read16(INA226_REG_MFG_ID, INA226_MFG_ID_VALUE, XY_DEVICE_OK);
    queue_read16(INA226_REG_DIE_ID, INA226_DIE_ID_VALUE, XY_DEVICE_OK);
    queue_write16(INA226_REG_CALIB, 204U, XY_DEVICE_OK);
    queue_write16(INA226_REG_CONFIG, 0xF220U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_INA_OK, xy_ina_init(ina, bus, INA226_ADDR_GND, &cfg));
}

static void test_ina226_init_read_getters_alert_and_deinit(void)
{
    xy_ina_t ina;
    xy_ina_config_t cfg = ina_config();
    float value;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_init(NULL, &bus, INA226_ADDR_GND, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_init(&ina, NULL, INA226_ADDR_GND, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_init(&ina, &bus, INA226_ADDR_GND, NULL));

    init_ina_ok(&ina, &bus);
    TEST_ASSERT_TRUE(ina.initialized);
    TEST_ASSERT_EQUAL_INT(XY_INA_DEVICE_INA226, ina.device);
    TEST_ASSERT_EQUAL_UINT16(INA226_ADDR_GND, g_last_addr);
    TEST_ASSERT_EQUAL_UINT32(1000U, g_last_timeout);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0025f, ina.current_lsb);
    TEST_ASSERT_EQUAL_UINT16(204U, ina.calib_value);

    queue_read16(INA226_REG_BUS_VOLT, 0x2000U, XY_DEVICE_OK);
    queue_read16(INA226_REG_SHUNT_VOLT, 0x0100U, XY_DEVICE_OK);
    queue_read16(INA226_REG_POWER, 0x0064U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_INA_OK, xy_ina_read(&ina));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10240.0f, ina.data.voltage_mv);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 64.0f, ina.data.current_ma);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.5f, ina.data.power_mw);
    TEST_ASSERT_EQUAL_UINT32(654321U, ina.data.timestamp);

    queue_read16(INA226_REG_BUS_VOLT, 0x1000U, XY_DEVICE_OK);
    queue_read16(INA226_REG_SHUNT_VOLT, 0x0000U, XY_DEVICE_OK);
    queue_read16(INA226_REG_POWER, 0x0000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_INA_OK, xy_ina_get_voltage(&ina, &value));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5120.0f, value);

    queue_write16(INA226_REG_MASK_EN, 0x0010U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ina_enable_alert(&ina, true));
    queue_write16(INA226_REG_CONFIG, 0x0000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_INA_OK, xy_ina_deinit(&ina));
    TEST_ASSERT_FALSE(ina.initialized);
}

static void test_ina226_partial_read_failures_keep_ok_and_preserve_failed_fields(void)
{
    xy_ina_t ina;
    int bus;

    init_ina_ok(&ina, &bus);
    ina.data.voltage_mv = 1.0f;
    ina.data.shunt_voltage_uv = 2.0f;
    ina.data.current_ma = 3.0f;
    ina.data.power_mw = 4.0f;
    ina.data.timestamp = 5U;

    queue_read16(INA226_REG_BUS_VOLT, 0x2000U, XY_DEVICE_ERROR);
    queue_read16(INA226_REG_SHUNT_VOLT, 0x0100U, XY_DEVICE_ERROR);
    queue_read16(INA226_REG_POWER, 0x0064U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_INA_OK, xy_ina_read(&ina));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, ina.data.voltage_mv);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, ina.data.shunt_voltage_uv);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, ina.data.current_ma);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, ina.data.power_mw);
    TEST_ASSERT_EQUAL_UINT32(654321U, ina.data.timestamp);
}

static void test_ina229_detection_and_invalid_paths(void)
{
    xy_ina_t ina = {0};
    xy_ina_config_t cfg = ina_config();
    float value;
    int bus;

    queue_read16(INA226_REG_MFG_ID, INA226_MFG_ID_VALUE, XY_DEVICE_OK);
    queue_read16(INA226_REG_DIE_ID, INA229_DIE_ID_VALUE, XY_DEVICE_OK);
    queue_write16(INA226_REG_CALIB, 204U, XY_DEVICE_OK);
    queue_write16(INA226_REG_CONFIG, 0xF220U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_INA_OK, xy_ina_init(&ina, &bus, INA226_ADDR_VREF, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_INA_DEVICE_INA229, ina.device);

    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_read(NULL));
    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_read(&(xy_ina_t){0}));
    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_get_voltage(NULL, &value));
    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_get_voltage(&ina, NULL));
    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_get_current(NULL, &value));
    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_get_current(&ina, NULL));
    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_get_power(NULL, &value));
    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_get_power(&ina, NULL));
    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_get_shunt_voltage(NULL, &value));
    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_get_shunt_voltage(&ina, NULL));
    TEST_ASSERT_EQUAL_INT(XY_INA_INVALID_PARAM, xy_ina_enable_alert(NULL, false));

    setUp();
    queue_read16(INA226_REG_MFG_ID, 0x0000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_INA_NOT_FOUND, xy_ina_init(&ina, &bus, INA226_ADDR_GND, &cfg));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_max17043_init_read_controls_and_reset);
    RUN_TEST(test_max17043_read_partial_failures_keep_ok_and_preserve_failed_fields);
    RUN_TEST(test_max17043_not_found_and_getter_invalid_paths);
    RUN_TEST(test_ina226_init_read_getters_alert_and_deinit);
    RUN_TEST(test_ina226_partial_read_failures_keep_ok_and_preserve_failed_fields);
    RUN_TEST(test_ina229_detection_and_invalid_paths);
    return UNITY_END();
}
