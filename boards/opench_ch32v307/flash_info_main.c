#include "ch32v30x.h"
#include "xy_hal_flash.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"

#include <stddef.h>

static void console_write(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0') {
        ++length;
    }
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, 1000U);
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
    xy_hal_flash_info_t info;
    xy_hal_flash_sector_info_t last_page;

    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK || xy_hal_flash_init(NULL) != XY_HAL_OK ||
        xy_hal_flash_get_info(NULL, &info) != XY_HAL_OK ||
        xy_hal_flash_get_sector_info(NULL, 71U, &last_page) != XY_HAL_OK) {
        for (;;) {
        }
    }

    for (;;) {
        if (info.flash_size == 288U * 1024U && info.sector_count == 72U &&
            info.page_size == 4096U && info.write_alignment == 4U &&
            last_page.start_addr == 0x08047000U && last_page.size == 4096U &&
            xy_hal_flash_is_valid_address(NULL, 0x08000000U) == 1 &&
            xy_hal_flash_is_valid_address(NULL, 0x08047FFFU) == 1 &&
            xy_hal_flash_is_valid_address(NULL, 0x08048000U) == 0)
            console_write("OPENCH_FLASH_INFO_BOUNDARY_OK\r\n");
        else
            console_write("OPENCH_FLASH_INFO_BOUNDARY_ERROR\r\n");
        for (volatile uint32_t delay = 0U; delay < 300000U; ++delay) {
        }
    }
}