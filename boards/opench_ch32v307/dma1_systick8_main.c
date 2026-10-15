#include "ch32v30x.h"
#include "xy_hal_dma.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include <stddef.h>
#include <string.h>

static uint8_t source[256];
static uint8_t destination[256];

static void marker(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
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
    const xy_hal_dma_config_t dma = {
        XY_HAL_DMA_DIR_MEM_TO_MEM, XY_HAL_DMA_MODE_NORMAL, XY_HAL_DMA_PRIORITY_HIGH,
        XY_HAL_DMA_WIDTH_BYTE,     XY_HAL_DMA_WIDTH_BYTE,  XY_HAL_DMA_INCR_ENABLE,
        XY_HAL_DMA_INCR_ENABLE,
    };
    uint32_t iteration;
    size_t index;
    int valid = 1;

    SystemCoreClockUpdate();
    SysTick->CTLR = 0U;
    SysTick->CNT = 0U;
    SysTick->CMP = UINT64_MAX;
    SysTick->CTLR = 1U;
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK ||
        xy_hal_dma_init(DMA1_Channel1, &dma) != XY_HAL_OK)
        for (;;) {
        }
    marker("OPENCH_DMA1_SYSTICK8_BOOT_20261014_A\r\n");
    for (iteration = 0U; iteration < 128U && valid; ++iteration) {
        for (index = 0U; index < sizeof(source); ++index)
            source[index] = (uint8_t)(index ^ iteration ^ 0xA5U);
        memset(destination, 0, sizeof(destination));
        valid = xy_hal_dma_start(DMA1_Channel1, (uint32_t)source, (uint32_t)destination,
                                 sizeof(source)) == XY_HAL_OK &&
                xy_hal_dma_poll_complete(DMA1_Channel1, 100U) == XY_HAL_OK &&
                xy_hal_dma_get_counter(DMA1_Channel1) == 0 &&
                memcmp(source, destination, sizeof(source)) == 0;
    }
    marker(valid ? "OPENCH_DMA1_SYSTICK8_MEMCPY_128X_OK_A\r\n"
                 : "OPENCH_DMA1_SYSTICK8_MEMCPY_ERROR_A\r\n");
    for (;;) {
    }
}
