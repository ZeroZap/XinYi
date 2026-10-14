#include "ch32v30x.h"
#include "xy_hal_adc.h"
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

static void settle(void) {
    for (volatile uint32_t delay = 0U; delay < 50000U; ++delay) {
    }
}

int main(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_gpio_config_t analog = {XY_HAL_GPIO_MODE_ANALOG, XY_HAL_GPIO_PULL_NONE,
                                         XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    const xy_hal_dac_config_t dac = {XY_HAL_DAC_RESOLUTION_12B, XY_HAL_DAC_DATAALIGN_RIGHT,
                                     XY_HAL_DAC_TRIGGER_SOFTWARE, XY_HAL_DAC_WAVE_NONE, 1U};
    const xy_hal_adc_config_t adc = {XY_HAL_ADC_RESOLUTION_12B,
                                     XY_HAL_ADC_DATAALIGN_RIGHT,
                                     XY_HAL_ADC_SCAN_DISABLE,
                                     XY_HAL_ADC_CONTINUOUS_DISABLE,
                                     XY_HAL_ADC_TRIGGER_SOFTWARE,
                                     8U,
                                     ADC_SampleTime_239Cycles5,
                                     0U,
                                     0U,
                                     0U};
    uint32_t iteration;
    int valid = 1;

    SystemCoreClockUpdate();
    SysTick->CTLR = 0U;
    SysTick->CNT = 0U;
    SysTick->CMP = UINT64_MAX;
    SysTick->CTLR = 1U;
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 4U, &analog) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK || xy_hal_dac_init(DAC, &dac) != XY_HAL_OK ||
        xy_hal_dac_start_output(DAC, XY_HAL_DAC_CHANNEL_1) != XY_HAL_OK ||
        xy_hal_adc_init(ADC1, &adc) != XY_HAL_OK) {
        for (;;) {
        }
    }
    marker("OPENCH_ADC1_SYSTICK8_BOOT_20261014_A\r\n");
    for (iteration = 0U; iteration < 32U && valid; ++iteration) {
        int32_t low;
        int32_t high;
        valid = xy_hal_dac_set_value(DAC, XY_HAL_DAC_CHANNEL_1, 512U, XY_HAL_DAC_DATAALIGN_RIGHT) ==
                    XY_HAL_OK &&
                xy_hal_dac_software_trigger(DAC, XY_HAL_DAC_CHANNEL_1) == XY_HAL_OK;
        settle();
        low = xy_hal_adc_read(ADC1, ADC_Channel_4, 100U);
        valid = valid &&
                xy_hal_dac_set_value(DAC, XY_HAL_DAC_CHANNEL_1, 3072U,
                                     XY_HAL_DAC_DATAALIGN_RIGHT) == XY_HAL_OK &&
                xy_hal_dac_software_trigger(DAC, XY_HAL_DAC_CHANNEL_1) == XY_HAL_OK;
        settle();
        high = xy_hal_adc_read(ADC1, ADC_Channel_4, 100U);
        valid = valid && low >= 100 && low <= 1200 && high >= 2400 && high <= 3800 &&
                high - low >= 1800;
    }
    marker(valid ? "OPENCH_ADC1_SYSTICK8_DAC_LOOPBACK_32X_OK_A\r\n"
                 : "OPENCH_ADC1_SYSTICK8_DAC_LOOPBACK_ERROR_A\r\n");
    for (;;) {
    }
}
