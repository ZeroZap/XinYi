#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"
#include <stddef.h>

static void marker(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, 1000U);
}

static int validate_source(uint32_t source_bit) {
    uint64_t raw_start;
    uint64_t raw_elapsed;
    uint64_t us_start;
    uint64_t us_elapsed;
    uint32_t ms_start;
    uint32_t ms_elapsed;
    uint32_t raw_target = source_bit != 0U ? SystemCoreClock / 50U : SystemCoreClock / 400U;

    SysTick->CTLR = 0U;
    SysTick->CNT = 0U;
    SysTick->CMP = UINT64_MAX;
    SysTick->CTLR = source_bit | 1U;
    raw_start = SysTick->CNT;
    us_start = xy_hal_sys_get_timestamp_us();
    ms_start = xy_hal_sys_get_tick_count();
    while (SysTick->CNT - raw_start < raw_target) {
    }
    raw_elapsed = SysTick->CNT - raw_start;
    us_elapsed = xy_hal_sys_get_timestamp_us() - us_start;
    ms_elapsed = xy_hal_sys_get_tick_count() - ms_start;
    return raw_elapsed >= raw_target && us_elapsed >= 19000U && us_elapsed <= 21000U &&
           ms_elapsed >= 19U && ms_elapsed <= 21U;
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
    int valid;

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
    marker("OPENCH_SYS_SYSTICK_SOURCE_BOOT_20261014_A\r\n");
    valid = validate_source(1U << 2U);
    marker(valid ? "OPENCH_SYS_SYSTICK_HCLK_TIME_OK_A\r\n"
                 : "OPENCH_SYS_SYSTICK_HCLK_TIME_ERROR_A\r\n");
    valid = valid && validate_source(0U);
    marker(valid ? "OPENCH_SYS_SYSTICK_HCLK8_TIME_OK_A\r\n"
                 : "OPENCH_SYS_SYSTICK_HCLK8_TIME_ERROR_A\r\n");
    for (;;) {
    }
}
