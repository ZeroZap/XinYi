/**
 * @file sensor_bus.c
 * @brief Sensor bus access (I2C/SPI) on top of the HAL.
 *
 * The declarations live in `xy_sensor_device.h` and the fuel-gauge drivers call them, so this
 * file is a contract with callers rather than a sample: it is compiled into `sensor_component`
 * and every transfer either reaches the HAL or fails closed.
 *
 * Two things this file deliberately does not do:
 *
 *  - It never publishes a success it did not get. Every HAL status other than `XY_HAL_OK`
 *    becomes `-1`, which is the value callers compare against.
 *  - It never drives SPI chip select. `xy_sensor_bus_t.chip_select` is a pin number with no
 *    port attached and cannot be turned into a current `xy_hal_gpio_*` call; the SPI handle
 *    owns its own CS (`xy_hal_spi_set_cs`) and the device owner configures it. Toggling a pin
 *    derived from this field would be worse than leaving the handle in charge.
 */

#include "xy_sensor.h"
#include "xy_hal_i2c.h"
#include "xy_hal_spi.h"
#include "xy_log.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/*: Whole-call budget for one transfer, in milliseconds. The HAL reads `0` as "wait forever",
 *: which turns a wedged bus into a hang inside a driver instead of a failed read. */
#define XY_SENSOR_BUS_TIMEOUT_MS 100U

/*: The HAL takes 7-bit unshifted addresses and applies the shift itself; anything wider is a
 *: caller passing a shifted address. Address 0 is the general-call address, never a sensor. */
#define XY_SENSOR_BUS_I2C_ADDR_MAX 0x7FU

static bool sensor_bus_i2c_ready(const xy_sensor_bus_t *bus)
{
    return bus != NULL && bus->type == XY_SENSOR_BUS_I2C && bus->bus_handle != NULL &&
           bus->address != 0U && bus->address <= XY_SENSOR_BUS_I2C_ADDR_MAX;
}

static bool sensor_bus_spi_ready(const xy_sensor_bus_t *bus)
{
    return bus != NULL && bus->type == XY_SENSOR_BUS_SPI && bus->bus_handle != NULL;
}

static int sensor_bus_status(xy_hal_error_t status)
{
    return status == XY_HAL_OK ? 0 : -1;
}

/* ==================== I2C bus ==================== */

int xy_sensor_i2c_read(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (!sensor_bus_i2c_ready(bus) || data == NULL || len == 0U) {
        return -1;
    }

    return sensor_bus_status(xy_hal_i2c_mem_read(bus->bus_handle, bus->address, reg, data, len,
                                                 XY_SENSOR_BUS_TIMEOUT_MS));
}

int xy_sensor_i2c_write(xy_sensor_bus_t *bus, uint8_t reg, const uint8_t *data, uint16_t len)
{
    if (!sensor_bus_i2c_ready(bus) || data == NULL || len == 0U) {
        return -1;
    }

    return sensor_bus_status(xy_hal_i2c_mem_write(bus->bus_handle, bus->address, reg, data, len,
                                                  XY_SENSOR_BUS_TIMEOUT_MS));
}

int xy_sensor_i2c_read_reg(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *value)
{
    return xy_sensor_i2c_read(bus, reg, value, 1U);
}

int xy_sensor_i2c_write_reg(xy_sensor_bus_t *bus, uint8_t reg, uint8_t value)
{
    return xy_sensor_i2c_write(bus, reg, &value, 1U);
}

int xy_sensor_i2c_read_reg16(xy_sensor_bus_t *bus, uint8_t reg, uint16_t *value)
{
    uint8_t buf[2];
    int ret;

    if (value == NULL) {
        return -1;
    }

    ret = xy_sensor_i2c_read(bus, reg, buf, sizeof(buf));
    if (ret == 0) {
        /* SMBus word order: least significant byte first. Reversing this silently scrambles
         * every 16-bit register value a caller reports. */
        *value = (uint16_t)(((uint16_t)buf[1] << 8) | buf[0]);
    }
    return ret;
}

int xy_sensor_i2c_write_reg16(xy_sensor_bus_t *bus, uint8_t reg, uint16_t value)
{
    uint8_t buf[2];

    buf[0] = (uint8_t)(value & 0xFFU);
    buf[1] = (uint8_t)((value >> 8) & 0xFFU);
    return xy_sensor_i2c_write(bus, reg, buf, sizeof(buf));
}

/* ==================== SPI bus ==================== */

int xy_sensor_spi_read(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    if (!sensor_bus_spi_ready(bus) || data == NULL || len == 0U) {
        return -1;
    }

    if (xy_hal_spi_transmit(bus->bus_handle, &reg, 1U, XY_SENSOR_BUS_TIMEOUT_MS) != XY_HAL_OK) {
        return -1;
    }

    return sensor_bus_status(
        xy_hal_spi_receive(bus->bus_handle, data, len, XY_SENSOR_BUS_TIMEOUT_MS));
}

int xy_sensor_spi_write(xy_sensor_bus_t *bus, uint8_t reg, const uint8_t *data, uint16_t len)
{
    if (!sensor_bus_spi_ready(bus) || data == NULL || len == 0U) {
        return -1;
    }

    if (xy_hal_spi_transmit(bus->bus_handle, &reg, 1U, XY_SENSOR_BUS_TIMEOUT_MS) != XY_HAL_OK) {
        return -1;
    }

    return sensor_bus_status(
        xy_hal_spi_transmit(bus->bus_handle, data, len, XY_SENSOR_BUS_TIMEOUT_MS));
}

/* ==================== Utilities ==================== */

int xy_sensor_check_device_id(xy_sensor_bus_t *bus, uint8_t id_reg, uint8_t expected_id)
{
    uint8_t id = 0U;
    int ret;

    if (bus == NULL) {
        return -1;
    }

    if (bus->type == XY_SENSOR_BUS_I2C) {
        ret = xy_sensor_i2c_read_reg(bus, id_reg, &id);
    } else if (bus->type == XY_SENSOR_BUS_SPI) {
        ret = xy_sensor_spi_read(bus, id_reg, &id, 1U);
    } else {
        return -1;
    }

    if (ret != 0) {
        xy_log_e("Failed to read device ID\n");
        return -1;
    }

    if (id != expected_id) {
        xy_log_e("Device ID mismatch: expected=0x%02X, got=0x%02X\n", expected_id, id);
        return -1;
    }

    xy_log_i("Device ID verified: 0x%02X\n", id);
    return 0;
}

void xy_sensor_bus_config_i2c(xy_sensor_bus_t *bus, void *handle, uint8_t address)
{
    if (bus == NULL) {
        return;
    }

    bus->type = XY_SENSOR_BUS_I2C;
    bus->bus_handle = handle;
    bus->address = address;
    bus->chip_select = 0U;
}

void xy_sensor_bus_config_spi(xy_sensor_bus_t *bus, void *handle, uint8_t cs_pin)
{
    if (bus == NULL) {
        return;
    }

    bus->type = XY_SENSOR_BUS_SPI;
    bus->bus_handle = handle;
    bus->address = 0U;
    bus->chip_select = cs_pin;
}
