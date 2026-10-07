#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_pwm.h"
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
    const xy_hal_gpio_config_t af = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    const xy_hal_pwm_config_t pwm = {1000U,
                                     5000U,
                                     XY_HAL_PWM_POLARITY_HIGH,
                                     XY_HAL_PWM_MODE_EDGE_ALIGN,
                                     XY_HAL_PWM_WAVE_SQUARE,
                                     0U,
                                     0U,
                                     0U};
    if (xy_hal_gpio_init(GPIOA, 9U, &af) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 6U, &af) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK ||
        xy_hal_pwm_init(TIM3, XY_HAL_PWM_CHANNEL_1, &pwm) != XY_HAL_OK ||
        xy_hal_pwm_start(TIM3, XY_HAL_PWM_CHANNEL_1) != XY_HAL_OK)
        for (;;) {
        }
    for (;;) {
        if (xy_hal_pwm_get_frequency(TIM3, XY_HAL_PWM_CHANNEL_1) == 1000 &&
            xy_hal_pwm_get_duty_cycle(TIM3, XY_HAL_PWM_CHANNEL_1) == 5000)
            console_write("OPENCH_TIM3_CH1_PA6_PWM_OK\r\n");
        else
            console_write("OPENCH_TIM3_CH1_PA6_PWM_ERROR\r\n");
        for (volatile uint32_t d = 0; d < 300000U; ++d) {
        }
    }
}
