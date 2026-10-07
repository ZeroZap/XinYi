#include "xy_hal_dma.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"

#include <stddef.h>
#include <stdint.h>

#include "ch32v30x.h"

static uint8_t uart6_tx[] = "OPENCH_UART6_DMA_TX\r\n";
static uint8_t uart7_tx[] = "OPENCH_UART7_DMA_TX\r\n";

static void console_write(const char* text) {
    size_t len = 0U;
    while (text[len] != '\0')
        ++len;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, len, 1000U);
}

static int init_pins(void) {
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
    const xy_hal_dma_config_t tx_dma = {
        XY_HAL_DMA_DIR_MEM_TO_PERIPH, XY_HAL_DMA_MODE_NORMAL, XY_HAL_DMA_PRIORITY_HIGH,
        XY_HAL_DMA_WIDTH_BYTE,        XY_HAL_DMA_WIDTH_BYTE,  XY_HAL_DMA_INCR_DISABLE,
        XY_HAL_DMA_INCR_ENABLE,
    };

    if (!init_pins() || xy_hal_uart_init(USART1, &uart) != XY_HAL_OK ||
        xy_hal_uart_init(UART6, &uart) != XY_HAL_OK ||
        xy_hal_uart_init(UART7, &uart) != XY_HAL_OK ||
        xy_hal_dma_init(DMA2_Channel6, &tx_dma) != XY_HAL_OK ||
        xy_hal_dma_init(DMA2_Channel8, &tx_dma) != XY_HAL_OK) {
        for (;;) {
        }
    }
    USART_DMACmd(UART6, USART_DMAReq_Tx, ENABLE);
    USART_DMACmd(UART7, USART_DMAReq_Tx, ENABLE);

    for (;;) {
        if (xy_hal_dma_start(DMA2_Channel6, (uint32_t)uart6_tx, (uint32_t)&UART6->DATAR,
                             sizeof(uart6_tx) - 1U) == XY_HAL_OK &&
            xy_hal_dma_poll_complete(DMA2_Channel6, 100U) == XY_HAL_OK) {
            console_write("OPENCH_UART6_DMA2_CH6_TX_OK\r\n");
        } else {
            console_write("OPENCH_UART6_DMA2_CH6_TX_ERROR\r\n");
        }
        if (xy_hal_dma_start(DMA2_Channel8, (uint32_t)uart7_tx, (uint32_t)&UART7->DATAR,
                             sizeof(uart7_tx) - 1U) == XY_HAL_OK &&
            xy_hal_dma_poll_complete(DMA2_Channel8, 100U) == XY_HAL_OK) {
            console_write("OPENCH_UART7_DMA2_CH8_TX_OK\r\n");
        } else {
            console_write("OPENCH_UART7_DMA2_CH8_TX_ERROR\r\n");
        }
        for (volatile uint32_t delay = 0; delay < 300000U; ++delay) {
            ;
        }
    }
}
