#include "xy_mux.h"
#include "xy_mux_gpio.h"
#include "xy_mux_i2c.h"
#include "xy_mux_spi.h"
#include "xy_mux_uart.h"

#include <string.h>

static int32_t g_gpio_level;
static xy_mux_gpio_config_t g_gpio_config;
static xy_mux_i2c_config_t g_i2c_config;
static xy_mux_spi_config_t g_spi_config;
static xy_mux_uart_config_t g_uart_config;
static uint8_t g_i2c_last_addr;
static uint32_t g_uart_timeout;

static int32_t example_gpio_ioctl(uint8_t channel, int cmd, void *arg)
{
    (void)channel;

    switch ((xy_mux_gpio_cmd_t)cmd) {
    case XY_MUX_GPIO_CMD_SET_CONFIG:
        if (!arg) {
            return XY_MUX_ERROR_INVALID_PARAM;
        }
        memcpy(&g_gpio_config, arg, sizeof(g_gpio_config));
        return XY_MUX_OK;
    case XY_MUX_GPIO_CMD_SET_LEVEL:
        if (!arg) {
            return XY_MUX_ERROR_INVALID_PARAM;
        }
        g_gpio_level = *(const uint8_t *)arg;
        return XY_MUX_OK;
    case XY_MUX_GPIO_CMD_GET_LEVEL:
        if (!arg) {
            return XY_MUX_ERROR_INVALID_PARAM;
        }
        *(uint8_t *)arg = (uint8_t)g_gpio_level;
        return XY_MUX_OK;
    case XY_MUX_GPIO_CMD_TOGGLE:
        g_gpio_level = (g_gpio_level == XY_MUX_GPIO_LOW) ? XY_MUX_GPIO_HIGH : XY_MUX_GPIO_LOW;
        return XY_MUX_OK;
    default:
        return XY_MUX_ERROR_NOT_SUPPORTED;
    }
}

static int32_t example_i2c_read(uint8_t channel, void *data, size_t len)
{
    (void)channel;
    if (!data || len == 0U) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    uint8_t *bytes = (uint8_t *)data;
    for (size_t i = 0; i < len; ++i) {
        bytes[i] = (uint8_t)(0xA0U + i);
    }
    return (int32_t)len;
}

static int32_t example_i2c_write(uint8_t channel, const void *data, size_t len)
{
    (void)channel;
    if (!data || len < 2U) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    const uint8_t *bytes = (const uint8_t *)data;
    g_i2c_last_addr = bytes[0];
    return (int32_t)len;
}

static int32_t example_i2c_ioctl(uint8_t channel, int cmd, void *arg)
{
    (void)channel;

    switch ((xy_mux_i2c_cmd_t)cmd) {
    case XY_MUX_I2C_CMD_SET_CONFIG:
        if (!arg) {
            return XY_MUX_ERROR_INVALID_PARAM;
        }
        memcpy(&g_i2c_config, arg, sizeof(g_i2c_config));
        return XY_MUX_OK;
    case XY_MUX_I2C_CMD_SCAN:
        if (!arg) {
            return XY_MUX_ERROR_INVALID_PARAM;
        }
        ((uint16_t *)arg)[0] = 0x50U;
        return 1;
    default:
        return XY_MUX_ERROR_NOT_SUPPORTED;
    }
}

static int32_t example_spi_read(uint8_t channel, void *data, size_t len)
{
    (void)channel;
    if (!data) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    memset(data, 0x5A, len);
    return (int32_t)len;
}

static int32_t example_spi_write(uint8_t channel, const void *data, size_t len)
{
    (void)channel;
    return data ? (int32_t)len : XY_MUX_ERROR_INVALID_PARAM;
}

static int32_t example_spi_ioctl(uint8_t channel, int cmd, void *arg)
{
    (void)channel;

    if ((xy_mux_spi_cmd_t)cmd == XY_MUX_SPI_CMD_SET_CONFIG) {
        if (!arg) {
            return XY_MUX_ERROR_INVALID_PARAM;
        }
        memcpy(&g_spi_config, arg, sizeof(g_spi_config));
        return XY_MUX_OK;
    }

    return XY_MUX_ERROR_NOT_SUPPORTED;
}

static int32_t example_uart_read(uint8_t channel, void *data, size_t len)
{
    (void)channel;
    static const char reply[] = "OK";
    if (!data) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    size_t copy_len = len < (sizeof(reply) - 1U) ? len : (sizeof(reply) - 1U);
    memcpy(data, reply, copy_len);
    return (int32_t)copy_len;
}

static int32_t example_uart_write(uint8_t channel, const void *data, size_t len)
{
    (void)channel;
    if (!data) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    if (len == 6U) {
        const uint8_t *bytes = (const uint8_t *)data;
        g_uart_timeout = (uint32_t)bytes[4] | ((uint32_t)bytes[5] << 8);
    }
    return (int32_t)len;
}

static int32_t example_uart_ioctl(uint8_t channel, int cmd, void *arg)
{
    (void)channel;

    switch ((xy_mux_uart_cmd_t)cmd) {
    case XY_MUX_UART_CMD_SET_CONFIG:
        if (!arg) {
            return XY_MUX_ERROR_INVALID_PARAM;
        }
        memcpy(&g_uart_config, arg, sizeof(g_uart_config));
        return XY_MUX_OK;
    case XY_MUX_UART_CMD_SET_TIMEOUT:
        if (!arg) {
            return XY_MUX_ERROR_INVALID_PARAM;
        }
        g_uart_timeout = *(const uint32_t *)arg;
        return XY_MUX_OK;
    default:
        return XY_MUX_ERROR_NOT_SUPPORTED;
    }
}

int main(void)
{
    uint8_t tx_buffer[128];
    uint8_t rx_buffer[128];
    xy_mux_manager_t mgr;

    if (xy_mux_init(&mgr, tx_buffer, rx_buffer, sizeof(tx_buffer)) != XY_MUX_OK) {
        return 1;
    }

    const xy_mux_ops_t gpio_ops = {.ioctl = example_gpio_ioctl};
    const xy_mux_ops_t i2c_ops = {
        .read = example_i2c_read,
        .write = example_i2c_write,
        .ioctl = example_i2c_ioctl,
    };
    const xy_mux_ops_t spi_ops = {
        .read = example_spi_read,
        .write = example_spi_write,
        .ioctl = example_spi_ioctl,
    };
    const xy_mux_ops_t uart_ops = {
        .read = example_uart_read,
        .write = example_uart_write,
        .ioctl = example_uart_ioctl,
    };

    if (xy_mux_gpio_register(&mgr, 0U, &gpio_ops, NULL) != XY_MUX_OK ||
        xy_mux_i2c_register(&mgr, 1U, &i2c_ops, NULL) != XY_MUX_OK ||
        xy_mux_spi_register(&mgr, 2U, &spi_ops, NULL) != XY_MUX_OK ||
        xy_mux_uart_register(&mgr, 3U, &uart_ops, NULL) != XY_MUX_OK) {
        return 2;
    }

    xy_mux_gpio_config_t gpio_cfg = {
        .dir = XY_MUX_GPIO_OUTPUT,
        .pull = XY_MUX_GPIO_LOW,
        .interrupt_enable = false,
    };
    if (xy_mux_gpio_config(&mgr, 0U, &gpio_cfg) != XY_MUX_OK ||
        xy_mux_gpio_write(&mgr, 0U, XY_MUX_GPIO_HIGH) != XY_MUX_OK ||
        xy_mux_gpio_read(&mgr, 0U) != XY_MUX_GPIO_HIGH ||
        xy_mux_gpio_toggle(&mgr, 0U) != XY_MUX_OK ||
        xy_mux_gpio_read(&mgr, 0U) != XY_MUX_GPIO_LOW) {
        return 3;
    }

    xy_mux_i2c_config_t i2c_cfg = {.speed = 400000U, .addr_bits = 7U};
    uint8_t i2c_tx[] = {0x50U, 0x01U, 0x02U};
    uint8_t i2c_rx[2] = {0U};
    uint16_t scan_addrs[2] = {0U};
    if (xy_mux_i2c_config(&mgr, 1U, &i2c_cfg) != XY_MUX_OK ||
        xy_mux_i2c_write(&mgr, 1U, 0x50U, i2c_tx, sizeof(i2c_tx)) !=
            (int32_t)(sizeof(i2c_tx) + 2U) ||
        xy_mux_i2c_read(&mgr, 1U, 0x50U, i2c_rx, sizeof(i2c_rx)) != (int32_t)sizeof(i2c_rx) ||
        xy_mux_i2c_scan(&mgr, 1U, scan_addrs, 2U) != XY_MUX_ERROR_NOT_SUPPORTED ||
        scan_addrs[0] != 0x50U || g_i2c_config.speed != 400000U || g_i2c_last_addr != 0x50U) {
        return 4;
    }

    xy_mux_spi_config_t spi_cfg = {.speed = 1000000U, .mode = XY_MUX_SPI_MODE0, .bits = 8U, .cs_pin = 5U};
    uint8_t spi_tx[] = {0x9FU, 0x00U};
    uint8_t spi_rx[sizeof(spi_tx)] = {0U};
    if (xy_mux_spi_config(&mgr, 2U, &spi_cfg) != XY_MUX_OK ||
        xy_mux_spi_write(&mgr, 2U, spi_tx, sizeof(spi_tx)) != (int32_t)sizeof(spi_tx) ||
        xy_mux_spi_read(&mgr, 2U, spi_rx, sizeof(spi_rx)) != (int32_t)sizeof(spi_rx) ||
        xy_mux_spi_transfer(&mgr, 2U, spi_tx, spi_rx, sizeof(spi_tx)) != (int32_t)sizeof(spi_tx) ||
        g_spi_config.cs_pin != 5U) {
        return 5;
    }

    xy_mux_uart_config_t uart_cfg = {
        .baudrate = 115200U,
        .data_bits = 8U,
        .stop_bits = 1U,
        .parity = 0U,
        .flow_control = 0U,
    };
    uint8_t uart_rx[4] = {0U};
    const uint8_t uart_tx[] = "AT";
    if (xy_mux_uart_config(&mgr, 3U, &uart_cfg) != XY_MUX_OK ||
        xy_mux_uart_write(&mgr, 3U, uart_tx, sizeof(uart_tx) - 1U, 100U) != (int32_t)(sizeof(uart_tx) - 1U) ||
        xy_mux_uart_read(&mgr, 3U, uart_rx, sizeof(uart_rx), 250U) != 2 ||
        memcmp(uart_rx, "OK", 2U) != 0 || g_uart_timeout != 250U ||
        g_uart_config.baudrate != 115200U) {
        return 6;
    }

    if (xy_mux_get_device_count(&mgr) != 4U || xy_mux_deinit(&mgr) != XY_MUX_OK) {
        return 7;
    }

    return 0;
}
