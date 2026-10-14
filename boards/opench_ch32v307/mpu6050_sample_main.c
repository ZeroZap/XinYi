#include "ch32v30x.h"
#include "xy_hal_delay.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_uart.h"
#include <stddef.h>

#define MPU6050_ADDRESS 0x68U

static void marker(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, 1000U);
}

static int16_t signed_be(const uint8_t* data) {
    return (int16_t)(((uint16_t)data[0] << 8U) | data[1]);
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
    uint8_t original_power;
    uint8_t active_power;
    uint8_t restored_power;
    uint8_t frame[14];
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t temperature;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int32_t temperature_centi_c;
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
    marker("OPENCH_MPU6050_SAMPLE_BOOT_20261014_A\r\n");
    if (xy_hal_i2c_init(I2C2, &i2c) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 10U, &i2c_pin) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 11U, &i2c_pin) != XY_HAL_OK) {
        marker("OPENCH_MPU6050_SAMPLE_INIT_ERROR_A\r\n");
        for (;;) {
        }
    }
    if (xy_hal_i2c_mem_read(I2C2, MPU6050_ADDRESS, 0x75U, &who_am_i, 1U, 100U) != XY_HAL_OK ||
        who_am_i != 0x68U ||
        xy_hal_i2c_mem_read(I2C2, MPU6050_ADDRESS, 0x6BU, &original_power, 1U, 100U) != XY_HAL_OK) {
        marker("OPENCH_MPU6050_IDENTITY_ERROR_A\r\n");
        for (;;) {
        }
    }
    marker("OPENCH_MPU6050_IDENTITY_OK_A\r\n");
    active_power = (uint8_t)(original_power & (uint8_t)~0x40U);
    if (xy_hal_i2c_mem_write(I2C2, MPU6050_ADDRESS, 0x6BU, &active_power, 1U, 100U) != XY_HAL_OK) {
        marker("OPENCH_MPU6050_WAKE_ERROR_A\r\n");
        for (;;) {
        }
    }
    xy_hal_delay_ms(100U);
    valid =
        xy_hal_i2c_mem_read(I2C2, MPU6050_ADDRESS, 0x3BU, frame, sizeof(frame), 100U) == XY_HAL_OK;
    accel_x = signed_be(&frame[0]);
    accel_y = signed_be(&frame[2]);
    accel_z = signed_be(&frame[4]);
    temperature = signed_be(&frame[6]);
    gyro_x = signed_be(&frame[8]);
    gyro_y = signed_be(&frame[10]);
    gyro_z = signed_be(&frame[12]);
    temperature_centi_c = ((int32_t)temperature * 100) / 340 + 3653;
    valid = valid && temperature_centi_c >= -4000 && temperature_centi_c <= 8500 &&
            (accel_x != 0 || accel_y != 0 || accel_z != 0) &&
            (gyro_x != 0 || gyro_y != 0 || gyro_z != 0);
    marker(valid ? "OPENCH_MPU6050_SAMPLE_OK_A\r\n" : "OPENCH_MPU6050_SAMPLE_ERROR_A\r\n");
    if (xy_hal_i2c_mem_write(I2C2, MPU6050_ADDRESS, 0x6BU, &original_power, 1U, 100U) !=
            XY_HAL_OK ||
        xy_hal_i2c_mem_read(I2C2, MPU6050_ADDRESS, 0x6BU, &restored_power, 1U, 100U) != XY_HAL_OK ||
        restored_power != original_power) {
        marker("OPENCH_MPU6050_RESTORE_ERROR_A\r\n");
        for (;;) {
        }
    }
    marker("OPENCH_MPU6050_RESTORE_OK_A\r\n");
    for (;;) {
    }
}
