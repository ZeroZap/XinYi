#include "unity.h"
#include "xy_fuel_gauge.h"
#include "xy_sensor_device.h"

#include <stdint.h>
#include <string.h>

int xy_fuel_gauge_bq27z561_register(void *i2c_handle, uint8_t addr);

#define BQ27Z561_ADDR        0x55
#define REG_DEVICE_ID        0x02
#define REG_TEMP             0x06
#define REG_VOLT             0x08
#define REG_NOM_CAP          0x12
#define REG_REM_CAP          0x14
#define REG_SOC              0x2C
#define REG_CURR             0x58
#define REG_SOH              0x7A

static uint16_t fake_reg16[256];
static uint8_t fake_reg8[256];
static xy_sensor_bus_t last_bus;
static int read16_count;
static int read8_count;
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

    *value = fake_reg16[reg];
    read16_count++;
    return 0;
}

int xy_sensor_i2c_read_reg(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *value)
{
    TEST_ASSERT_NOT_NULL(bus);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, bus->type);
    TEST_ASSERT_NOT_NULL(value);

    *value = fake_reg8[reg];
    read8_count++;
    return 0;
}

int xy_sensor_i2c_write_reg16(xy_sensor_bus_t *bus, uint8_t reg, uint16_t value)
{
    (void)bus;
    (void)reg;
    (void)value;
    return -1;
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
    memset(fake_reg16, 0, sizeof(fake_reg16));
    memset(fake_reg8, 0, sizeof(fake_reg8));
    memset(&last_bus, 0, sizeof(last_bus));
    read16_count = 0;
    read8_count = 0;
    fake_tick = 1000;

    fake_reg16[REG_DEVICE_ID] = 0x0561;
    fake_reg16[REG_VOLT] = 3700;
    fake_reg16[REG_CURR] = (uint16_t)(int16_t)-123;
    fake_reg8[REG_SOC] = 66;
    fake_reg8[REG_SOH] = 97;
    fake_reg16[REG_TEMP] = 2981;
    fake_reg16[REG_NOM_CAP] = 3000;
    fake_reg16[REG_REM_CAP] = 1980;
}

void tearDown(void)
{
}

static xy_fuel_gauge_t *registered_bq27z561(void)
{
    xy_fuel_gauge_t *fg = xy_fuel_gauge_device_get("BQ27z561");
    if (!fg) {
        TEST_ASSERT_EQUAL(XY_FG_OK,
                          xy_fuel_gauge_bq27z561_register((void *)0x27561, 0));
        fg = xy_fuel_gauge_device_get("BQ27z561");
    }
    TEST_ASSERT_NOT_NULL(fg);
    return fg;
}

void test_bq27z561_registers_default_i2c_bus(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();

    TEST_ASSERT_NOT_NULL(fg);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, last_bus.type);
    TEST_ASSERT_EQUAL_PTR((void *)0x27561, last_bus.bus_handle);
    TEST_ASSERT_EQUAL_UINT8(BQ27Z561_ADDR, last_bus.address);
}

void test_bq27z561_init_reads_device_id(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE(fg->initialized);
    TEST_ASSERT_EQUAL_INT(1, read16_count);
}

void test_bq27z561_fetch_and_channel_get(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));

    fake_tick = 27561;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(27561, fg->latest.timestamp);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3700, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-123, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(66, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOH, &value));
    TEST_ASSERT_EQUAL_INT32(97, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT32(250, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_FULL_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(3000, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_REMAIN_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(1980, value);

    TEST_ASSERT_GREATER_OR_EQUAL(6, read16_count);
    TEST_ASSERT_GREATER_OR_EQUAL(2, read8_count);
}

void test_bq27z561_rejects_invalid_output_and_unknown_channel(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z561();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_INVALID_PARAM,
                      xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, NULL));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_get(fg, (xy_fuel_gauge_data_type_t)99, &value));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bq27z561_registers_default_i2c_bus);
    RUN_TEST(test_bq27z561_init_reads_device_id);
    RUN_TEST(test_bq27z561_fetch_and_channel_get);
    RUN_TEST(test_bq27z561_rejects_invalid_output_and_unknown_channel);
    return UNITY_END();
}
