/**
 * @file demo_hal.c
 * @brief HAL Component Demo
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include <string.h>
#include "xy_hal.h"

/* Simplified log macros for demo */
#define xy_log_i(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define xy_log_d(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define xy_log_e(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

#ifdef DEMO_HAL

/**
 * @brief 初始化 HAL 演示
 */
int demo_hal_init(void)
{
    xy_log_i("HAL initialized (PC simulation mode)\n");
    return 0;
}

/**
 * @brief 运行 HAL 演示
 */
void demo_hal_run(void)
{
    xy_hal_gpio_config_t pin_cfg;
    int ret;

    /* GPIO 演示 */
    xy_log_i("GPIO Demo:\n");

    pin_cfg.mode = XY_HAL_GPIO_MODE_OUTPUT;
    pin_cfg.pull = XY_HAL_GPIO_PULL_NONE;
    pin_cfg.otype = XY_HAL_GPIO_OTYPE_PP;
    pin_cfg.speed = XY_HAL_GPIO_SPEED_LOW;

    ret = xy_hal_gpio_init((xy_hal_gpio_port_t)0x1, 5, &pin_cfg);  /* PA5 */
    if (ret == XY_HAL_OK) {
        xy_log_d("  GPIO PA5 configured as output\n");
    }

    xy_hal_gpio_write((xy_hal_gpio_port_t)0x1, 5, 1);
    xy_log_d("  GPIO PA5 set to 1\n");

    xy_hal_gpio_write((xy_hal_gpio_port_t)0x1, 5, 0);
    xy_log_d("  GPIO PA5 set to 0\n");

    int level = xy_hal_gpio_read((xy_hal_gpio_port_t)0x1, 5);
    xy_log_d("  GPIO PA5 read: %d\n", level);
    
    /* UART 演示 */
    xy_log_i("UART Demo:\n");
    
    const char *msg = "Hello, XinYi!";
    ret = xy_hal_uart_send(NULL, (const unsigned char *)msg, strlen(msg), 1000);
    if (ret == XY_HAL_OK) {
        xy_log_d("  UART TX: \"%s\"\n", msg);
    }
    
    /* I2C 演示 */
    xy_log_i("I2C Demo:\n");
    
    unsigned char reg = 0x00;
    unsigned char data[2];
    
    ret = xy_hal_i2c_master_transmit(NULL, 0x44, &reg, 1, 1000);
    if (ret == XY_HAL_OK) {
        xy_log_d("  I2C TX: addr=0x44, reg=0x%02X\n", reg);
    }
    
    ret = xy_hal_i2c_master_receive(NULL, 0x44, data, 2, 1000);
    if (ret == XY_HAL_OK) {
        xy_log_d("  I2C RX: [0x%02X, 0x%02X]\n", data[0], data[1]);
    }
    
    /* SPI 演示 */
    xy_log_i("SPI Demo:\n");
    
    unsigned char tx_data = 0xAA;
    unsigned char rx_data;
    
    ret = xy_hal_spi_transmit_receive(NULL, &tx_data, &rx_data, 1, 1000);
    if (ret == XY_HAL_OK) {
        xy_log_d("  SPI TX/RX: 0x%02X -> 0x%02X\n", tx_data, rx_data);
    }
    
    xy_log_i("HAL demo completed\n");
}

#endif /* DEMO_HAL */
