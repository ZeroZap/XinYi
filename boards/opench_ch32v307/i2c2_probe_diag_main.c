#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_uart.h"
#include <stddef.h>

static void marker(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, 1000U);
}

int main(void) {
    const xy_hal_gpio_config_t uart_tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                          XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t uart_rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                          XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_gpio_config_t i2c_pin = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                          XY_HAL_GPIO_OTYPE_OD, XY_HAL_GPIO_SPEED_HIGH, 0U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    const xy_hal_i2c_config_t i2c = {100000U, XY_HAL_I2C_ADDR_7BIT, XY_HAL_I2C_DUTY_2, 0U, 0U};
    uint8_t who_am_i;
    uint32_t iteration;
    int valid = 1;

    SystemCoreClockUpdate();
    SysTick->CTLR = 0U;
    SysTick->CNT = 0U;
    SysTick->CMP = UINT64_MAX;
    SysTick->CTLR = (1U << 5U) | 1U;
    if (xy_hal_gpio_init(GPIOA, 9U, &uart_tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &uart_rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK)
        for (;;) {
        }
    marker("OPENCH_I2C2_PROBE_DIAG_BOOT_20261014_A\r\n");
    if (xy_hal_gpio_init(GPIOB, 10U, &i2c_pin) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 11U, &i2c_pin) != XY_HAL_OK ||
        xy_hal_gpio_write(GPIOB, 10U, 1U) != XY_HAL_OK ||
        xy_hal_gpio_write(GPIOB, 11U, 1U) != XY_HAL_OK ||
        xy_hal_i2c_init(I2C2, &i2c) != XY_HAL_OK) {
        marker("OPENCH_I2C2_PROBE_DIAG_INIT_ERROR_A\r\n");
        for (;;) {
        }
    }
    for (iteration = 0U; iteration < 32U && valid; ++iteration) {
        valid = xy_hal_i2c_is_device_ready(I2C2, 0x7FU, 1U, 100U) == XY_HAL_ERROR_NOT_FOUND;
        valid = valid && xy_hal_i2c_is_device_ready(I2C2, 0x68U, 1U, 100U) == XY_HAL_OK;
        valid = valid &&
                xy_hal_i2c_mem_read(I2C2, 0x68U, 0x75U, &who_am_i, 1U, 100U) == XY_HAL_OK &&
                who_am_i == 0x68U;
    }
    marker(valid ? "OPENCH_I2C2_PROBE_DIAG_32X_OK_A\r\n" : "OPENCH_I2C2_PROBE_DIAG_ERROR_A\r\n");
    for (;;) {
    }
}
