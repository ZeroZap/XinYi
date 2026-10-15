#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2s.h"
#include "xy_hal_uart.h"
#include <stddef.h>

static volatile uint32_t completions;

static void marker(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, 1000U);
}

static void callback(void* i2s, xy_hal_i2s_evt_t event, void* arg) {
    (void)i2s;
    (void)arg;
    if (event == XY_HAL_I2S_EVENT_TX_COMPLETE)
        ++completions;
}

static int run_rounds(void) {
    const uint16_t samples[] = {0x1234U, 0xABCDU, 0x55AAU, 0x0F0FU};
    uint32_t iteration;
    for (iteration = 0U; iteration < 64U; ++iteration) {
        uint32_t before = completions;
        if (xy_hal_i2s_send(SPI2, samples, 4U, 100U) != 4 || completions != before + 1U)
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
    const xy_hal_i2s_config_t i2s = {XY_HAL_I2S_MODE_MASTER_TX,
                                     XY_HAL_I2S_STANDARD_PHILIPS,
                                     XY_HAL_I2S_DATAFORMAT_16B,
                                     XY_HAL_I2S_CPOL_LOW,
                                     XY_HAL_I2S_MCLK_DISABLE,
                                     48000U,
                                     144000000U,
                                     XY_HAL_I2S_COMMUNICATION_TXONLY};
    int valid;

    SystemCoreClockUpdate();
    SysTick->CTLR = 0U;
    SysTick->CNT = 0U;
    SysTick->CMP = UINT64_MAX;
    SysTick->CTLR = (1U << 2U) | 1U;
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK)
        for (;;) {
        }
    marker("OPENCH_I2S2_SYSTICK_SOURCE_BOOT_20261014_A\r\n");
    if (xy_hal_i2s_init(SPI2, &i2s) != XY_HAL_OK ||
        xy_hal_i2s_register_callback(SPI2, callback, NULL) != XY_HAL_OK ||
        xy_hal_i2s_start(SPI2) != XY_HAL_OK) {
        marker("OPENCH_I2S2_SYSTICK_SOURCE_INIT_ERROR_A\r\n");
        for (;;) {
        }
    }
    valid = run_rounds();
    marker(valid ? "OPENCH_I2S2_SYSTICK_HCLK_64X_OK_A\r\n"
                 : "OPENCH_I2S2_SYSTICK_HCLK_ERROR_A\r\n");
    SysTick->CTLR = 0U;
    SysTick->CNT = 0U;
    SysTick->CMP = UINT64_MAX;
    SysTick->CTLR = 1U;
    valid = valid && run_rounds();
    marker(valid ? "OPENCH_I2S2_SYSTICK_HCLK8_64X_OK_A\r\n"
                 : "OPENCH_I2S2_SYSTICK_HCLK8_ERROR_A\r\n");
    for (;;) {
    }
}
