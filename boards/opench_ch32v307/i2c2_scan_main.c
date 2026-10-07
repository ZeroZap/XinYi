#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_uart.h"

#include <stddef.h>
#include <stdint.h>

#include "ch32v30x.h"

static void uart_write(const char* text) {
    size_t len = 0U;
    while (text[len] != '\0')
        ++len;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, len, 1000U);
}

int main(void) {
    const xy_hal_gpio_config_t uart_tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                          XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t uart_rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                          XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_gpio_config_t i2c_pin = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP,
                                          XY_HAL_GPIO_OTYPE_OD, XY_HAL_GPIO_SPEED_HIGH, 0U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    const xy_hal_i2c_config_t i2c = {100000U, XY_HAL_I2C_ADDR_7BIT, XY_HAL_I2C_DUTY_2, 0U, 0U};
    uint16_t address;
    uint32_t found;

    if (xy_hal_gpio_init(GPIOA, 9U, &uart_tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &uart_rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK) {
        for (;;) {
        }
    }
    if (xy_hal_gpio_init(GPIOB, 10U, &i2c_pin) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 11U, &i2c_pin) != XY_HAL_OK ||
        xy_hal_i2c_init(I2C2, &i2c) != XY_HAL_OK) {
        uart_write("OPENCH_I2C2_INIT_ERROR\r\n");
        for (;;) {
        }
    }

    for (;;) {
        found = 0U;
        for (address = 0x08U; address <= 0x77U; ++address) {
            if (xy_hal_i2c_is_device_ready(I2C2, address, 1U, 5U) == XY_HAL_OK)
                ++found;
        }
        if (found == 0U)
            uart_write("OPENCH_I2C2_SCAN_OK COUNT=0\r\n");
        else
            uart_write("OPENCH_I2C2_SCAN_OK COUNT=NONZERO\r\n");
        for (volatile uint32_t delay = 0; delay < 1200000U; ++delay) {
            ;
        }
    }
}
