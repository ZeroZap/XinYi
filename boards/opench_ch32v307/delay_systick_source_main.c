#include "ch32v30x.h"
#include "xy_hal_delay.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include <stddef.h>

static void marker(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, 1000U);
}

static void run_source(uint32_t source_bit, const char* marker_text) {
    uint32_t iteration;
    SysTick->CTLR = source_bit;
    for (iteration = 0U; iteration < 8U; ++iteration) {
        marker(marker_text);
        xy_hal_delay_ms(250U);
    }
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

    SystemCoreClockUpdate();
    SysTick->CTLR = 0U;
    SysTick->CNT = 0U;
    SysTick->CMP = UINT64_MAX;
    SysTick->CTLR = (1U << 5U) | 1U;
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK)
        for (;;) {
        }
    marker("OPENCH_DELAY_SYSTICK_SOURCE_BOOT_20261014_A\r\n");
    run_source(1U << 2U, "OPENCH_DELAY_HCLK_INTERVAL_A\r\n");
    marker("OPENCH_DELAY_HCLK_8X_DONE_A\r\n");
    run_source(0U, "OPENCH_DELAY_HCLK8_INTERVAL_A\r\n");
    marker("OPENCH_DELAY_HCLK8_8X_DONE_A\r\n");
    for (;;) {
    }
}
