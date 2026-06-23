#include "unity.h"
#include "xy_fuel_gauge.h"
#include "xy_sensor_device.h"

#include <stdint.h>
#include <string.h>

int xy_fuel_gauge_max17043_register(void *i2c_handle, uint8_t addr);

#define MAX17043_ADDR       0x36
#define REG_VCELL           0x02
#define REG_SOC             0x04
#define REG_VER             0x08
#define REG_CONFIG          0x0C
#define REG_CRATE           0x16

static uint16_t fake_regs[256];
static xy_sensor_bus_t last_bus;
static uint8_t last_write_reg;
static uint16_t last_write_value;
static int read16_count;
static int write16_count;
static uint32_t fake_tick;

uint32_t xy_os_tick_get(void)
{
    return fake_tick;
}

void xy_sensor_bus_config_i2c(xy_sensor_bus_t *bus, void *handle, uint8_t address)
{
    TEST_ASSERT_NOT_NULL(bus);
    bus->type = XY_SENSOR_BUS_I2C;
    bus->bus_handle = handle;
    bus->address = address;
    bus->chip_select = 0;
    last_bus = *bus;
}

int xy_sensor_i2c_read_reg16(xy_sensor_bus_t *bus, uint8_t reg, uint16_t *value)
{
    TEST_ASSERT_NOT_NULL(bus);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, bus->type);
    TEST_ASSERT_NOT_NULL(value);

    *value = fake_regs[reg];
    read16_count++;
    return 0;
}

int xy_sensor_i2c_write_reg16(xy_sensor_bus_t *bus, uint8_t reg, uint16_t value)
{
    TEST_ASSERT_NOT_NULL(bus);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, bus->type);

    fake_regs[reg] = value;
    last_write_reg = reg;
    last_write_value = value;
    write16_count++;
    return 0;
}

int xy_sensor_i2c_read(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)reg;
    (void)data;
    (void)len;
    return -1;
}

int xy_sensor_i2c_write(xy_sensor_bus_t *bus, uint8_t reg, const uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)reg;
    (void)data;
    (void)len;
    return -1;
}

int xy_sensor_i2c_read_reg(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *value)
{
    (void)bus;
    (void)reg;
    (void)value;
    return -1;
}

int xy_sensor_i2c_write_reg(xy_sensor_bus_t *bus, uint8_t reg, uint8_t value)
{
    (void)bus;
    (void)reg;
    (void)value;
    return -1;
}

int xy_sensor_spi_read(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)reg;
    (void)data;
    (void)len;
    return -1;
}

int xy_sensor_spi_write(xy_sensor_bus_t *bus, uint8_t reg, const uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)reg;
    (void)data;
    (void)len;
    return -1;
}

int xy_sensor_check_device_id(xy_sensor_bus_t *bus, uint8_t id_reg, uint8_t expected_id)
{
    (void)bus;
    (void)id_reg;
    (void)expected_id;
    return -1;
}

void xy_sensor_bus_config_spi(xy_sensor_bus_t *bus, void *handle, uint8_t cs_pin)
{
    (void)bus;
    (void)handle;
    (void)cs_pin;
}

void setUp(void)
{
    memset(fake_regs, 0, sizeof(fake_regs));
    memset(&last_bus, 0, sizeof(last_bus));
    last_write_reg = 0;
    last_write_value = 0xFFFF;
    read16_count = 0;
    write16_count = 0;
    fake_tick = 1000;

    fake_regs[REG_VER] = 0x0011;
    fake_regs[REG_VCELL] = 0xC350;
    fake_regs[REG_SOC] = 0x4B00U;
    fake_regs[REG_CRATE] = (uint16_t)(int16_t)-1000;
}

void tearDown(void)
{
}

static xy_fuel_gauge_t *registered_max17043(void)
{
    xy_fuel_gauge_t *fg = xy_fuel_gauge_device_get("MAX17043");
    if (!fg) {
        TEST_ASSERT_EQUAL(XY_FG_OK,
                          xy_fuel_gauge_max17043_register((void *)0x17043, 0));
        fg = xy_fuel_gauge_device_get("MAX17043");
    }
    TEST_ASSERT_NOT_NULL(fg);
    return fg;
}

void test_max17043_registers_default_i2c_bus(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();

    TEST_ASSERT_NOT_NULL(fg);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, last_bus.type);
    TEST_ASSERT_EQUAL_PTR((void *)0x17043, last_bus.bus_handle);
    TEST_ASSERT_EQUAL_UINT8(MAX17043_ADDR, last_bus.address);
}

void test_max17043_init_writes_default_config(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE(fg->initialized);
    TEST_ASSERT_EQUAL_UINT8(REG_CONFIG, last_write_reg);
    TEST_ASSERT_EQUAL_UINT16(0, last_write_value);
    TEST_ASSERT_EQUAL_INT(1, write16_count);
}

void test_max17043_fetch_and_channel_get(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    fake_tick = 17043;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(17043, fg->latest.timestamp);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3906, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(75, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-208, value);

    TEST_ASSERT_GREATER_THAN(0, read16_count);
}

void test_max17043_rejects_unsupported_channel(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_get(fg, XY_FG_DATA_TEMPERATURE, &value));
}

void test_max17043_alert_set_get_uses_cached_thresholds(void)
{
    xy_fuel_gauge_t *fg = registered_max17043();
    xy_fuel_gauge_alert_t alert = {
        .low_soc_threshold = 15,
        .high_soc_threshold = 90,
        .low_voltage_mv = 3300,
        .high_voltage_mv = 4200,
        .over_current_ma = 1800,
        .over_temp_c = 55,
    };
    xy_fuel_gauge_alert_t readback;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_set_alert(fg, &alert));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_set_alert(NULL, &alert));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_get_alert(NULL, &readback));

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_set_alert(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_get_alert(fg, NULL));
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_set_alert(fg, &alert));
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get_alert(fg, &readback));

    TEST_ASSERT_EQUAL_UINT8(alert.low_soc_threshold, readback.low_soc_threshold);
    TEST_ASSERT_EQUAL_UINT8(alert.high_soc_threshold, readback.high_soc_threshold);
    TEST_ASSERT_EQUAL_UINT16(alert.low_voltage_mv, readback.low_voltage_mv);
    TEST_ASSERT_EQUAL_UINT16(alert.high_voltage_mv, readback.high_voltage_mv);
    TEST_ASSERT_EQUAL_INT16(alert.over_current_ma, readback.over_current_ma);
    TEST_ASSERT_EQUAL_INT16(alert.over_temp_c, readback.over_temp_c);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_max17043_registers_default_i2c_bus);
    RUN_TEST(test_max17043_init_writes_default_config);
    RUN_TEST(test_max17043_fetch_and_channel_get);
    RUN_TEST(test_max17043_rejects_unsupported_channel);
    RUN_TEST(test_max17043_alert_set_get_uses_cached_thresholds);
    return UNITY_END();
}
