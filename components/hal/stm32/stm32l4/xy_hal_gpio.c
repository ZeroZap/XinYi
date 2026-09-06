#include "xy_hal_gpio.h"

#include "stm32l4xx_hal.h"

static uint32_t gpio_mode(const xy_hal_gpio_config_t *config)
{
    if (config->mode == XY_HAL_GPIO_MODE_OUTPUT) {
        return config->otype == XY_HAL_GPIO_OTYPE_OD ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT_PP;
    }
    if (config->mode == XY_HAL_GPIO_MODE_AF) {
        return config->otype == XY_HAL_GPIO_OTYPE_OD ? GPIO_MODE_AF_OD : GPIO_MODE_AF_PP;
    }
    if (config->mode == XY_HAL_GPIO_MODE_ANALOG) {
        return GPIO_MODE_ANALOG;
    }
    return GPIO_MODE_INPUT;
}

static uint32_t gpio_pull(xy_hal_gpio_pull_t pull)
{
    if (pull == XY_HAL_GPIO_PULL_UP) {
        return GPIO_PULLUP;
    }
    if (pull == XY_HAL_GPIO_PULL_DOWN) {
        return GPIO_PULLDOWN;
    }
    return GPIO_NOPULL;
}

static uint32_t gpio_speed(xy_hal_gpio_speed_t speed)
{
    switch (speed) {
    case XY_HAL_GPIO_SPEED_MEDIUM:
        return GPIO_SPEED_FREQ_MEDIUM;
    case XY_HAL_GPIO_SPEED_HIGH:
        return GPIO_SPEED_FREQ_HIGH;
    case XY_HAL_GPIO_SPEED_VERY_HIGH:
        return GPIO_SPEED_FREQ_VERY_HIGH;
    default:
        return GPIO_SPEED_FREQ_LOW;
    }
}

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    GPIO_InitTypeDef init = {0};
    if (port == NULL || config == NULL || pin > 15U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    init.Pin = 1U << pin;
    init.Mode = gpio_mode(config);
    init.Pull = gpio_pull(config->pull);
    init.Speed = gpio_speed(config->speed);
    init.Alternate = config->alternate;
    HAL_GPIO_Init((GPIO_TypeDef *)port, &init);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_deinit(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (port == NULL || pin > 15U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_GPIO_DeInit((GPIO_TypeDef *)port, 1U << pin);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value)
{
    if (port == NULL || pin > 15U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_GPIO_WritePin((GPIO_TypeDef *)port, 1U << pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (port == NULL || pin > 15U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return HAL_GPIO_ReadPin((GPIO_TypeDef *)port, 1U << pin) == GPIO_PIN_SET ? 1 : 0;
}

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (port == NULL || pin > 15U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_GPIO_TogglePin((GPIO_TypeDef *)port, 1U << pin);
    return XY_HAL_OK;
}