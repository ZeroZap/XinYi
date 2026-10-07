#include "xy_hal_adc.h"
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

int main(void) {
    const xy_hal_gpio_config_t uart_tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                          XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t uart_rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                          XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_gpio_config_t analog = {XY_HAL_GPIO_MODE_ANALOG, XY_HAL_GPIO_PULL_NONE,
                                         XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    const xy_hal_adc_config_t adc = {
        XY_HAL_ADC_RESOLUTION_12B,
        XY_HAL_ADC_DATAALIGN_RIGHT,
        XY_HAL_ADC_SCAN_DISABLE,
        XY_HAL_ADC_CONTINUOUS_DISABLE,
        XY_HAL_ADC_TRIGGER_SOFTWARE,
        8U,
        ADC_SampleTime_239Cycles5,
        0U,
        0U,
        0U,
    };

    if (xy_hal_gpio_init(GPIOA, 9U, &uart_tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &uart_rx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 0U, &analog) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK || xy_hal_adc_init(ADC1, &adc) != XY_HAL_OK) {
        for (;;) {
        }
    }

    for (;;) {
        int32_t value = xy_hal_adc_read(ADC1, ADC_Channel_0, 100U);
        if (value >= 0 && value <= 4095)
            console_write("OPENCH_ADC1_CH0_PA0_READ_OK\r\n");
        else
            console_write("OPENCH_ADC1_CH0_PA0_READ_ERROR\r\n");
        for (volatile uint32_t delay = 0; delay < 300000U; ++delay) {
            ;
        }
    }
}
