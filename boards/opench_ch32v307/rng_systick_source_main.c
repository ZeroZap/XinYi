#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_rng.h"
#include "xy_hal_uart.h"
#include <stddef.h>

static void marker(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, 1000U);
}

static int run_source(uint32_t source_bit) {
    uint32_t buffer[16];
    uint32_t iteration;
    SysTick->CTLR = 0U;
    SysTick->CNT = 0U;
    SysTick->CMP = UINT64_MAX;
    SysTick->CTLR = source_bit | 1U;
    for (iteration = 0U; iteration < 32U; ++iteration) {
        size_t index;
        int changed = 0;
        if (xy_hal_rng_get_buffer(RNG, buffer, 16U, 100U) != XY_HAL_OK)
            return 0;
        for (index = 1U; index < 16U; ++index)
            if (buffer[index] != buffer[0])
                changed = 1;
        if (!changed)
            return 0;
    }
    return 1;
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
    const xy_hal_rng_config_t rng = {1U, 0U};
    int valid;

    SystemCoreClockUpdate();
    SysTick->CTLR = 0U;
    SysTick->CNT = 0U;
    SysTick->CMP = UINT64_MAX;
    SysTick->CTLR = (1U << 2U) | 1U;
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK || xy_hal_rng_init(RNG, &rng) != XY_HAL_OK)
        for (;;) {
        }
    marker("OPENCH_RNG_SYSTICK_SOURCE_BOOT_20261014_A\r\n");
    valid = run_source(1U << 2U);
    marker(valid ? "OPENCH_RNG_SYSTICK_HCLK_32X_OK_A\r\n" : "OPENCH_RNG_SYSTICK_HCLK_ERROR_A\r\n");
    valid = valid && run_source(0U);
    marker(valid ? "OPENCH_RNG_SYSTICK_HCLK8_32X_OK_A\r\n"
                 : "OPENCH_RNG_SYSTICK_HCLK8_ERROR_A\r\n");
    for (;;) {
    }
}
