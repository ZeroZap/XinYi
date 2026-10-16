#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include <stddef.h>
static void mark(const char* s) {
    size_t n = 0;
    while (s[n])
        n++;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)s, n, 1000);
}
int main(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0};
    const xy_hal_uart_config_t u = {115200,
                                    XY_HAL_UART_WORDLEN_8B,
                                    XY_HAL_UART_STOPBITS_1,
                                    XY_HAL_UART_PARITY_NONE,
                                    XY_HAL_UART_FLOWCTRL_NONE,
                                    XY_HAL_UART_MODE_TX_RX};
    int ok = 1;
    if (xy_hal_gpio_init(GPIOA, 9, &tx) || xy_hal_gpio_init(GPIOA, 10, &rx))
        for (;;) {
        }
    ok &= xy_hal_uart_available(USART1) == XY_HAL_ERROR_NOT_INIT;
    ok &= xy_hal_uart_flush(USART1) == XY_HAL_ERROR_NOT_INIT;
    ok &= xy_hal_uart_error(USART1) == XY_HAL_ERROR_NOT_INIT;
    if (xy_hal_uart_init(USART1, &u))
        for (;;) {
        }
    mark("OPENCH_UART_AUX_STATE_BOOT_20261016_A\r\n");
    ok &= xy_hal_uart_available(USART1) >= 0;
    ok &= xy_hal_uart_flush(USART1) == XY_HAL_OK;
    ok &= xy_hal_uart_error(USART1) == XY_HAL_OK;
    ok &= xy_hal_uart_available(UART6) == XY_HAL_ERROR_NOT_INIT;
    ok &= xy_hal_uart_flush(UART6) == XY_HAL_ERROR_NOT_INIT;
    ok &= xy_hal_uart_error(UART6) == XY_HAL_ERROR_NOT_INIT;
    mark(ok ? "OPENCH_UART_AUX_STATE_OK_A\r\n" : "OPENCH_UART_AUX_STATE_ERROR_A\r\n");
    ok &= xy_hal_uart_deinit(USART1) == XY_HAL_OK;
    ok &= xy_hal_uart_available(USART1) == XY_HAL_ERROR_NOT_INIT;
    ok &= xy_hal_uart_flush(USART1) == XY_HAL_ERROR_NOT_INIT;
    ok &= xy_hal_uart_error(USART1) == XY_HAL_ERROR_NOT_INIT;
    ok &= xy_hal_uart_init(USART1, &u) == XY_HAL_OK;
    mark(ok ? "OPENCH_UART_AUX_DEINIT_CLEAR_OK_A\r\n" : "OPENCH_UART_AUX_DEINIT_CLEAR_ERROR_A\r\n");
    for (;;) {
    }
}
