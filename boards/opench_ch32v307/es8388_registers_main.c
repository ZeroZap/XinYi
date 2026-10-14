#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_uart.h"
#include <stddef.h>

#define ES8388_ADDRESS 0x10U

static void marker(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, 1000U);
}

static int read_register(uint8_t reg, uint8_t* value) {
    return xy_hal_i2c_mem_read(I2C2, ES8388_ADDRESS, reg, value, 1U, 100U) == XY_HAL_OK;
}

static int restore_register(uint8_t reg, uint8_t value) {
    uint8_t restored;
    return xy_hal_i2c_mem_write(I2C2, ES8388_ADDRESS, reg, &value, 1U, 100U) == XY_HAL_OK &&
           read_register(reg, &restored) && restored == value;
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
    const uint8_t register_list[] = {0x00U, 0x01U, 0x02U, 0x04U, 0x08U, 0x17U,
                                     0x1AU, 0x1BU, 0x27U, 0x2AU, 0x2EU, 0x2FU};
    uint8_t snapshot[sizeof(register_list)];
    uint8_t changed_left;
    uint8_t changed_right;
    uint8_t readback_left;
    uint8_t readback_right;
    size_t index;

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
    marker("OPENCH_ES8388_REGISTERS_BOOT_20261014_A\r\n");
    if (xy_hal_i2c_init(I2C2, &i2c) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 10U, &i2c_pin) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 11U, &i2c_pin) != XY_HAL_OK) {
        marker("OPENCH_ES8388_REGISTERS_INIT_ERROR_A\r\n");
        for (;;) {
        }
    }
    for (index = 0U; index < sizeof(register_list); ++index) {
        if (!read_register(register_list[index], &snapshot[index])) {
            marker("OPENCH_ES8388_SNAPSHOT_ERROR_A\r\n");
            for (;;) {
            }
        }
    }
    marker("OPENCH_ES8388_SNAPSHOT_OK_A\r\n");

    changed_left = (uint8_t)(snapshot[6] ^ 0x01U);
    changed_right = (uint8_t)(snapshot[7] ^ 0x01U);
    if (xy_hal_i2c_mem_write(I2C2, ES8388_ADDRESS, 0x1AU, &changed_left, 1U, 100U) != XY_HAL_OK ||
        xy_hal_i2c_mem_write(I2C2, ES8388_ADDRESS, 0x1BU, &changed_right, 1U, 100U) != XY_HAL_OK ||
        !read_register(0x1AU, &readback_left) || !read_register(0x1BU, &readback_right) ||
        readback_left != changed_left || readback_right != changed_right) {
        (void)restore_register(0x1AU, snapshot[6]);
        (void)restore_register(0x1BU, snapshot[7]);
        marker("OPENCH_ES8388_RW_ERROR_A\r\n");
        for (;;) {
        }
    }
    marker("OPENCH_ES8388_RW_OK_A\r\n");
    if (!restore_register(0x1AU, snapshot[6]) || !restore_register(0x1BU, snapshot[7])) {
        marker("OPENCH_ES8388_RESTORE_ERROR_A\r\n");
        for (;;) {
        }
    }
    marker("OPENCH_ES8388_RESTORE_OK_A\r\n");
    for (;;) {
    }
}
