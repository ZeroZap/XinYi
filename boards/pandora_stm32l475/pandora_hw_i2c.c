#include "pandora_hw_i2c.h"

#include "stm32l4xx_hal.h"
#include "xy_hal_i2c.h"

static I2C_HandleTypeDef i2c3;

void *pandora_hw_i2c3_init(void)
{
    GPIO_InitTypeDef gpio = {0};

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
    i2c3.Init.OwnAddress1 = 0U;
    i2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    i2c3.Init.OwnAddress2 = 0U;
    i2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    i2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&i2c3) != HAL_OK ||
        HAL_I2CEx_ConfigAnalogFilter(&i2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK ||
        HAL_I2CEx_ConfigDigitalFilter(&i2c3, 0U) != HAL_OK) {
        return NULL;
    }

    return &i2c3;
}

xy_hal_error_t xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                          const uint8_t *data, size_t len,
                                          uint32_t timeout)
{
    if (i2c != &i2c3 || data == NULL || len == 0U || len > UINT16_MAX || dev_addr > 0x7FU) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return HAL_I2C_Master_Transmit(&i2c3, (uint16_t)(dev_addr << 1), (uint8_t *)data,
                                   (uint16_t)len, timeout) == HAL_OK
               ? XY_HAL_OK
               : XY_HAL_ERROR_IO;
}

xy_hal_error_t xy_hal_i2c_master_receive(void *i2c, uint16_t dev_addr, uint8_t *data,
                                         size_t len, uint32_t timeout)
{
    if (i2c != &i2c3 || data == NULL || len == 0U || len > UINT16_MAX || dev_addr > 0x7FU) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return HAL_I2C_Master_Receive(&i2c3, (uint16_t)(dev_addr << 1), data, (uint16_t)len,
                                  timeout) == HAL_OK
               ? XY_HAL_OK
               : XY_HAL_ERROR_IO;
}
