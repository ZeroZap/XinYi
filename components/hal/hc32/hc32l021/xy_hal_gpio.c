/**
 * @file xy_hal_gpio.c
 * @brief HC32L021 implementation of xy_hal_gpio API.
 *
 * Wraps the vendor `GPIO_Init` plus direct BSET/BCLR/IN register access.
 * IRQ attach/detach are no-ops on HC32L021 because per-pin EXTI dispatch
 * lives in the vendor interrupt handler and isn't wired through this HAL.
 */

#include "../../inc/xy_hal_gpio.h"
#include "../../inc/xy_hal.h"

#if defined(HC32L021) || defined(XY_HAL_HC32L021)

#include "hc32l021_gpio.h"
#include "hc32l021.h"
#include <string.h>

static uint32_t xy_to_hc32_mode(xy_hal_gpio_mode_t mode, xy_hal_gpio_otype_t otype)
{
    switch (mode) {
    case XY_HAL_GPIO_MODE_INPUT:  return GPIO_MD_INPUT;
    case XY_HAL_GPIO_MODE_OUTPUT: return otype == XY_HAL_GPIO_OTYPE_OD
                                       ? GPIO_MD_OUTPUT_OD : GPIO_MD_OUTPUT_PP;
    case XY_HAL_GPIO_MODE_ANALOG: return GPIO_MD_ANALOG;
    case XY_HAL_GPIO_MODE_AF:     return GPIO_MD_OUTPUT_PP; /* AF needs PSEL setup elsewhere */
    default:                      return GPIO_MD_INPUT;
    }
}

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    if (!port || !config || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    stc_gpio_init_t init = { 0 };
    init.u32Pin       = (1U << pin);
    init.u32Mode      = xy_to_hc32_mode(config->mode, config->otype);
    init.u32PullUp    = (config->pull == XY_HAL_GPIO_PULL_UP) ? GPIO_PULL_UP : GPIO_PULL_NONE;
    init.u32ExternInt = GPIO_EXTI_NONE;

    GPIO_Init((GPIO_TypeDef *)port, &init);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_deinit(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    /* Reset to high-impedance input (vendor library has no explicit DeInit). */
    stc_gpio_init_t init = { 0 };
    init.u32Pin   = (1U << pin);
    init.u32Mode  = GPIO_MD_INPUT;
    GPIO_Init((GPIO_TypeDef *)port, &init);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    GPIO_TypeDef *p = (GPIO_TypeDef *)port;
    if (value) {
        p->BSET = (1U << pin);
    } else {
        p->BCLR = (1U << pin);
    }
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return (((GPIO_TypeDef *)port)->IN >> pin) & 0x1u;
}

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    GPIO_TypeDef *p = (GPIO_TypeDef *)port;
    p->OUT ^= (1U << pin);
    return XY_HAL_OK;
}

/* IRQ paths are not wired on HC32L021 — pin EXTI dispatch lives in the
 * vendor interrupt handlers, not in this HAL. Return NOT_SUPPORTED so
 * callers can detect the gap rather than getting silent success. */

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

#endif /* HC32L021 */
