#include "ch32v30x.h"
#include "xy_hal_exti.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include <stddef.h>

static volatile uint32_t callback_count;
static void console_write(const char* s) {
    size_t n = 0U;
    while (s[n] != '\0')
        ++n;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)s, n, 1000U);
}
static void callback(xy_hal_exti_line_t line, void* arg) {
    (void)arg;
    if (line == XY_HAL_EXTI_LINE_0)
        ++callback_count;
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
    const xy_hal_exti_config_t exti = {XY_HAL_EXTI_LINE_0, XY_HAL_EXTI_TRIGGER_RISING, 1U};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK)
        for (;;) {
        }
    console_write("OPENCH_EXTI0_BOOT_ID_20261008_B\r\n");
    if (xy_hal_exti_init() != XY_HAL_OK ||
        xy_hal_exti_map_gpio(GPIO_PortSourceGPIOA, GPIO_PinSource0, XY_HAL_EXTI_LINE_0) !=
            XY_HAL_OK ||
        xy_hal_exti_register_callback(XY_HAL_EXTI_LINE_0, callback, NULL) != XY_HAL_OK ||
        xy_hal_exti_configure(&exti) != XY_HAL_OK) {
        console_write("OPENCH_EXTI0_SETUP_ERROR_B\r\n");
        for (;;) {
        }
    }
    console_write("OPENCH_EXTI0_READY_B\r\n");
    for (;;) {
        uint32_t before = callback_count;
        if (xy_hal_exti_generate_software_interrupt(XY_HAL_EXTI_LINE_0) != XY_HAL_OK) {
            console_write("OPENCH_EXTI0_GENERATE_ERROR_B\r\n");
        } else {
            for (volatile uint32_t wait = 0U; wait < 10000U && callback_count == before; ++wait) {
            }
            if (callback_count == before + 1U)
                console_write("OPENCH_EXTI0_CALLBACK_OK_B\r\n");
            else
                console_write("OPENCH_EXTI0_CALLBACK_ERROR_B\r\n");
        }
        for (volatile uint32_t delay = 0U; delay < 3000000U; ++delay) {
        }
    }
}
