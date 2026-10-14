#include "ch32v30x.h"
#include "xy_hal_delay.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_uart.h"
#include <stddef.h>

#define AHT10_ADDRESS 0x38U

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
    const uint8_t init_command[] = {0xE1U, 0x08U, 0x00U};
    const uint8_t measure_command[] = {0xACU, 0x33U, 0x00U};
    uint8_t status;
    uint8_t frame[6];
    uint32_t humidity_raw;
    uint32_t temperature_raw;
    uint32_t humidity_centi_pct;
    int32_t temperature_centi_c;

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
    marker("OPENCH_AHT10_PROTOCOL_BOOT_20261014_A\r\n");
    if (xy_hal_i2c_init(I2C2, &i2c) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 10U, &i2c_pin) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 11U, &i2c_pin) != XY_HAL_OK) {
        marker("OPENCH_AHT10_PROTOCOL_INIT_ERROR_A\r\n");
        for (;;) {
        }
    }

    if (xy_hal_i2c_master_receive(I2C2, AHT10_ADDRESS, &status, 1U, 100U) != XY_HAL_OK) {
        marker("OPENCH_AHT10_STATUS_ERROR_A\r\n");
        for (;;) {
        }
    }
    marker("OPENCH_AHT10_STATUS_OK_A\r\n");
    if ((status & 0x08U) == 0U) {
        if (xy_hal_i2c_master_transmit(I2C2, AHT10_ADDRESS, init_command, sizeof(init_command),
                                       100U) != XY_HAL_OK) {
            marker("OPENCH_AHT10_CALIBRATION_ERROR_A\r\n");
            for (;;) {
            }
        }
        xy_hal_delay_ms(10U);
        if (xy_hal_i2c_master_receive(I2C2, AHT10_ADDRESS, &status, 1U, 100U) != XY_HAL_OK ||
            (status & 0x08U) == 0U) {
            marker("OPENCH_AHT10_CALIBRATION_ERROR_A\r\n");
            for (;;) {
            }
        }
    }
    marker("OPENCH_AHT10_CALIBRATED_OK_A\r\n");

    if (xy_hal_i2c_master_transmit(I2C2, AHT10_ADDRESS, measure_command, sizeof(measure_command),
                                   100U) != XY_HAL_OK) {
        marker("OPENCH_AHT10_MEASUREMENT_ERROR_A\r\n");
        for (;;) {
        }
    }
    xy_hal_delay_ms(80U);
    if (xy_hal_i2c_master_receive(I2C2, AHT10_ADDRESS, frame, sizeof(frame), 100U) != XY_HAL_OK ||
        (frame[0] & 0x80U) != 0U || (frame[0] & 0x08U) == 0U) {
        marker("OPENCH_AHT10_MEASUREMENT_ERROR_A\r\n");
        for (;;) {
        }
    }
    humidity_raw = ((uint32_t)frame[1] << 12U) | ((uint32_t)frame[2] << 4U) | (frame[3] >> 4U);
    temperature_raw = ((uint32_t)(frame[3] & 0x0FU) << 16U) | ((uint32_t)frame[4] << 8U) | frame[5];
    humidity_centi_pct = (uint32_t)(((uint64_t)humidity_raw * 10000U) >> 20U);
    temperature_centi_c = (int32_t)(((uint64_t)temperature_raw * 20000U) >> 20U) - 5000;
    if (humidity_centi_pct <= 10000U && temperature_centi_c >= -5000 &&
        temperature_centi_c <= 15000)
        marker("OPENCH_AHT10_MEASUREMENT_OK_A\r\n");
    else
        marker("OPENCH_AHT10_MEASUREMENT_RANGE_ERROR_A\r\n");
    for (;;) {
    }
}
