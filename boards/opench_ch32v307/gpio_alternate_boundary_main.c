#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"

#include <stddef.h>

static void marker(const char* text) {
    size_t n = 0;

    while (text[n] != '\0') {
        n++;
    }
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, n, 1000);
}

int main(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0};
    const xy_hal_gpio_config_t out = {XY_HAL_GPIO_MODE_OUTPUT, XY_HAL_GPIO_PULL_NONE,
                                      XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0};
    xy_hal_gpio_config_t bad = out;
    const xy_hal_uart_config_t u = {115200,
                                    XY_HAL_UART_WORDLEN_8B,
                                    XY_HAL_UART_STOPBITS_1,
                                    XY_HAL_UART_PARITY_NONE,
                                    XY_HAL_UART_FLOWCTRL_NONE,
                                    XY_HAL_UART_MODE_TX_RX};
    int v = 1;
    uint32_t before, after;
    if (xy_hal_gpio_init(GPIOA, 9, &tx) || xy_hal_gpio_init(GPIOA, 10, &rx) ||
        xy_hal_uart_init(USART1, &u))
        for (;;) {
        }
    marker("OPENCH_GPIO_ALTERNATE_BOUNDARY_BOOT_20261015_A\r\n");
    bad.alternate = 1;
    before = GPIOC->CFGLR;
    v &= xy_hal_gpio_init(GPIOC, 4, &bad) == XY_HAL_ERROR_INVALID_PARAM;
    after = GPIOC->CFGLR;
    v &= before == after;
    v &= xy_hal_gpio_get_mode(GPIOC, 4) == XY_HAL_ERROR_NOT_INIT;
    marker(v ? "OPENCH_GPIO_ALTERNATE_INIT_REJECT_OK_A\r\n"
             : "OPENCH_GPIO_ALTERNATE_INIT_REJECT_ERROR_A\r\n");
    v &= xy_hal_gpio_init(GPIOC, 4, &out) == XY_HAL_OK;
    before = GPIOC->CFGLR;
    v &= xy_hal_gpio_set_alternate(GPIOC, 4, 1) == XY_HAL_ERROR_INVALID_PARAM;
    after = GPIOC->CFGLR;
    v &= before == after;
    v &= xy_hal_gpio_get_alternate(GPIOC, 4) == 0;
    v &= xy_hal_gpio_set_alternate(GPIOC, 4, 0) == XY_HAL_OK;
    marker(v ? "OPENCH_GPIO_ALTERNATE_SET_REJECT_OK_A\r\n"
             : "OPENCH_GPIO_ALTERNATE_SET_REJECT_ERROR_A\r\n");
    for (;;) {
    }
}
