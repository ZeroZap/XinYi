#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2s.h"
#include "xy_hal_uart.h"
#include <stddef.h>

static volatile uint32_t completion_count;
static void console_write(const char* s) {
    size_t n = 0U;
    while (s[n] != '\0')
        ++n;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)s, n, 1000U);
}
static void callback(void* i2s, xy_hal_i2s_evt_t event, void* arg) {
    (void)i2s;
    (void)arg;
    if (event == XY_HAL_I2S_EVENT_TX_COMPLETE)
        ++completion_count;
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
    const xy_hal_i2s_config_t config = {XY_HAL_I2S_MODE_MASTER_TX,
                                        XY_HAL_I2S_STANDARD_PHILIPS,
                                        XY_HAL_I2S_DATAFORMAT_16B,
                                        XY_HAL_I2S_CPOL_LOW,
                                        XY_HAL_I2S_MCLK_DISABLE,
                                        48000U,
                                        144000000U,
                                        XY_HAL_I2S_COMMUNICATION_TXONLY};
    const uint16_t samples[] = {0x1234U, 0xABCDU, 0x55AAU, 0x0F0FU};
    SystemCoreClockUpdate();
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK)
        for (;;) {
        }
    console_write("OPENCH_I2S2_BOOT_ID_20261008_A\r\n");
    xy_hal_error_t init_status = xy_hal_i2s_init(SPI2, &config);
    console_write(init_status == XY_HAL_OK ? "OPENCH_I2S2_INIT_OK_A\r\n"
                                           : "OPENCH_I2S2_INIT_ERROR_A\r\n");
    if (init_status != XY_HAL_OK) {
        for (;;) {
        }
    }
    xy_hal_error_t callback_status = xy_hal_i2s_register_callback(SPI2, callback, NULL);
    console_write(callback_status == XY_HAL_OK ? "OPENCH_I2S2_CALLBACK_OK_A\r\n"
                                               : "OPENCH_I2S2_CALLBACK_ERROR_A\r\n");
    xy_hal_error_t start_status = xy_hal_i2s_start(SPI2);
    console_write(start_status == XY_HAL_OK ? "OPENCH_I2S2_START_OK_A\r\n"
                                            : "OPENCH_I2S2_START_ERROR_A\r\n");
    if (callback_status != XY_HAL_OK || start_status != XY_HAL_OK) {
        console_write("OPENCH_I2S2_SETUP_ERROR_A\r\n");
        for (;;) {
        }
    }
    console_write("OPENCH_I2S2_READY_A\r\n");
    for (;;) {
        uint32_t before = completion_count;
        int32_t sent = xy_hal_i2s_send(SPI2, samples, 4U, 100U);
        if (sent == 4 && completion_count == before + 1U &&
            xy_hal_i2s_get_audio_freq(SPI2) == 48000 &&
            xy_hal_i2s_get_data_format(SPI2) == XY_HAL_I2S_DATAFORMAT_16B)
            console_write("OPENCH_I2S2_TX_OK_A\r\n");
        else
            console_write("OPENCH_I2S2_TX_ERROR_A\r\n");
        for (volatile uint32_t delay = 0U; delay < 3000000U; ++delay) {
        }
    }
}
