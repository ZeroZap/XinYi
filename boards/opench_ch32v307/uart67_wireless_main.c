#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"

#include <stddef.h>
#include <stdint.h>

#include "ch32v30x.h"

static void console_write(const char* text) {
    size_t len = 0U;
    while (text[len] != '\0')
        ++len;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, len, 1000U);
}

static int init_uart_pins(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_FullRemap_USART6, DISABLE);
    GPIO_PinRemapConfig(GPIO_FullRemap_USART7, DISABLE);

    return xy_hal_gpio_init(GPIOA, 9U, &tx) == XY_HAL_OK &&
           xy_hal_gpio_init(GPIOA, 10U, &rx) == XY_HAL_OK &&
           xy_hal_gpio_init(GPIOC, 0U, &tx) == XY_HAL_OK &&
           xy_hal_gpio_init(GPIOC, 1U, &rx) == XY_HAL_OK &&
           xy_hal_gpio_init(GPIOC, 2U, &tx) == XY_HAL_OK &&
           xy_hal_gpio_init(GPIOC, 3U, &rx) == XY_HAL_OK;
}

int main(void) {
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};

    if (!init_uart_pins() || xy_hal_uart_init(USART1, &uart) != XY_HAL_OK) {
        for (;;) {
        }
    }
    if (xy_hal_uart_init(UART6, &uart) != XY_HAL_OK) {
        console_write("OPENCH_UART6_ESP_INIT_ERROR\r\n");
        for (;;) {
        }
    }
    if (xy_hal_uart_init(UART7, &uart) != XY_HAL_OK) {
        console_write("OPENCH_UART7_BLE_INIT_ERROR\r\n");
        for (;;) {
        }
    }

    for (;;) {
        console_write("OPENCH_UART6_ESP_READY PC0_TX PC1_RX\r\n");
        console_write("OPENCH_UART7_BLE_READY PC2_TX PC3_RX\r\n");
        for (volatile uint32_t delay = 0; delay < 300000U; ++delay) {
            ;
        }
    }
}
