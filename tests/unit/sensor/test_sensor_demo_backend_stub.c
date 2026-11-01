/**
 * SPDX-License-Identifier: MIT
 * @file    test_sensor_demo_backend_stub.c
 * @brief   Backend stubs for the CTest target that compiles the sensor sample.
 *
 * `components/sensor/examples/sensor_demo.c` supplies the three HAL entry points the SHT30 path
 * actually calls (`xy_hal_i2c_master_transmit`, `xy_hal_i2c_master_receive`, `xy_hal_delay_ms`)
 * because it asserts on them. It links the real `xy_i2c_device_*` helpers, and that translation
 * unit also contains the SPI/UART/GPIO helpers and the registry call - none of which the sample
 * reaches. They are stubbed here rather than linked so the sample stays readable and the sample's
 * backend remains entirely local.
 *
 * Prototypes come from the HAL headers, so these cannot drift from them: a signature change breaks
 * this build instead of a link step nobody reads. Returning NOT_SUPPORTED (rather than OK) means a
 * driver that quietly switched to one of these paths would fail its assertions instead of
 * succeeding against a rubber stamp.
 */

#include <stddef.h>
#include <stdint.h>

#include "xy_device.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_spi.h"
#include "xy_hal_uart.h"

int xy_device_registry_register(xy_device_t *dev)
{
    (void)dev;
    return 0;
}

xy_hal_error_t xy_hal_i2c_mem_read(void *i2c, uint16_t dev_addr, uint16_t reg_addr,
                                   uint8_t *data, size_t len, uint32_t timeout)
{
    (void)i2c;
    (void)dev_addr;
    (void)reg_addr;
    (void)data;
    (void)len;
    (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_i2c_mem_write(void *i2c, uint16_t dev_addr, uint16_t reg_addr,
                                    const uint8_t *data, size_t len, uint32_t timeout)
{
    (void)i2c;
    (void)dev_addr;
    (void)reg_addr;
    (void)data;
    (void)len;
    (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_spi_transmit(void *spi, const uint8_t *data, size_t len, uint32_t timeout)
{
    (void)spi;
    (void)data;
    (void)len;
    (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_spi_receive(void *spi, uint8_t *data, size_t len, uint32_t timeout)
{
    (void)spi;
    (void)data;
    (void)len;
    (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_spi_transmit_receive(void *spi, const uint8_t *tx_data, uint8_t *rx_data,
                                           size_t len, uint32_t timeout)
{
    (void)spi;
    (void)tx_data;
    (void)rx_data;
    (void)len;
    (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_uart_send(void *uart, const uint8_t *data, size_t len, uint32_t timeout)
{
    (void)uart;
    (void)data;
    (void)len;
    (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_uart_recv(void *uart, uint8_t *data, size_t len, uint32_t timeout)
{
    (void)uart;
    (void)data;
    (void)len;
    (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    (void)port;
    (void)pin;
    (void)config;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value)
{
    (void)port;
    (void)pin;
    (void)value;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin)
{
    (void)port;
    (void)pin;
    return 0;
}

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_port_t port, uint8_t pin)
{
    (void)port;
    (void)pin;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
