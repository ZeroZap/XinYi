#include "ch32v30x.h"
#include "xy_hal_delay.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_uart.h"
#include <stddef.h>

#define AP3216C_ADDRESS 0x1EU

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
    const uint8_t active_mode = 0x03U;
    uint8_t original_mode;
    uint8_t restored_mode;
    uint8_t frame[6];
    uint16_t infrared;

    uint16_t proximity;
    int valid;

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
    marker("OPENCH_AP3216C_PROTOCOL_BOOT_20261014_A\r\n");
    if (xy_hal_i2c_init(I2C2, &i2c) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 10U, &i2c_pin) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 11U, &i2c_pin) != XY_HAL_OK) {
        marker("OPENCH_AP3216C_PROTOCOL_INIT_ERROR_A\r\n");
        for (;;) {
        }
    }
    if (xy_hal_i2c_mem_read(I2C2, AP3216C_ADDRESS, 0x00U, &original_mode, 1U, 100U) != XY_HAL_OK ||
        (original_mode & 0xF8U) != 0U) {
        marker("OPENCH_AP3216C_CONFIG_READ_ERROR_A\r\n");
        for (;;) {
        }
    }
    marker("OPENCH_AP3216C_CONFIG_READ_OK_A\r\n");
    if (xy_hal_i2c_mem_write(I2C2, AP3216C_ADDRESS, 0x00U, &active_mode, 1U, 100U) != XY_HAL_OK) {
        marker("OPENCH_AP3216C_MODE_SET_ERROR_A\r\n");
        for (;;) {
        }
    }
    xy_hal_delay_ms(120U);
    valid =
        xy_hal_i2c_mem_read(I2C2, AP3216C_ADDRESS, 0x0AU, frame, sizeof(frame), 100U) == XY_HAL_OK;
    infrared = (uint16_t)(((uint16_t)frame[1] << 2U) | (frame[0] & 0x03U));

    proximity = (uint16_t)(((uint16_t)(frame[5] & 0x3FU) << 4U) | (frame[4] & 0x0FU));
    valid = valid && (frame[0] & 0x80U) == 0U && (frame[4] & 0x40U) == 0U && infrared <= 1023U &&
            proximity <= 1023U;
    marker(valid ? "OPENCH_AP3216C_SAMPLE_OK_A\r\n" : "OPENCH_AP3216C_SAMPLE_ERROR_A\r\n");
    if (xy_hal_i2c_mem_write(I2C2, AP3216C_ADDRESS, 0x00U, &original_mode, 1U, 100U) != XY_HAL_OK ||
        xy_hal_i2c_mem_read(I2C2, AP3216C_ADDRESS, 0x00U, &restored_mode, 1U, 100U) != XY_HAL_OK ||
        restored_mode != original_mode) {
        marker("OPENCH_AP3216C_RESTORE_ERROR_A\r\n");
        for (;;) {
        }
    }
    marker("OPENCH_AP3216C_RESTORE_OK_A\r\n");
    for (;;) {
    }
}
