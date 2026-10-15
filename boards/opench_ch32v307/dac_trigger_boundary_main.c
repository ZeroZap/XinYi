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

static void marker_hex(const char* prefix, uint32_t value) {
    char text[32];
    static const char digits[] = "0123456789ABCDEF";
    size_t index = 0U;
    while (prefix[index] != '\0') {
        text[index] = prefix[index];
        ++index;
    }
    text[index++] = '0';
    text[index++] = 'x';
    for (int shift = 28; shift >= 0; shift -= 4)
        text[index++] = digits[(value >> shift) & 0xFU];
    text[index++] = '\r';
    text[index++] = '\n';
    text[index] = '\0';
    marker(text);
}

static int init_probe(xy_hal_dac_trigger_src_t trigger, xy_hal_error_t expected) {
    const xy_hal_dac_config_t config = {XY_HAL_DAC_RESOLUTION_12B, XY_HAL_DAC_DATAALIGN_RIGHT,
                                        trigger, XY_HAL_DAC_WAVE_NONE, 1U};
    xy_hal_error_t result = xy_hal_dac_init(DAC, &config);
    if (result == XY_HAL_OK)
        (void)xy_hal_dac_deinit(DAC);
    return result == expected;
}

int main(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_dac_config_t timer = {XY_HAL_DAC_RESOLUTION_12B, XY_HAL_DAC_DATAALIGN_RIGHT,
                                       XY_HAL_DAC_TRIGGER_TIMER, XY_HAL_DAC_WAVE_NONE, 1U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    int valid;

    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK || xy_hal_dac_init(DAC, &timer) != XY_HAL_OK)
        for (;;) {
        }
    marker("OPENCH_DAC_TRIGGER_BOUNDARY_BOOT_20261014_A\r\n");
    valid = (DAC->CTLR & 0x3CU) == DAC_Trigger_T6_TRGO;
    marker_hex("OPENCH_DAC_TIMER_CTLR_", DAC->CTLR);
    valid = valid && xy_hal_dac_enable_channel(DAC, XY_HAL_DAC_CHANNEL_1) == XY_HAL_OK;
    valid = valid &&
            xy_hal_dac_software_trigger(DAC, XY_HAL_DAC_CHANNEL_1) == XY_HAL_ERROR_NOT_SUPPORTED;
    marker(valid ? "OPENCH_DAC_TIMER_TRIGGER_T6_REGISTER_OK_A\r\n"
                 : "OPENCH_DAC_TIMER_TRIGGER_ERROR_A\r\n");
    valid = valid && init_probe(XY_HAL_DAC_TRIGGER_EXTI, XY_HAL_ERROR_INVALID_PARAM);
    valid = valid && init_probe((xy_hal_dac_trigger_src_t)99, XY_HAL_ERROR_INVALID_PARAM);
    marker(valid ? "OPENCH_DAC_TRIGGER_UNSUPPORTED_REJECTED_OK_A\r\n"
                 : "OPENCH_DAC_TRIGGER_UNSUPPORTED_ERROR_A\r\n");
    for (;;) {
    }
}
