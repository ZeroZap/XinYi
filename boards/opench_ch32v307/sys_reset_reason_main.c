#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"
#include <stddef.h>

#define RESET_COOKIE 0xA529U

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
    uint32_t reason;

    SystemCoreClockUpdate();
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK || xy_hal_sys_init() != XY_HAL_OK) {
        for (;;) {
        }
    }
    console_write("OPENCH_SYS_RESET_BOOT_ID_20261009_A\r\n");
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);
    reason = xy_hal_sys_get_reset_reason();
    if (BKP_ReadBackupRegister(BKP_DR20) == RESET_COOKIE) {
        console_write("OPENCH_SYS_RESET_COOKIE_SEEN_A\r\n");
        BKP_WriteBackupRegister(BKP_DR20, 0U);
        if ((reason & XY_HAL_SYS_RESET_REASON_SOFTWARE) != 0U) {
            console_write("OPENCH_SYS_SOFTWARE_RESET_REASON_OK_A\r\n");
            (void)xy_hal_sys_clear_reset_reason();
            if (xy_hal_sys_get_reset_reason() == XY_HAL_SYS_RESET_REASON_NONE)
                console_write("OPENCH_SYS_RESET_REASON_CLEAR_OK_A\r\n");
            else
                console_write("OPENCH_SYS_RESET_REASON_CLEAR_ERROR_A\r\n");
        } else {
            console_write("OPENCH_SYS_SOFTWARE_RESET_REASON_ERROR_A\r\n");
        }
    } else {
        BKP_WriteBackupRegister(BKP_DR20, RESET_COOKIE);
        console_write("OPENCH_SYS_SOFTWARE_RESET_ARMED_A\r\n");
        (void)xy_hal_sys_software_reset();
        console_write("OPENCH_SYS_SOFTWARE_RESET_RETURNED_ERROR_A\r\n");
    }
    for (;;) {
    }
}
