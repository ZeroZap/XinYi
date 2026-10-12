#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include <stddef.h>
#include <string.h>

static void marker(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, 1000U);
}

static void start_tick(void) {
    SysTick->CTLR = 0U;
    SysTick->CNT = 0U;
    SysTick->CMP = UINT64_MAX;
    SysTick->CTLR = (1U << 5U) | 1U;
}

static int receive_contains(void* uart, const char* needle, uint32_t timeout_ms) {
    char response[128] = {0};
    size_t length = 0U;
    uint32_t start = (uint32_t)(SysTick->CNT / (SystemCoreClock / 8000U));
    while ((uint32_t)(SysTick->CNT / (SystemCoreClock / 8000U)) - start < timeout_ms) {
        if (xy_hal_uart_available(uart) > 0 && length + 1U < sizeof(response)) {
            if (xy_hal_uart_recv(uart, (uint8_t*)&response[length], 1U, 10U) == XY_HAL_OK) {
                ++length;
                response[length] = '\0';
                if (strstr(response, needle) != NULL)
                    return 1;
            }
        }
    }
    return 0;
}

int main(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_UP,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_HIGH, 0U};
    const xy_hal_gpio_config_t output = {XY_HAL_GPIO_MODE_OUTPUT, XY_HAL_GPIO_PULL_NONE,
                                         XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_HIGH, 0U};
    const xy_hal_uart_config_t config = {115200U,
                                         XY_HAL_UART_WORDLEN_8B,
                                         XY_HAL_UART_STOPBITS_1,
                                         XY_HAL_UART_PARITY_NONE,
                                         XY_HAL_UART_FLOWCTRL_NONE,
                                         XY_HAL_UART_MODE_TX_RX};
    static const uint8_t esp_command[] = "AT\r\n";
    static const uint8_t ble_command[] = "AT+TPL?\r\n";

    SystemCoreClockUpdate();
    start_tick();
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_FullRemap_USART6, DISABLE);
    GPIO_PinRemapConfig(GPIO_FullRemap_USART7, DISABLE);
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &config) != XY_HAL_OK)
        for (;;) {
        }
    marker("OPENCH_WIRELESS_PEER_BOOT_20261012_A\r\n");
    if (xy_hal_gpio_init(GPIOC, 0U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOC, 1U, &rx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOC, 2U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOC, 3U, &rx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 7U, &output) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOC, 13U, &output) != XY_HAL_OK ||
        xy_hal_uart_init(UART6, &config) != XY_HAL_OK ||
        xy_hal_uart_init(UART7, &config) != XY_HAL_OK) {
        marker("OPENCH_WIRELESS_PEER_INIT_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_hal_gpio_write(GPIOA, 7U, 0U);
    (void)xy_hal_gpio_write(GPIOC, 13U, 1U);
    for (volatile uint32_t delay = 0U; delay < 8000000U; ++delay) {
    }
    (void)xy_hal_uart_flush(UART6);
    (void)xy_hal_uart_flush(UART7);
    if (xy_hal_uart_send(UART6, esp_command, sizeof(esp_command) - 1U, 100U) == XY_HAL_OK &&
        receive_contains(UART6, "OK", 1500U))
        marker("OPENCH_UART6_ESP8266_AT_OK_A\r\n");
    else
        marker("OPENCH_UART6_ESP8266_AT_ERROR_A\r\n");
    if (xy_hal_uart_send(UART7, ble_command, sizeof(ble_command) - 1U, 100U) == XY_HAL_OK &&
        receive_contains(UART7, "TPL", 1500U))
        marker("OPENCH_UART7_CH9141_AT_OK_A\r\n");
    else
        marker("OPENCH_UART7_CH9141_AT_ERROR_A\r\n");
    for (;;) {
    }
}
