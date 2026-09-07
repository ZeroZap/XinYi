#include "pandora_hw_i2c.h"

#include "stm32l4xx_hal.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"

static I2C_HandleTypeDef i2c3;

void *pandora_hw_i2c3_init(void)
{
    const xy_hal_gpio_config_t gpio = {
        .mode = XY_HAL_GPIO_MODE_AF,
        .pull = XY_HAL_GPIO_PULL_UP,
        .otype = XY_HAL_GPIO_OTYPE_OD,
        .speed = XY_HAL_GPIO_SPEED_VERY_HIGH,
        .alternate = GPIO_AF4_I2C3,
    };
    const xy_hal_i2c_config_t config = {
        .clock_speed = 100000U,
        .addr_mode = XY_HAL_I2C_ADDR_7BIT,
        .duty_cycle = XY_HAL_I2C_DUTY_2,
        .own_address = 0U,
        .general_call_mode = 0U,
    };

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_I2C3_CLK_ENABLE();

    if (xy_hal_gpio_init(GPIOC, 0U, &gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOC, 1U, &gpio) != XY_HAL_OK) {
        return NULL;
    }

    i2c3.Instance = I2C3;
    i2c3.Init.Timing = 0x10909CECU;
    if (xy_hal_i2c_init(&i2c3, &config) != XY_HAL_OK) {
        return NULL;
    }

    return &i2c3;
}
