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
    const xy_hal_gpio_config_t out = {XY_HAL_GPIO_MODE_OUTPUT, XY_HAL_GPIO_PULL_NONE,
                                      XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0};
    const xy_hal_uart_config_t u = {115200,
                                    XY_HAL_UART_WORDLEN_8B,
                                    XY_HAL_UART_STOPBITS_1,
                                    XY_HAL_UART_PARITY_NONE,
                                    XY_HAL_UART_FLOWCTRL_NONE,
                                    XY_HAL_UART_MODE_TX_RX};
    int ok = 1;
    uint32_t cfg, outdr;
    if (xy_hal_gpio_init(GPIOA, 9, &tx) || xy_hal_gpio_init(GPIOA, 10, &rx) ||
        xy_hal_uart_init(USART1, &u))
        for (;;) {
        }
    mark("OPENCH_GPIO_SETTER_VALIDATION_BOOT_20261016_A\r\n");
    ok &= xy_hal_gpio_set_mode(GPIOC, 4, (xy_hal_gpio_mode_t)99) == XY_HAL_ERROR_INVALID_PARAM;
    ok &= xy_hal_gpio_set_pull(GPIOC, 4, (xy_hal_gpio_pull_t)99) == XY_HAL_ERROR_INVALID_PARAM;
    ok &= xy_hal_gpio_set_otype(GPIOC, 4, (xy_hal_gpio_otype_t)99) == XY_HAL_ERROR_INVALID_PARAM;
    ok &= xy_hal_gpio_set_speed(GPIOC, 4, (xy_hal_gpio_speed_t)99) == XY_HAL_ERROR_INVALID_PARAM;
    ok &= xy_hal_gpio_set_alternate(GPIOC, 4, 1) == XY_HAL_ERROR_INVALID_PARAM;
    ok &= xy_hal_gpio_set_mode(GPIOC, 4, XY_HAL_GPIO_MODE_OUTPUT) == XY_HAL_ERROR_NOT_INIT;
    ok &= xy_hal_gpio_set_alternate(GPIOC, 4, 0) == XY_HAL_ERROR_NOT_INIT;
    mark(ok ? "OPENCH_GPIO_SETTER_INVALID_PRIORITY_OK_A\r\n"
            : "OPENCH_GPIO_SETTER_INVALID_PRIORITY_ERROR_A\r\n");
    ok &= xy_hal_gpio_init(GPIOC, 4, &out) == XY_HAL_OK;
    cfg = GPIOC->CFGLR;
    outdr = GPIOC->OUTDR;
    ok &= xy_hal_gpio_set_mode(GPIOC, 4, (xy_hal_gpio_mode_t)99) == XY_HAL_ERROR_INVALID_PARAM;
    ok &= xy_hal_gpio_set_pull(GPIOC, 4, (xy_hal_gpio_pull_t)99) == XY_HAL_ERROR_INVALID_PARAM;
    ok &= xy_hal_gpio_set_otype(GPIOC, 4, (xy_hal_gpio_otype_t)99) == XY_HAL_ERROR_INVALID_PARAM;
    ok &= xy_hal_gpio_set_speed(GPIOC, 4, (xy_hal_gpio_speed_t)99) == XY_HAL_ERROR_INVALID_PARAM;
    ok &= xy_hal_gpio_set_alternate(GPIOC, 4, 1) == XY_HAL_ERROR_INVALID_PARAM;
    ok &= GPIOC->CFGLR == cfg && GPIOC->OUTDR == outdr;
    ok &= xy_hal_gpio_get_mode(GPIOC, 4) == XY_HAL_GPIO_MODE_OUTPUT;
    ok &= xy_hal_gpio_get_pull(GPIOC, 4) == XY_HAL_GPIO_PULL_NONE;
    mark(ok ? "OPENCH_GPIO_SETTER_FAILURE_ATOMIC_OK_A\r\n"
            : "OPENCH_GPIO_SETTER_FAILURE_ATOMIC_ERROR_A\r\n");
    for (;;) {
    }
}
