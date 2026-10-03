#include "xy_hal_uart.h"

#include <stddef.h>
#include <stdint.h>

#include "ch32v30x.h"

static void uart_write(const char *text)
{
    size_t len = 0U;
    while (text[len] != '\0') {
        ++len;
    }
    (void)xy_hal_uart_send(USART1, (const uint8_t *)text, len, 1000U);
}

int main(void)
{
    xy_hal_uart_config_t config = {
        115200U,
        XY_HAL_UART_WORDLEN_8B,
        XY_HAL_UART_STOPBITS_1,
        XY_HAL_UART_PARITY_NONE,
        XY_HAL_UART_FLOWCTRL_NONE,
        XY_HAL_UART_MODE_TX_RX,
    };

    /* Board startup boundary: configure the CH32V30x UART1 pins before the
     * reusable UART HAL is initialized. */
    GPIO_InitTypeDef gpio = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    if (xy_hal_uart_init(USART1, &config) != XY_HAL_OK) {
        for (;;) {
        }
    }

    uart_write("XINYI OPENCH CH32V307 UART1 READY\r\n");
    uart_write("PA9=UART1_TX PA10=UART1_RX WCHLINK=CH549F\r\n");
    for (;;) {
        uart_write("OPENCH_UART1_ALIVE\r\n");
        for (volatile uint32_t delay = 0; delay < 1200000U; ++delay) {
        }
    }
}
