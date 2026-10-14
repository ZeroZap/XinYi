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

static uint32_t tick_ms(void) {
    return (uint32_t)(SysTick->CNT / (SystemCoreClock / 8000U));
}

static int receive_contains(void* uart, const char* needle, uint32_t timeout_ms) {
    char response[128] = {0};
    size_t length = 0U;
    uint32_t start = tick_ms();
    while (tick_ms() - start < timeout_ms) {
        if (xy_hal_uart_available(uart) > 0 && length + 1U < sizeof(response) &&
            xy_hal_uart_recv(uart, (uint8_t*)&response[length], 1U, 20U) == XY_HAL_OK) {
            ++length;
            response[length] = '\0';
            if (strstr(response, needle) != NULL)
                return 1;
        }
    }
    return 0;
}

static int peer_roundtrip(void* uart, const uint8_t* command, size_t command_length,
                          const char* expected) {
    (void)xy_hal_uart_flush(uart);
    return xy_hal_uart_send(uart, command, command_length, 100U) == XY_HAL_OK &&
           receive_contains(uart, expected, 1500U);
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
    uint32_t iteration;
    int valid = 1;

    SystemCoreClockUpdate();
    SysTick->CTLR = 0U;
    SysTick->CNT = 0U;
    SysTick->CMP = UINT64_MAX;
    SysTick->CTLR = 1U;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_FullRemap_USART6, DISABLE);
    GPIO_PinRemapConfig(GPIO_FullRemap_USART7, DISABLE);
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &config) != XY_HAL_OK)
        for (;;) {
        }
    marker("OPENCH_UART67_SYSTICK8_BOOT_20261014_A\r\n");
    if (xy_hal_gpio_init(GPIOC, 0U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOC, 1U, &rx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOC, 2U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOC, 3U, &rx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 7U, &output) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOC, 13U, &output) != XY_HAL_OK ||
        xy_hal_uart_init(UART6, &config) != XY_HAL_OK ||
        xy_hal_uart_init(UART7, &config) != XY_HAL_OK) {
        marker("OPENCH_UART67_SYSTICK8_INIT_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_hal_gpio_write(GPIOA, 7U, 0U);
    (void)xy_hal_gpio_write(GPIOC, 13U, 1U);
    for (volatile uint32_t delay = 0U; delay < 8000000U; ++delay) {
    }
    for (iteration = 0U; iteration < 8U && valid; ++iteration) {
        valid = peer_roundtrip(UART6, esp_command, sizeof(esp_command) - 1U, "OK");
        valid = valid && peer_roundtrip(UART7, ble_command, sizeof(ble_command) - 1U, "TPL");
    }
    marker(valid ? "OPENCH_UART67_SYSTICK8_PEERS_8X_OK_A\r\n"
                 : "OPENCH_UART67_SYSTICK8_PEERS_ERROR_A\r\n");
    for (;;) {
    }
}
