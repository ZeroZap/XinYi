#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include "xy_hal_wdg.h"

#include <stddef.h>
#include <stdint.h>

static void console_write(const char* text) {
    size_t length = 0U;

    while (text[length] != '\0') {
        ++length;
    }
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, 1000U);
}

int main(void) {
    const xy_hal_gpio_config_t uart_tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                          XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t uart_rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                          XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    const xy_hal_iwdg_config_t watchdog = {IWDG_Prescaler_32, 1249U, 1000U};

    if (xy_hal_gpio_init(GPIOA, 9U, &uart_tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &uart_rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK ||
        xy_hal_iwdg_init(IWDG, &watchdog) != XY_HAL_OK || xy_hal_iwdg_start(IWDG) != XY_HAL_OK) {
        for (;;) {
        }
    }

    for (;;) {
        console_write("OPENCH_IWDG_FEED_OK\r\n");
        (void)xy_hal_iwdg_feed(IWDG);
        for (volatile uint32_t delay = 0U; delay < 300000U; ++delay) {
        }
    }
}