#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_uart.h"
#include <stddef.h>
static void m(const char* s) {
    size_t n = 0;
    while (s[n])
        ++n;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)s, n, 1000);
}
static int rr(uint8_t a, uint8_t r) {
    uint8_t o, p, q, z;
    if (xy_hal_i2c_mem_read(I2C2, a, r, &o, 1, 100) != XY_HAL_OK)
        return 0;
    p = o ^ 1;
    if (xy_hal_i2c_mem_write(I2C2, a, r, &p, 1, 100) != XY_HAL_OK)
        return 0;
    if (xy_hal_i2c_mem_read(I2C2, a, r, &q, 1, 100) != XY_HAL_OK || q != p)
        return 0;
    if (xy_hal_i2c_mem_write(I2C2, a, r, &o, 1, 100) != XY_HAL_OK)
        return 0;
    return xy_hal_i2c_mem_read(I2C2, a, r, &z, 1, 100) == XY_HAL_OK && z == o;
}
int main(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0},
                               rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0},
                               ip = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_OD, XY_HAL_GPIO_SPEED_HIGH, 0};
    const xy_hal_uart_config_t u = {115200,
                                    XY_HAL_UART_WORDLEN_8B,
                                    XY_HAL_UART_STOPBITS_1,
                                    XY_HAL_UART_PARITY_NONE,
                                    XY_HAL_UART_FLOWCTRL_NONE,
                                    XY_HAL_UART_MODE_TX_RX};
    const xy_hal_i2c_config_t c = {100000, XY_HAL_I2C_ADDR_7BIT, XY_HAL_I2C_DUTY_2, 0, 0};
    uint8_t v = 0;
    SystemCoreClockUpdate();
    SysTick->CTLR = 0;
    SysTick->CNT = 0;
    SysTick->CMP = UINT64_MAX;
    SysTick->CTLR = (1U << 5U) | 1U;
    xy_hal_gpio_init(GPIOA, 9, &tx);
    xy_hal_gpio_init(GPIOA, 10, &rx);
    xy_hal_uart_init(USART1, &u);
    m("OPENCH_I2C2_HAL_BOOT_20261012_D\r\n");
    xy_hal_gpio_init(GPIOB, 10, &ip);
    xy_hal_gpio_init(GPIOB, 11, &ip);
    xy_hal_gpio_write(GPIOB, 10, 1);
    xy_hal_gpio_write(GPIOB, 11, 1);
    xy_hal_i2c_init(I2C2, &c);
    m(xy_hal_i2c_mem_read(I2C2, 0x68, 0x75, &v, 1, 100) == XY_HAL_OK && v == 0x68
          ? "OPENCH_MPU6050_WHOAMI_OK_D\r\n"
          : "OPENCH_MPU6050_WHOAMI_ERROR_D\r\n");
    m(rr(0x1E, 0) ? "OPENCH_AP3216C_RW_RESTORE_OK_D\r\n" : "OPENCH_AP3216C_RW_RESTORE_ERROR_D\r\n");
    m(rr(0x10, 0x1A) ? "OPENCH_ES8388_RW_RESTORE_OK_D\r\n"
                     : "OPENCH_ES8388_RW_RESTORE_ERROR_D\r\n");
    m(xy_hal_i2c_is_device_ready(I2C2, 0x38, 2, 100) == XY_HAL_OK ? "OPENCH_AHT10_ACK_OK_D\r\n"
                                                                  : "OPENCH_AHT10_ACK_ERROR_D\r\n");
    for (;;) {
    }
}
