#include "pandora_hw_i2c.h"

#include "stm32l4xx_hal.h"
#include "xy_hal_i2c.h"

static I2C_HandleTypeDef i2c3;

void *pandora_hw_i2c3_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    const xy_hal_i2c_config_t config = {
        .clock_speed = 100000U,
        .addr_mode = XY_HAL_I2C_ADDR_7BIT,
        .duty_cycle = XY_HAL_I2C_DUTY_2,
        .own_address = 0U,
        .general_call_mode = 0U,
    };

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_I2C3_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOC, &gpio);

    i2c3.Instance = I2C3;
    i2c3.Init.Timing = 0x10909CECU;
    if (xy_hal_i2c_init(&i2c3, &config) != XY_HAL_OK) {
        return NULL;
    }

    return &i2c3;
}
