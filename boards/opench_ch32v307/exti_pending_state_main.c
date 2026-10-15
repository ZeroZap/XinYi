#include "ch32v30x.h"
#include "xy_hal_exti.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include <stddef.h>

static volatile uint32_t callback_count;
static void marker(const char* text) {
    size_t n = 0U;
    while (text[n] != '\0')
        ++n;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, n, 1000U);
}
static void callback(xy_hal_exti_line_t line, void* arg) {
    (void)arg;
    if (line == XY_HAL_EXTI_LINE_5)
        ++callback_count;
}
int main(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_exti_config_t config = {XY_HAL_EXTI_LINE_5, XY_HAL_EXTI_TRIGGER_RISING, 1U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    uint32_t before;
    int valid;
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK || xy_hal_exti_init() != XY_HAL_OK ||
        xy_hal_exti_map_gpio(GPIO_PortSourceGPIOA, 5U, XY_HAL_EXTI_LINE_5) != XY_HAL_OK ||
        xy_hal_exti_configure(&config) != XY_HAL_OK ||
        xy_hal_exti_register_callback(XY_HAL_EXTI_LINE_5, callback, NULL) != XY_HAL_OK)
        for (;;) {
        }
    marker("OPENCH_EXTI_PENDING_STATE_BOOT_20261014_A\r\n");
    valid = xy_hal_exti_clear_pending(XY_HAL_EXTI_LINE_5) == XY_HAL_OK &&
            !xy_hal_exti_get_pending(XY_HAL_EXTI_LINE_5);
    valid = valid && xy_hal_exti_set_pending(XY_HAL_EXTI_LINE_5) == XY_HAL_OK;
    marker(valid ? "OPENCH_EXTI_PENDING_SET_ISSUED_OK_A\r\n"
                 : "OPENCH_EXTI_PENDING_SET_ERROR_A\r\n");
    before = callback_count;
    valid = valid && xy_hal_exti_clear_pending(XY_HAL_EXTI_LINE_5) == XY_HAL_OK;
    for (volatile uint32_t delay = 0U; delay < 10000U; ++delay) {
    }
    valid = valid && !xy_hal_exti_get_pending(XY_HAL_EXTI_LINE_5) && callback_count == before;
    marker(valid ? "OPENCH_EXTI_PENDING_CLEAR_SUPPRESS_OK_A\r\n"
                 : "OPENCH_EXTI_PENDING_CLEAR_ERROR_A\r\n");
    valid = valid && xy_hal_exti_generate_software_interrupt(XY_HAL_EXTI_LINE_5) == XY_HAL_OK;
    for (volatile uint32_t delay = 0U; delay < 10000U; ++delay) {
    }
    valid = valid && callback_count == before + 1U;
    valid = valid && xy_hal_exti_unregister_callback(XY_HAL_EXTI_LINE_5) == XY_HAL_OK;
    valid = valid && xy_hal_exti_generate_software_interrupt(XY_HAL_EXTI_LINE_5) == XY_HAL_OK;
    for (volatile uint32_t delay = 0U; delay < 10000U; ++delay) {
    }
    marker(valid && callback_count == before + 1U ? "OPENCH_EXTI_UNREGISTER_SUPPRESS_OK_A\r\n"
                                                  : "OPENCH_EXTI_UNREGISTER_ERROR_A\r\n");
    for (;;) {
    }
}
