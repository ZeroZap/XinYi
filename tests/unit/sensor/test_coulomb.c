#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_coulomb.h"
#include "xy_device.h"
#include "xy_hal_sys.h"

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
    TEST_ASSERT_LESS_THAN_UINT(g_read_count, g_read_index);
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
    TEST_ASSERT_LESS_THAN_UINT(g_write_count, g_write_index);
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
    return 987654U;
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
}

void tearDown(void)
{
}

static xy_coulomb_config_t coulomb_config(void)
{
    xy_coulomb_config_t cfg = {
        .shunt_resistor_mohm = 10.0f,
        .capacity_mah = 3276.8f,
        .avg_samples = 4U,
        .alert_current_ma = 500U,
    };
    return cfg;
}

static void init_coulomb_ok(xy_coulomb_t *coulomb, int *bus)
{
    xy_coulomb_config_t cfg = coulomb_config();
    queue_read16(INA226_REG_MFG_ID, INA226_MFG_ID_VALUE, XY_DEVICE_OK);
    queue_read16(INA226_REG_DIE_ID, INA226_DIE_ID_VALUE, XY_DEVICE_OK);
    queue_write16(INA226_REG_CALIB, 5120U, XY_DEVICE_OK);
    queue_write16(INA226_REG_CONFIG, 0xF240U, XY_DEVICE_OK);
    queue_write16(INA226_REG_CURRENT, 0x0000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_OK, xy_coulomb_init(coulomb, bus, INA226_ADDR_GND, &cfg));
}

static void test_coulomb_init_read_controls_and_invalid_paths(void)
{
    xy_coulomb_t coulomb;
    xy_coulomb_config_t cfg = coulomb_config();
    float value;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_init(NULL, &bus, INA226_ADDR_GND, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_init(&coulomb, NULL, INA226_ADDR_GND, &cfg));
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_init(&coulomb, &bus, INA226_ADDR_GND, NULL));

    init_coulomb_ok(&coulomb, &bus);
    TEST_ASSERT_TRUE(coulomb.initialized);
    TEST_ASSERT_EQUAL_UINT16(INA226_ADDR_GND, g_last_addr);
    TEST_ASSERT_EQUAL_UINT32(1000U, g_last_timeout);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.0001f, coulomb.current_lsb);
    TEST_ASSERT_EQUAL_UINT16(5120U, coulomb.calib_value);

    queue_read16(INA226_REG_BUS_VOLT, 8000U, XY_DEVICE_OK);
    queue_read16(INA226_REG_SHUNT_VOLT, 400U, XY_DEVICE_OK);
    queue_read16(INA226_REG_POWER, 1000U, XY_DEVICE_OK);
    queue_read16(INA226_REG_CURRENT, 10000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_OK, xy_coulomb_read(&coulomb));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10000.0f, coulomb.data.voltage_mv);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, coulomb.data.current_ma);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, coulomb.data.power_mw);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1000.0f, coulomb.data.charge_mah);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 69.482f, coulomb.data.percentage);
    TEST_ASSERT_EQUAL_UINT32(987654U, coulomb.data.timestamp);

    TEST_ASSERT_EQUAL_INT(XY_COULOMB_OK, xy_coulomb_set_capacity(&coulomb, 2500.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2500.0f, coulomb.config.capacity_mah);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_set_capacity(&coulomb, 0.0f));

    queue_write16(INA226_REG_CURRENT, 0x0000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_coulomb_reset_charge(&coulomb));
    queue_write16(INA226_REG_MASK_EN, 0x0010U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_coulomb_enable_alert(&coulomb, true));
    queue_write16(INA226_REG_MASK_EN, 0x0000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_coulomb_enable_alert(&coulomb, false));
    queue_write16(INA226_REG_CONFIG, 0x0000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_OK, xy_coulomb_deinit(&coulomb));
    TEST_ASSERT_FALSE(coulomb.initialized);

    value = 55.0f;
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_get_voltage(NULL, &value));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 55.0f, value);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_get_current(&coulomb, NULL));
    value = 66.0f;
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_get_power(NULL, &value));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 66.0f, value);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_get_charge(&coulomb, NULL));
    value = 77.0f;
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_get_percentage(NULL, &value));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 77.0f, value);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_reset_charge(NULL));
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_enable_alert(NULL, true));
}

static void test_coulomb_not_found_and_write_failures(void)
{
    xy_coulomb_t coulomb;
    xy_coulomb_config_t cfg = coulomb_config();
    int bus;

    queue_read16(INA226_REG_MFG_ID, 0x0000U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_NOT_FOUND, xy_coulomb_init(&coulomb, &bus, INA226_ADDR_GND, &cfg));

    queue_read16(INA226_REG_MFG_ID, INA226_MFG_ID_VALUE, XY_DEVICE_OK);
    queue_read16(INA226_REG_DIE_ID, 0x0000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_NOT_FOUND, xy_coulomb_init(&coulomb, &bus, INA226_ADDR_GND, &cfg));

    queue_read16(INA226_REG_MFG_ID, INA226_MFG_ID_VALUE, XY_DEVICE_OK);
    queue_read16(INA226_REG_DIE_ID, INA226_DIE_ID_VALUE, XY_DEVICE_OK);
    queue_write16(INA226_REG_CALIB, 5120U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_ERROR, xy_coulomb_init(&coulomb, &bus, INA226_ADDR_GND, &cfg));

    queue_read16(INA226_REG_MFG_ID, INA226_MFG_ID_VALUE, XY_DEVICE_OK);
    queue_read16(INA226_REG_DIE_ID, INA226_DIE_ID_VALUE, XY_DEVICE_OK);
    queue_write16(INA226_REG_CALIB, 5120U, XY_DEVICE_OK);
    queue_write16(INA226_REG_CONFIG, 0xF240U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_ERROR, xy_coulomb_init(&coulomb, &bus, INA226_ADDR_GND, &cfg));
}

static void test_coulomb_init_tolerates_reset_charge_failure(void)
{
    xy_coulomb_t coulomb;
    xy_coulomb_config_t cfg = coulomb_config();
    int bus;

    queue_read16(INA226_REG_MFG_ID, INA226_MFG_ID_VALUE, XY_DEVICE_OK);
    queue_read16(INA226_REG_DIE_ID, INA226_DIE_ID_VALUE, XY_DEVICE_OK);
    queue_write16(INA226_REG_CALIB, 5120U, XY_DEVICE_OK);
    queue_write16(INA226_REG_CONFIG, 0xF240U, XY_DEVICE_OK);
    queue_write16(INA226_REG_CURRENT, 0x0000U, XY_DEVICE_ERROR);

    TEST_ASSERT_EQUAL_INT(XY_COULOMB_OK, xy_coulomb_init(&coulomb, &bus, INA226_ADDR_GND, &cfg));
    TEST_ASSERT_TRUE(coulomb.initialized);
    TEST_ASSERT_EQUAL_UINT(3U, g_write_index);
}

static void test_coulomb_getters_reread_and_clamp_percentage(void)
{
    xy_coulomb_t coulomb;
    float value;
    int bus;

    init_coulomb_ok(&coulomb, &bus);

    queue_read16(INA226_REG_BUS_VOLT, 4000U, XY_DEVICE_OK);
    queue_read16(INA226_REG_SHUNT_VOLT, 0xFF9CU, XY_DEVICE_OK);
    queue_read16(INA226_REG_POWER, 0U, XY_DEVICE_OK);
    queue_read16(INA226_REG_CURRENT, 0x8000U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_OK, xy_coulomb_get_voltage(&coulomb, &value));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5000.0f, value);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -25.0f, coulomb.data.current_ma);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, coulomb.data.percentage);

    queue_read16(INA226_REG_BUS_VOLT, 0U, XY_DEVICE_ERROR);
    queue_read16(INA226_REG_SHUNT_VOLT, 0U, XY_DEVICE_ERROR);
    queue_read16(INA226_REG_POWER, 0U, XY_DEVICE_ERROR);
    queue_read16(INA226_REG_CURRENT, 0U, XY_DEVICE_ERROR);
    value = 1234.0f;
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_OK, xy_coulomb_get_current(&coulomb, &value));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -25.0f, value);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5000.0f, coulomb.data.voltage_mv);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, coulomb.data.power_mw);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, coulomb.data.percentage);

    queue_read16(INA226_REG_BUS_VOLT, 0U, XY_DEVICE_OK);
    queue_read16(INA226_REG_SHUNT_VOLT, 0U, XY_DEVICE_OK);
    queue_read16(INA226_REG_POWER, 200U, XY_DEVICE_OK);
    queue_read16(INA226_REG_CURRENT, 0U, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_OK, xy_coulomb_get_power(&coulomb, &value));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, value);
}

static void test_coulomb_control_failures_and_uninitialized_getters(void)
{
    xy_coulomb_t coulomb;
    float value = 88.0f;
    int bus;

    memset(&coulomb, 0, sizeof(coulomb));
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_read(&coulomb));
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_get_charge(&coulomb, &value));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 88.0f, value);

    init_coulomb_ok(&coulomb, &bus);

    TEST_ASSERT_EQUAL_INT(XY_COULOMB_OK, xy_coulomb_set_capacity(&coulomb, 2000.0f));
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_INVALID_PARAM, xy_coulomb_set_capacity(&coulomb, -1.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2000.0f, coulomb.config.capacity_mah);

    queue_write16(INA226_REG_CURRENT, 0x0000U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_coulomb_reset_charge(&coulomb));

    queue_write16(INA226_REG_MASK_EN, 0x0010U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_coulomb_enable_alert(&coulomb, true));

    queue_write16(INA226_REG_CONFIG, 0x0000U, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_COULOMB_OK, xy_coulomb_deinit(&coulomb));
    TEST_ASSERT_FALSE(coulomb.initialized);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_coulomb_init_read_controls_and_invalid_paths);
    RUN_TEST(test_coulomb_not_found_and_write_failures);
    RUN_TEST(test_coulomb_init_tolerates_reset_charge_failure);
    RUN_TEST(test_coulomb_getters_reread_and_clamp_percentage);
    RUN_TEST(test_coulomb_control_failures_and_uninitialized_getters);
    return UNITY_END();
}
