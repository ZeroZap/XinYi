#include "unity.h"
#include "xy_fg_bq27z746.h"
#include "xy_sensor_device.h"

#include <stdint.h>
#include <string.h>

#define REG_CTRL      0x00
#define REG_TEMP      0x06
#define REG_VOLT      0x08
#define REG_FLAGS     0x0A
#define REG_REM_CAP   0x14
#define REG_SOC       0x2C
#define REG_CURR      0x58
#define REG_SOH       0x7A
#define REG_CYCLE_CNT 0x2A
#define REG_FULL_CAP  0x12

static uint16_t fake_regs[256];
static xy_sensor_bus_t last_bus;
static int read_count;
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

int xy_sensor_i2c_read(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_NOT_NULL(bus);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, bus->type);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT16(2, len);

    uint16_t value = fake_regs[reg];
    data[0] = (uint8_t)(value & 0xFFu);
    data[1] = (uint8_t)(value >> 8);
    read_count++;
    return 0;
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

int xy_sensor_i2c_read_reg16(xy_sensor_bus_t *bus, uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    int ret = xy_sensor_i2c_read(bus, reg, data, sizeof(data));
    if (ret == 0) {
        *value = ((uint16_t)data[1] << 8) | data[0];
    }
    return ret;
}

int xy_sensor_i2c_write_reg16(xy_sensor_bus_t *bus, uint8_t reg, uint16_t value)
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
    read_count = 0;
    fake_tick = 1000;

    fake_regs[REG_CTRL] = 0x0746;
    fake_regs[REG_FLAGS] = 0x0009;
    fake_regs[REG_VOLT] = 3811;
    fake_regs[REG_CURR] = (uint16_t)(int16_t)-321;
    fake_regs[REG_SOC] = 67;
    fake_regs[REG_SOH] = 94;
    fake_regs[REG_TEMP] = 2981;
    fake_regs[REG_FULL_CAP] = 4800;
    fake_regs[REG_REM_CAP] = 3200;
    fake_regs[REG_CYCLE_CNT] = 123;
}

void tearDown(void)
{
}

static xy_fuel_gauge_t *registered_bq27z746(void)
{
    xy_fuel_gauge_t *fg = xy_fuel_gauge_device_get("BQ27Z746");
    if (!fg) {
        TEST_ASSERT_EQUAL(XY_FG_OK,
                          xy_fuel_gauge_bq27z746_register((void *)0x1234, 0));
        fg = xy_fuel_gauge_device_get("BQ27Z746");
    }
    TEST_ASSERT_NOT_NULL(fg);
    return fg;
}

void test_bq27z746_registers_default_i2c_bus(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();

    TEST_ASSERT_NOT_NULL(fg);
    TEST_ASSERT_EQUAL(XY_SENSOR_BUS_I2C, last_bus.type);
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, last_bus.bus_handle);
    TEST_ASSERT_EQUAL_UINT8(BQ27Z746_ADDR, last_bus.address);
}

void test_bq27z746_init_fetch_and_channel_get(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_TRUE(fg->initialized);
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_charging(fg));
    TEST_ASSERT_TRUE(xy_fuel_gauge_bq27z746_is_full(fg));
    TEST_ASSERT_EQUAL_UINT16(0x0009, xy_fuel_gauge_bq27z746_get_flags(fg));

    fake_tick = 4242;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_fetch(fg));
    TEST_ASSERT_EQUAL_UINT32(4242, fg->latest.timestamp);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3811, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-321, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(67, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT32(250, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_FULL_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(4800, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_REMAIN_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(3200, value);

    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_get(fg, XY_FG_DATA_CYCLE_COUNT, &value));
    TEST_ASSERT_EQUAL_INT32(123, value);
    TEST_ASSERT_GREATER_THAN(0, read_count);
}

void test_bq27z746_rejects_unsupported_channel(void)
{
    xy_fuel_gauge_t *fg = registered_bq27z746();
    int32_t value = 0;

    fg->initialized = false;
    TEST_ASSERT_EQUAL(XY_FG_OK, xy_fuel_gauge_init(fg));
    TEST_ASSERT_EQUAL(XY_FG_ERROR_NOT_SUPPORTED,
                      xy_fuel_gauge_get(fg, XY_FG_DATA_TIME_TO_FULL, &value));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bq27z746_registers_default_i2c_bus);
    RUN_TEST(test_bq27z746_init_fetch_and_channel_get);
    RUN_TEST(test_bq27z746_rejects_unsupported_channel);
    return UNITY_END();
}
