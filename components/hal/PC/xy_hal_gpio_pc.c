/**
 * @file xy_hal_gpio_pc.c
 * @brief PC simulation implementation of xy_hal_gpio API
 *
 * Provides a stateful, in-memory GPIO model so device-layer code can
 * exercise the gpio interface without real hardware.
 */

#include "xy_hal_gpio.h"
#include "xy_hal_error.h"
#include "xy_hal_pc.h"
#include <stdio.h>
#include <string.h>

#define PC_GPIO_MAX_PINS 256

typedef struct {
    uint8_t value;
    uint8_t mode;
    uint8_t pull;
    uint8_t otype;
    uint8_t configured;
    xy_hal_gpio_irq_handler_t irq_handler;
    void *irq_arg;
    uint8_t irq_mode;
    uint8_t irq_enabled;
} pc_pin_state_t;

static pc_pin_state_t g_pin_states[PC_GPIO_MAX_PINS];

static inline int pin_index(xy_hal_gpio_port_t port, uint8_t pin)
{
    uintptr_t port_id = (uintptr_t)port;
    return (int)((port_id & 0xF) * 16u + (pin & 0xF));
}

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    if (config == NULL || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    int idx = pin_index(port, pin);
    g_pin_states[idx].mode = (uint8_t)config->mode;
    g_pin_states[idx].pull = (uint8_t)config->pull;
    g_pin_states[idx].otype = (uint8_t)config->otype;
    g_pin_states[idx].value = 0;
    g_pin_states[idx].configured = 1;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_deinit(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    int idx = pin_index(port, pin);
    memset(&g_pin_states[idx], 0, sizeof(pc_pin_state_t));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value)
{
    if (pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    g_pin_states[pin_index(port, pin)].value = value ? 1 : 0;
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return g_pin_states[pin_index(port, pin)].value;
}

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    int idx = pin_index(port, pin);
    g_pin_states[idx].value = !g_pin_states[idx].value;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_attach_irq(xy_hal_gpio_port_t port, uint8_t pin,
                                      xy_hal_gpio_irq_mode_t mode,
                                      xy_hal_gpio_irq_handler_t handler,
                                      void *arg)
{
    if (pin >= 16 || handler == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    int idx = pin_index(port, pin);
    g_pin_states[idx].irq_handler = handler;
    g_pin_states[idx].irq_arg = arg;
    g_pin_states[idx].irq_mode = (uint8_t)mode;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_detach_irq(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    int idx = pin_index(port, pin);
    g_pin_states[idx].irq_handler = NULL;
    g_pin_states[idx].irq_arg = NULL;
    g_pin_states[idx].irq_enabled = 0;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_irq_enable(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    g_pin_states[pin_index(port, pin)].irq_enabled = 1;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_irq_disable(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    g_pin_states[pin_index(port, pin)].irq_enabled = 0;
    return XY_HAL_OK;
}
