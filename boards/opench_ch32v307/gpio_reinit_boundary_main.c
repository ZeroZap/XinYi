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
    const xy_hal_gpio_config_t in = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_UP,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_HIGH, 0};
    const xy_hal_uart_config_t u = {115200,
                                    XY_HAL_UART_WORDLEN_8B,
                                    XY_HAL_UART_STOPBITS_1,
                                    XY_HAL_UART_PARITY_NONE,
                                    XY_HAL_UART_FLOWCTRL_NONE,
                                    XY_HAL_UART_MODE_TX_RX};
    int ok = 1;
    uint32_t before, after;
    if (xy_hal_gpio_init(GPIOA, 9, &tx) || xy_hal_gpio_init(GPIOA, 10, &rx) ||
        xy_hal_uart_init(USART1, &u))
        for (;;) {
        }
    mark("OPENCH_GPIO_REINIT_BOUNDARY_BOOT_20261016_A\r\n");
    ok &= xy_hal_gpio_init(GPIOC, 4, &out) == XY_HAL_OK;
    before = GPIOC->CFGLR;
    ok &= xy_hal_gpio_init(GPIOC, 4, &in) == XY_HAL_ERROR_ALREADY_INIT;
    after = GPIOC->CFGLR;
    ok &= before == after;
    ok &= xy_hal_gpio_get_mode(GPIOC, 4) == XY_HAL_GPIO_MODE_OUTPUT;
    ok &= xy_hal_gpio_get_pull(GPIOC, 4) == XY_HAL_GPIO_PULL_NONE;
    mark(ok ? "OPENCH_GPIO_REINIT_REJECT_ATOMIC_OK_A\r\n"
            : "OPENCH_GPIO_REINIT_REJECT_ATOMIC_ERROR_A\r\n");
    ok &= xy_hal_gpio_set_mode(GPIOC, 4, XY_HAL_GPIO_MODE_INPUT) == XY_HAL_OK;
    ok &= xy_hal_gpio_set_pull(GPIOC, 4, XY_HAL_GPIO_PULL_UP) == XY_HAL_OK;
    ok &= xy_hal_gpio_get_mode(GPIOC, 4) == XY_HAL_GPIO_MODE_INPUT;
    ok &= xy_hal_gpio_get_pull(GPIOC, 4) == XY_HAL_GPIO_PULL_UP;
    ok &= xy_hal_gpio_deinit(GPIOC, 4) == XY_HAL_OK;
    ok &= xy_hal_gpio_init(GPIOC, 4, &out) == XY_HAL_OK;
    mark(ok ? "OPENCH_GPIO_RECONFIG_PATH_OK_A\r\n" : "OPENCH_GPIO_RECONFIG_PATH_ERROR_A\r\n");
    for (;;) {
    }
}
