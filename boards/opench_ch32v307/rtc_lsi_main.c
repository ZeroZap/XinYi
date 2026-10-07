#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_rtc.h"
#include "xy_hal_uart.h"
#include <stddef.h>

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
    uint32_t backup;

    SystemCoreClockUpdate();
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK) {
        for (;;) {
        }
    }
    console_write("OPENCH_RTC_UART_READY\r\n");
    if (xy_hal_rtc_init(RTC) != XY_HAL_OK) {
        console_write("OPENCH_RTC_INIT_ERROR\r\n");
        for (;;) {
        }
    }
    console_write("OPENCH_RTC_LSI_READY\r\n");
    if (xy_hal_rtc_set_timestamp(RTC, 1000) != XY_HAL_OK ||
        xy_hal_rtc_backup_write(0U, 0x51A7C0DEU) != XY_HAL_OK ||
        xy_hal_rtc_backup_read(0U, &backup) != XY_HAL_OK || backup != 0x51A7C0DEU) {
        console_write("OPENCH_RTC_STATE_ERROR\r\n");
        for (;;) {
        }
    }
    console_write("OPENCH_RTC_BACKUP_OK\r\n");

    for (;;) {
        int64_t before = xy_hal_rtc_get_timestamp(RTC);
        for (volatile uint32_t delay = 0U; delay < 10000000U; ++delay) {
        }
        int64_t after = xy_hal_rtc_get_timestamp(RTC);
        if (before >= 1000 && after > before)
            console_write("OPENCH_RTC_LSI_COUNTER_BACKUP_OK\r\n");
        else
            console_write("OPENCH_RTC_COUNTER_ERROR\r\n");
    }
}
