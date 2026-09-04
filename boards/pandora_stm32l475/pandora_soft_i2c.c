#include "pandora_soft_i2c.h"

#include "stm32l4xx_hal.h"
#include "xy_hal_i2c.h"

#define SOFT_I2C_SDA_PORT GPIOC
#define SOFT_I2C_SDA_PIN GPIO_PIN_1
#define SOFT_I2C_SCL_PORT GPIOD
#define SOFT_I2C_SCL_PIN GPIO_PIN_6

static uint8_t soft_i2c_bus;

static void soft_i2c_delay(void)
{
    for (volatile uint32_t i = 0; i < 80U; ++i) {
        __NOP();
    }
}

static void soft_i2c_sda(GPIO_PinState state)
{
    HAL_GPIO_WritePin(SOFT_I2C_SDA_PORT, SOFT_I2C_SDA_PIN, state);
    soft_i2c_delay();
}

static void soft_i2c_scl(GPIO_PinState state)
{
    HAL_GPIO_WritePin(SOFT_I2C_SCL_PORT, SOFT_I2C_SCL_PIN, state);
    soft_i2c_delay();
}

static void soft_i2c_start(void)
{
    soft_i2c_sda(GPIO_PIN_SET);
    soft_i2c_scl(GPIO_PIN_SET);
    soft_i2c_sda(GPIO_PIN_RESET);
    soft_i2c_scl(GPIO_PIN_RESET);
}

static void soft_i2c_stop(void)
{
    soft_i2c_sda(GPIO_PIN_RESET);
    soft_i2c_scl(GPIO_PIN_SET);
    soft_i2c_sda(GPIO_PIN_SET);
}

static int soft_i2c_write_byte(uint8_t value)
{
    GPIO_InitTypeDef gpio = {0};

    for (uint32_t bit = 0; bit < 8U; ++bit) {
        soft_i2c_sda((value & 0x80U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        soft_i2c_scl(GPIO_PIN_SET);
        soft_i2c_scl(GPIO_PIN_RESET);
        value <<= 1;
    }
    gpio.Pin = SOFT_I2C_SDA_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SOFT_I2C_SDA_PORT, &gpio);
    soft_i2c_scl(GPIO_PIN_SET);
    int ack = HAL_GPIO_ReadPin(SOFT_I2C_SDA_PORT, SOFT_I2C_SDA_PIN) == GPIO_PIN_RESET;
    soft_i2c_scl(GPIO_PIN_RESET);
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(SOFT_I2C_SDA_PORT, &gpio);
    return ack;
}

static uint8_t soft_i2c_read_byte(int send_ack)
{
    GPIO_InitTypeDef gpio = {0};
    uint8_t value = 0;

    gpio.Pin = SOFT_I2C_SDA_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SOFT_I2C_SDA_PORT, &gpio);
    for (uint32_t bit = 0; bit < 8U; ++bit) {
        value <<= 1;
        soft_i2c_scl(GPIO_PIN_SET);
        if (HAL_GPIO_ReadPin(SOFT_I2C_SDA_PORT, SOFT_I2C_SDA_PIN) == GPIO_PIN_SET) {
            value |= 1U;
        }
        soft_i2c_scl(GPIO_PIN_RESET);
    }
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(SOFT_I2C_SDA_PORT, &gpio);
    soft_i2c_sda(send_ack ? GPIO_PIN_RESET : GPIO_PIN_SET);
    soft_i2c_scl(GPIO_PIN_SET);
    soft_i2c_scl(GPIO_PIN_RESET);
    soft_i2c_sda(GPIO_PIN_SET);
    return value;
}

void *pandora_soft_i2c_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    gpio.Pin = SOFT_I2C_SDA_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SOFT_I2C_SDA_PORT, &gpio);
    gpio.Pin = SOFT_I2C_SCL_PIN;
    HAL_GPIO_Init(SOFT_I2C_SCL_PORT, &gpio);
    soft_i2c_sda(GPIO_PIN_SET);
    soft_i2c_scl(GPIO_PIN_SET);
    return &soft_i2c_bus;
}

xy_hal_error_t xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                          const uint8_t *data, size_t len,
                                          uint32_t timeout)
{
    (void)timeout;
    if (i2c != &soft_i2c_bus || data == NULL || len == 0U || dev_addr > 0x7FU) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    soft_i2c_start();
    if (!soft_i2c_write_byte((uint8_t)(dev_addr << 1))) {
        soft_i2c_stop();
        return XY_HAL_ERROR_IO;
    }
    for (size_t i = 0; i < len; ++i) {
        if (!soft_i2c_write_byte(data[i])) {
            soft_i2c_stop();
            return XY_HAL_ERROR_IO;
        }
    }
    soft_i2c_stop();
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive(void *i2c, uint16_t dev_addr, uint8_t *data,
                                         size_t len, uint32_t timeout)
{
    (void)timeout;
    if (i2c != &soft_i2c_bus || data == NULL || len == 0U || dev_addr > 0x7FU) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    soft_i2c_start();
    if (!soft_i2c_write_byte((uint8_t)((dev_addr << 1) | 1U))) {
        soft_i2c_stop();
        return XY_HAL_ERROR_IO;
    }
    for (size_t i = 0; i < len; ++i) {
        data[i] = soft_i2c_read_byte(i + 1U < len);
    }
    soft_i2c_stop();
    return XY_HAL_OK;
}
