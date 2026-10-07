#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_timer.h"
#include "xy_hal_uart.h"
#include <stddef.h>
#include <stdint.h>
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
    const xy_hal_timer_config_t timer = {143U, 49999U, XY_HAL_TIMER_COUNT_UP, XY_HAL_TIMER_CKDIV_1,
                                         1U};
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK ||
        xy_hal_timer_init(TIM2, &timer) != XY_HAL_OK || xy_hal_timer_start(TIM2) != XY_HAL_OK)
        for (;;) {
        }
    for (;;) {
        int before = xy_hal_timer_get_counter(TIM2);
        for (volatile uint32_t d = 0; d < 10000U; ++d) {
        }
        int after = xy_hal_timer_get_counter(TIM2);
        if (before >= 0 && after >= 0 && before != after)
            console_write("OPENCH_TIM2_COUNTER_RUNNING_OK\r\n");
        else
            console_write("OPENCH_TIM2_COUNTER_RUNNING_ERROR\r\n");
        for (volatile uint32_t d = 0; d < 300000U; ++d) {
        }
    }
}
