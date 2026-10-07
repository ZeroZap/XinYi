#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_rng.h"
#include "xy_hal_uart.h"
#include <stddef.h>

static void console_write(const char* s) {
    size_t n = 0U;
    while (s[n] != '\0')
        ++n;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)s, n, 1000U);
}

int main(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    const xy_hal_rng_config_t rng = {1U, 0U};

    SystemCoreClockUpdate();
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK || xy_hal_rng_init(RNG, &rng) != XY_HAL_OK) {
        for (;;) {
        }
    }

    for (;;) {
        uint32_t first = 0U;
        uint32_t second = 0U;
        xy_hal_error_t first_status;
        xy_hal_error_t second_status;
        do {
            first_status = xy_hal_rng_get_random_nb(RNG, &first);
        } while (first_status == XY_HAL_ERROR_BUSY);
        do {
            second_status = xy_hal_rng_get_random_nb(RNG, &second);
        } while (second_status == XY_HAL_ERROR_BUSY);
        if (first_status == XY_HAL_OK && second_status == XY_HAL_OK && first != second)
            console_write("OPENCH_RNG_VARIATION_OK\r\n");
        else
            console_write("OPENCH_RNG_VARIATION_ERROR\r\n");
        for (volatile uint32_t delay = 0U; delay < 300000U; ++delay) {
        }
    }
}
