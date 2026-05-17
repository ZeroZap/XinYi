/**
 * @file xy_hal_gpio.c
 * @brief STM32U5 implementation of xy_hal_gpio API
 *
 * STM32U5 EXTI lines map 1:1 to IRQs (EXTI0_IRQn..EXTI15_IRQn), unlike F4
 * which groups 5..9 and 10..15.
 */

#include "../../inc/xy_hal_gpio.h"
#include "../../inc/xy_hal.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* IRQ context only needed when EXTI*_IRQn is available. */
#ifdef EXTI0_IRQn
typedef struct {
    xy_hal_gpio_irq_handler_t handler;
    void *arg;
} pin_irq_ctx_t;

static pin_irq_ctx_t g_pin_irq_ctx[16];
#endif

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

/* STM32U5 has one IRQ per EXTI line. The minimal SDK included in-tree may
 * not define EXTI*_IRQn; in that case the IRQ entry points below return
 * XY_HAL_ERROR_NOT_SUPPORTED rather than failing to link. */
#ifdef EXTI0_IRQn
#define XY_GPIO_HAS_EXTI_IRQ 1
static IRQn_Type pin_to_irqn(uint8_t pin)
{
    return (IRQn_Type)(EXTI0_IRQn + pin);
}
#else
#define XY_GPIO_HAS_EXTI_IRQ 0
#endif

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

#if XY_GPIO_HAS_EXTI_IRQ
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

#else /* !XY_GPIO_HAS_EXTI_IRQ — SDK lacks EXTI*_IRQn defs */

xy_hal_error_t xy_hal_gpio_attach_irq(xy_hal_gpio_port_t port, uint8_t pin,
                                      xy_hal_gpio_irq_mode_t mode,
                                      xy_hal_gpio_irq_handler_t handler,
                                      void *arg)
{
    (void)port; (void)pin; (void)mode; (void)handler; (void)arg;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_detach_irq(xy_hal_gpio_port_t port, uint8_t pin)
{
    (void)port; (void)pin;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_irq_enable(xy_hal_gpio_port_t port, uint8_t pin)
{
    (void)port; (void)pin;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_irq_disable(xy_hal_gpio_port_t port, uint8_t pin)
{
    (void)port; (void)pin;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

#endif /* XY_GPIO_HAS_EXTI_IRQ */

#endif /* STM32U5 || STM32U5xx */
