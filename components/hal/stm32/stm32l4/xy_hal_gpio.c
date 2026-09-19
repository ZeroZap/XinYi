#include "xy_hal_gpio.h"

#include "stm32l4xx_hal.h"

typedef struct {
    xy_hal_gpio_port_t port;
    xy_hal_gpio_irq_handler_t handler;
    void *arg;
} gpio_irq_context_t;

static gpio_irq_context_t gpio_irq_contexts[16];

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
    if (config->mode == XY_HAL_GPIO_MODE_IT_RISING) {
        return GPIO_MODE_IT_RISING;
    }
    if (config->mode == XY_HAL_GPIO_MODE_IT_FALLING) {
        return GPIO_MODE_IT_FALLING;
    }
    if (config->mode == XY_HAL_GPIO_MODE_IT_BOTH) {
        return GPIO_MODE_IT_RISING_FALLING;
    }
    return GPIO_MODE_INPUT;
}

static IRQn_Type gpio_irqn(uint8_t pin)
{
    if (pin <= 4U) {
        static const IRQn_Type irqs[] = {EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn,
                                         EXTI4_IRQn};
        return irqs[pin];
    }
    return pin <= 9U ? EXTI9_5_IRQn : EXTI15_10_IRQn;
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

xy_hal_error_t xy_hal_gpio_attach_irq(xy_hal_gpio_port_t port, uint8_t pin,
                                      xy_hal_gpio_irq_mode_t mode,
                                      xy_hal_gpio_irq_handler_t handler, void *arg)
{
    xy_hal_gpio_config_t config = {0};
    xy_hal_error_t result;

    if (port == NULL || pin > 15U || handler == NULL || mode > XY_HAL_GPIO_IRQ_BOTH) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    config.mode = mode == XY_HAL_GPIO_IRQ_RISING    ? XY_HAL_GPIO_MODE_IT_RISING
                  : mode == XY_HAL_GPIO_IRQ_FALLING ? XY_HAL_GPIO_MODE_IT_FALLING
                                                    : XY_HAL_GPIO_MODE_IT_BOTH;
    config.pull = XY_HAL_GPIO_PULL_DOWN;
    config.otype = XY_HAL_GPIO_OTYPE_PP;
    config.speed = XY_HAL_GPIO_SPEED_LOW;
    result = xy_hal_gpio_init(port, pin, &config);
    if (result != XY_HAL_OK) {
        return result;
    }
    gpio_irq_contexts[pin].port = port;
    gpio_irq_contexts[pin].handler = handler;
    gpio_irq_contexts[pin].arg = arg;
    __HAL_GPIO_EXTI_CLEAR_IT(1U << pin);
    HAL_NVIC_SetPriority(gpio_irqn(pin), 5U, 0U);
    HAL_NVIC_EnableIRQ(gpio_irqn(pin));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_detach_irq(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (port == NULL || pin > 15U || gpio_irq_contexts[pin].port != port) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    gpio_irq_contexts[pin].port = NULL;
    gpio_irq_contexts[pin].handler = NULL;
    gpio_irq_contexts[pin].arg = NULL;
    return xy_hal_gpio_deinit(port, pin);
}

xy_hal_error_t xy_hal_gpio_irq_enable(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (port == NULL || pin > 15U || gpio_irq_contexts[pin].port != port) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_NVIC_EnableIRQ(gpio_irqn(pin));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_irq_disable(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (port == NULL || pin > 15U || gpio_irq_contexts[pin].port != port) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    HAL_NVIC_DisableIRQ(gpio_irqn(pin));
    return XY_HAL_OK;
}

void xy_hal_gpio_irq_handler(xy_hal_gpio_port_t port, uint8_t pin)
{
    if (port == NULL || pin > 15U) {
        return;
    }
    HAL_GPIO_EXTI_IRQHandler(1U << pin);
    if (gpio_irq_contexts[pin].port == port && gpio_irq_contexts[pin].handler != NULL) {
        gpio_irq_contexts[pin].handler(gpio_irq_contexts[pin].arg);
    }
}
