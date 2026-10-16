#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include <stddef.h>
static void m(const char* s) {
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
    const xy_hal_gpio_config_t in = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0};
    const xy_hal_uart_config_t u = {115200,
                                    XY_HAL_UART_WORDLEN_8B,
                                    XY_HAL_UART_STOPBITS_1,
                                    XY_HAL_UART_PARITY_NONE,
                                    XY_HAL_UART_FLOWCTRL_NONE,
                                    XY_HAL_UART_MODE_TX_RX};
    int v = 1, r;
    if (xy_hal_gpio_init(GPIOA, 9, &tx) || xy_hal_gpio_init(GPIOA, 10, &rx) ||
        xy_hal_uart_init(USART1, &u))
        for (;;) {
        }
    m("OPENCH_GPIO_BATCH_READ_BOOT_20261015_A\r\n");
    v &= xy_hal_gpio_read_batch(GPIOC, 0) == XY_HAL_ERROR_INVALID_PARAM;
    v &= xy_hal_gpio_read_batch(GPIOC, 0x0030) == XY_HAL_ERROR_NOT_INIT;
    v &= xy_hal_gpio_init(GPIOC, 4, &in) == XY_HAL_OK;
    v &= xy_hal_gpio_read_batch(GPIOC, 0x0030) == XY_HAL_ERROR_NOT_INIT;
    m(v ? "OPENCH_GPIO_BATCH_READ_REJECT_OK_A\r\n" : "OPENCH_GPIO_BATCH_READ_REJECT_ERROR_A\r\n");
    v &= xy_hal_gpio_init(GPIOC, 5, &in) == XY_HAL_OK;
    r = xy_hal_gpio_read_batch(GPIOC, 0x0030);
    v &= r >= 0;
    v &= (r & ~0x0030) == 0;
    v &= xy_hal_gpio_deinit(GPIOC, 5) == XY_HAL_OK;
    v &= xy_hal_gpio_read_batch(GPIOC, 0x0030) == XY_HAL_ERROR_NOT_INIT;
    m(v ? "OPENCH_GPIO_BATCH_READ_CONFIG_OK_A\r\n" : "OPENCH_GPIO_BATCH_READ_CONFIG_ERROR_A\r\n");
    for (;;) {
    }
}
