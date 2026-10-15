#include "ch32v30x.h"
#include "xy_hal_dac.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include <stddef.h>

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
    const xy_hal_dac_config_t dac = {XY_HAL_DAC_RESOLUTION_12B, XY_HAL_DAC_DATAALIGN_RIGHT,
                                     XY_HAL_DAC_TRIGGER_SOFTWARE, XY_HAL_DAC_WAVE_NONE, 1U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    uint32_t triangle_before;
    uint32_t triangle_after;
    uint32_t noise_before;
    uint32_t noise_after;
    int valid;

    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK || xy_hal_dac_init(DAC, &dac) != XY_HAL_OK)
        for (;;) {
        }
    marker("OPENCH_DAC_WAVE_REGISTER_BOOT_20261014_A\r\n");
    triangle_before = DAC->CTLR;
    valid = xy_hal_dac_generate_triangle(DAC, XY_HAL_DAC_CHANNEL_1, 255U) == XY_HAL_OK;
    triangle_after = DAC->CTLR;
    valid = valid && (triangle_after & 0x0F80U) == 0x0780U;
    valid = valid && xy_hal_dac_generate_triangle(DAC, XY_HAL_DAC_CHANNEL_1, 256U) ==
                         XY_HAL_ERROR_INVALID_PARAM;
    marker(valid ? "OPENCH_DAC_TRIANGLE_255_REGISTER_OK_A\r\n"
                 : "OPENCH_DAC_TRIANGLE_REGISTER_ERROR_A\r\n");
    noise_before = DAC->CTLR;
    valid = valid && xy_hal_dac_generate_noise(DAC, XY_HAL_DAC_CHANNEL_2, 5U) == XY_HAL_OK;
    noise_after = DAC->CTLR;
    valid = valid && (noise_after & 0x0F400000U) == 0x05400000U;
    valid = valid && triangle_before != triangle_after && noise_before != noise_after;
    marker(valid ? "OPENCH_DAC_NOISE_CH2_MASK5_REGISTER_OK_A\r\n"
                 : "OPENCH_DAC_NOISE_REGISTER_ERROR_A\r\n");
    for (;;) {
    }
}
