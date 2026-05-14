/**
 * @file xy_hal_gpio.c
 * @brief STM32F4 implementation of xy_hal_gpio API
 */

#include "../../inc/xy_hal_gpio.h"
#include "../../inc/xy_hal.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"
#include <string.h>

typedef struct {
    xy_hal_gpio_irq_handler_t handler;
    void *arg;
} pin_irq_ctx_t;

static pin_irq_ctx_t g_pin_irq_ctx[16];

static uint32_t xy_to_stm32_mode(xy_hal_gpio_mode_t mode)
{
    switch (mode) {
    case XY_HAL_GPIO_MODE_INPUT:  return GPIO_MODE_INPUT;
    case XY_HAL_GPIO_MODE_OUTPUT: return GPIO_MODE_OUTPUT_PP;
    case XY_HAL_GPIO_MODE_AF:     return GPIO_MODE_AF_PP;
    case XY_HAL_GPIO_MODE_ANALOG: return GPIO_MODE_ANALOG;
    default:                      return GPIO_MODE_INPUT;
    }
}

static uint32_t xy_to_stm32_pull(xy_hal_gpio_pull_t pull)
{
    switch (pull) {
    case XY_HAL_GPIO_PULL_NONE: return GPIO_NOPULL;
    case XY_HAL_GPIO_PULL_UP:   return GPIO_PULLUP;
    case XY_HAL_GPIO_PULL_DOWN: return GPIO_PULLDOWN;
    default:                    return GPIO_NOPULL;
    }
}

static uint32_t xy_to_stm32_speed(xy_hal_gpio_speed_t speed)
{
    switch (speed) {
    case XY_HAL_GPIO_SPEED_LOW:       return GPIO_SPEED_FREQ_LOW;
    case XY_HAL_GPIO_SPEED_MEDIUM:    return GPIO_SPEED_FREQ_MEDIUM;
    case XY_HAL_GPIO_SPEED_HIGH:      return GPIO_SPEED_FREQ_HIGH;
    case XY_HAL_GPIO_SPEED_VERY_HIGH: return GPIO_SPEED_FREQ_VERY_HIGH;
    default:                          return GPIO_SPEED_FREQ_LOW;
    }
}

static IRQn_Type pin_to_irqn(uint8_t pin)
{
    switch (pin) {
    case 0: return EXTI0_IRQn;
    case 1: return EXTI1_IRQn;
    case 2: return EXTI2_IRQn;
    case 3: return EXTI3_IRQn;
    case 4: return EXTI4_IRQn;
    case 5: case 6: case 7: case 8: case 9:
        return EXTI9_5_IRQn;
    default:
        return EXTI15_10_IRQn;
    }
}

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    if (!port || !config || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    GPIO_InitTypeDef gpio_init = { 0 };
    gpio_init.Pin              = (1U << pin);
    gpio_init.Mode             = xy_to_stm32_mode(config->mode);
    gpio_init.Pull             = xy_to_stm32_pull(config->pull);
    gpio_init.Speed            = xy_to_stm32_speed(config->speed);

    if (config->mode == XY_HAL_GPIO_MODE_AF) {
        gpio_init.Alternate = config->alternate;
    }
    if (config->mode == XY_HAL_GPIO_MODE_OUTPUT && config->otype == XY_HAL_GPIO_OTYPE_OD) {
        gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    }

    HAL_GPIO_Init((GPIO_TypeDef *)port, &gpio_init);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_deinit(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_GPIO_DeInit((GPIO_TypeDef *)port, (1U << pin));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_GPIO_WritePin((GPIO_TypeDef *)port, (1U << pin),
                      value ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return HAL_GPIO_ReadPin((GPIO_TypeDef *)port, (1U << pin)) == GPIO_PIN_SET ? 1 : 0;
}

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_GPIO_TogglePin((GPIO_TypeDef *)port, (1U << pin));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_attach_irq(xy_hal_gpio_port_t port, uint8_t pin,
                                      xy_hal_gpio_irq_mode_t mode,
                                      xy_hal_gpio_irq_handler_t handler,
                                      void *arg)
{
    if (!port || pin > 15 || !handler) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uint32_t irq_mode;
    switch (mode) {
    case XY_HAL_GPIO_IRQ_RISING:  irq_mode = GPIO_MODE_IT_RISING; break;
    case XY_HAL_GPIO_IRQ_FALLING: irq_mode = GPIO_MODE_IT_FALLING; break;
    case XY_HAL_GPIO_IRQ_BOTH:    irq_mode = GPIO_MODE_IT_RISING_FALLING; break;
    default: return XY_HAL_ERROR_INVALID_PARAM;
    }

    GPIO_InitTypeDef gpio_init = { 0 };
    gpio_init.Pin              = (1U << pin);
    gpio_init.Mode             = irq_mode;
    gpio_init.Pull             = GPIO_NOPULL;
    HAL_GPIO_Init((GPIO_TypeDef *)port, &gpio_init);

    g_pin_irq_ctx[pin].handler = handler;
    g_pin_irq_ctx[pin].arg     = arg;

    IRQn_Type irqn = pin_to_irqn(pin);
    HAL_NVIC_SetPriority(irqn, 5, 0);
    HAL_NVIC_EnableIRQ(irqn);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_detach_irq(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_GPIO_DeInit((GPIO_TypeDef *)port, (1U << pin));
    g_pin_irq_ctx[pin].handler = NULL;
    g_pin_irq_ctx[pin].arg     = NULL;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_irq_enable(xy_hal_gpio_port_t port, uint8_t pin)
{
    (void)port;
    if (pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_NVIC_EnableIRQ(pin_to_irqn(pin));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_irq_disable(xy_hal_gpio_port_t port, uint8_t pin)
{
    (void)port;
    if (pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_NVIC_DisableIRQ(pin_to_irqn(pin));
    return XY_HAL_OK;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    for (int i = 0; i < 16; i++) {
        if ((GPIO_Pin & (1U << i)) && g_pin_irq_ctx[i].handler) {
            g_pin_irq_ctx[i].handler(g_pin_irq_ctx[i].arg);
        }
    }
}

#endif /* STM32_HAL_ENABLED */
