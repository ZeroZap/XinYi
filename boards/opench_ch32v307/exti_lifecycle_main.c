#include "ch32v30x.h"
#include "xy_hal_exti.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include <stddef.h>
static volatile uint32_t count;
static void mark(const char* s) {
    size_t n = 0;
    while (s[n])
        n++;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)s, n, 1000);
}
static void cb(xy_hal_exti_line_t l, void* a) {
    (void)a;
    if (l == XY_HAL_EXTI_LINE_5)
        ++count;
}
int main(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0};
    const xy_hal_exti_config_t c = {XY_HAL_EXTI_LINE_5, XY_HAL_EXTI_TRIGGER_RISING, 1};
    const xy_hal_uart_config_t u = {115200,
                                    XY_HAL_UART_WORDLEN_8B,
                                    XY_HAL_UART_STOPBITS_1,
                                    XY_HAL_UART_PARITY_NONE,
                                    XY_HAL_UART_FLOWCTRL_NONE,
                                    XY_HAL_UART_MODE_TX_RX};
    int v = 1;
    if (xy_hal_gpio_init(GPIOA, 9, &tx) || xy_hal_gpio_init(GPIOA, 10, &rx) ||
        xy_hal_uart_init(USART1, &u) || xy_hal_exti_init())
        for (;;) {
        }
    mark("OPENCH_EXTI_LIFECYCLE_BOOT_20261015_A\r\n");
    v &= xy_hal_exti_map_gpio(GPIO_PortSourceGPIOA, 5, XY_HAL_EXTI_LINE_5) == XY_HAL_OK;
    v &= xy_hal_exti_configure(&c) == XY_HAL_OK;
    v &= xy_hal_exti_register_callback(XY_HAL_EXTI_LINE_5, cb, NULL) == XY_HAL_OK;
    v &= xy_hal_exti_generate_software_interrupt(XY_HAL_EXTI_LINE_5) == XY_HAL_OK;
    for (volatile uint32_t d = 0; d < 10000; d++) {
    }
    v &= count == 1;
    v &= xy_hal_exti_deinit() == XY_HAL_OK;
    v &= xy_hal_exti_get_pending(XY_HAL_EXTI_LINE_5) == 0;
    mark(v ? "OPENCH_EXTI_DEINIT_CLEAN_OK_A\r\n" : "OPENCH_EXTI_DEINIT_CLEAN_ERROR_A\r\n");
    v &= xy_hal_exti_init() == XY_HAL_OK;
    v &= xy_hal_exti_map_gpio(GPIO_PortSourceGPIOA, 5, XY_HAL_EXTI_LINE_5) == XY_HAL_OK;
    v &= xy_hal_exti_configure(&c) == XY_HAL_OK;
    v &= xy_hal_exti_register_callback(XY_HAL_EXTI_LINE_5, cb, NULL) == XY_HAL_OK;
    mark(v ? "OPENCH_EXTI_REINIT_READY_OK_A\r\n" : "OPENCH_EXTI_REINIT_ERROR_A\r\n");
    v &= xy_hal_exti_generate_software_interrupt(XY_HAL_EXTI_LINE_5) == XY_HAL_OK;
    for (volatile uint32_t d = 0; d < 10000; d++) {
    }
    mark(v && count == 2 ? "OPENCH_EXTI_REINIT_CALLBACK_OK_A\r\n"
                         : "OPENCH_EXTI_REINIT_CALLBACK_ERROR_A\r\n");
    for (;;) {
    }
}
