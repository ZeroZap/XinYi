#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"
#include <stddef.h>
#include <string.h>

static void console_write(const char* s) {
    size_t n = 0U;
    while (s[n] != '\0')
        ++n;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)s, n, 1000U);
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
    xy_hal_sys_clock_info_t clocks;
    uint32_t uid[3];
    uint64_t timestamp_start;
    char name[20];

    SystemCoreClockUpdate();
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK || xy_hal_sys_init() != XY_HAL_OK ||
        xy_hal_sys_get_clock_info(&clocks) != XY_HAL_OK ||
        xy_hal_sys_get_chip_name(name, sizeof(name)) != XY_HAL_OK ||
        xy_hal_sys_get_unique_id(uid) != XY_HAL_OK) {
        for (;;) {
        }
    }

    console_write("OPENCH_SYS_BOOT_ID_20261009_A\r\n");
    timestamp_start = xy_hal_sys_get_timestamp_us();
    for (volatile uint32_t warmup = 0U; warmup < 1000U; ++warmup) {
    }
    while (xy_hal_sys_get_timestamp_us() == timestamp_start) {
    }

    for (;;) {
        int32_t chip = xy_hal_sys_get_chip_id();
        uint32_t mask = 0U;
        mask |= (chip == (int32_t)DBGMCU_GetCHIPID() && chip != 0) ? 1U << 0 : 0U;
        mask |= strcmp(name, "CH32V307VCT6") == 0 ? 1U << 1 : 0U;
        mask |= (clocks.sysclk == SystemCoreClock && clocks.hclk != 0U && clocks.pclk1 != 0U &&
                 clocks.pclk2 != 0U && xy_hal_sys_get_cpu_freq() == SystemCoreClock)
                    ? 1U << 2
                    : 0U;
        mask |= xy_hal_sys_get_clk_source(XY_HAL_SYS_CLKDOMAIN_SYS) == XY_HAL_SYS_CLKSRC_HSI
                    ? 1U << 3
                    : 0U;
        mask |= (xy_hal_sys_get_flash_size() == 288U * 1024U &&
                 xy_hal_sys_get_ram_size() == 32U * 1024U)
                    ? 1U << 4
                    : 0U;
        mask |= (uid[0] != 0U && uid[0] != UINT32_MAX) ? 1U << 5 : 0U;
        mask |= xy_hal_sys_get_timestamp_us() > timestamp_start ? 1U << 6 : 0U;
        if (mask == 0x7FU)
            console_write("OPENCH_SYS_ID_CLOCK_MEMORY_UID_TIME_OK_A\r\n");
        else {
            static const char hex[] = "0123456789ABCDEF";
            char error[] = "OPENCH_SYS_ERROR_MASK_00_A\r\n";
            error[22] = hex[(mask >> 4) & 0xFU];
            error[23] = hex[mask & 0xFU];
            console_write(error);
        }
        for (volatile uint32_t delay = 0U; delay < 300000U; ++delay) {
        }
    }
}
