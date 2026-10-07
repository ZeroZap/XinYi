#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"

#include <stddef.h>
#include <stdint.h>

#include "ch32v30x.h"

static void uart_write(const char* text) {
    size_t len = 0U;
    while (text[len] != '\0') {
        ++len;
    }
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, len, 1000U);
}

int main(void) {
    xy_hal_uart_config_t config = {
        115200U,
        XY_HAL_UART_WORDLEN_8B,
        XY_HAL_UART_STOPBITS_1,
        XY_HAL_UART_PARITY_NONE,
        XY_HAL_UART_FLOWCTRL_NONE,
        XY_HAL_UART_MODE_TX_RX,
    };

    const xy_hal_gpio_config_t tx = {
        XY_HAL_GPIO_MODE_AF,
        XY_HAL_GPIO_PULL_NONE,
        XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_VERY_HIGH,
        0U,
    };
    const xy_hal_gpio_config_t rx = {
        XY_HAL_GPIO_MODE_INPUT,
        XY_HAL_GPIO_PULL_NONE,
        XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_LOW,
        0U,
    };

    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK) {
        for (;;) {
        }
    }

    if (xy_hal_uart_init(USART1, &config) != XY_HAL_OK) {
        for (;;) {
        }
    }

    for (;;) {
        uart_write("XINYI OPENCH CH32V307 UART1 READY\r\n");
        uart_write("PA9=UART1_TX PA10=UART1_RX WCHLINK=CH549F\r\n");
        uart_write("OPENCH_UART1_ALIVE\r\n");
        for (volatile uint32_t delay = 0; delay < 300000U; ++delay) {
            ;
        }
    }
}
