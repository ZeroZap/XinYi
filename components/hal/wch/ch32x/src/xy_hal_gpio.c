/**
 * @file xy_hal_gpio.c
 * @brief WCH CH32V30x GPIO HAL implementation
 */

#include "xy_hal_gpio.h"

#ifdef MCU_CH32

#include "ch32v30x.h"

#define XY_WCH_GPIO_PORT_COUNT 5U
#define XY_WCH_GPIO_PIN_COUNT 16U

typedef struct {
    xy_hal_gpio_mode_t mode;
    xy_hal_gpio_pull_t pull;
    xy_hal_gpio_otype_t otype;
    xy_hal_gpio_speed_t speed;
    uint8_t alternate;
    uint8_t configured;
} xy_wch_gpio_state_t;

static xy_wch_gpio_state_t gpio_state[XY_WCH_GPIO_PORT_COUNT][XY_WCH_GPIO_PIN_COUNT];

static int gpio_port_index(xy_hal_gpio_port_t port) {
    if (port == GPIOA)
        return 0;
    if (port == GPIOB)
        return 1;
    if (port == GPIOC)
        return 2;
    if (port == GPIOD)
        return 3;
    if (port == GPIOE)
        return 4;
    return -1;
}

static xy_hal_error_t gpio_validate(xy_hal_gpio_port_t port, uint8_t pin) {
    return (gpio_port_index(port) >= 0 && pin < XY_WCH_GPIO_PIN_COUNT) ? XY_HAL_OK
                                                                       : XY_HAL_ERROR_INVALID_PARAM;
}

static void gpio_enable_clock(xy_hal_gpio_port_t port) {
    static const uint32_t clocks[] = {
        RCC_APB2Periph_GPIOA, RCC_APB2Periph_GPIOB, RCC_APB2Periph_GPIOC,
        RCC_APB2Periph_GPIOD, RCC_APB2Periph_GPIOE,
    };
    RCC_APB2PeriphClockCmd(clocks[gpio_port_index(port)], ENABLE);
}

static GPIOSpeed_TypeDef gpio_speed(xy_hal_gpio_speed_t speed) {
    if (speed == XY_HAL_GPIO_SPEED_LOW)
        return GPIO_Speed_2MHz;
    if (speed == XY_HAL_GPIO_SPEED_MEDIUM)
        return GPIO_Speed_10MHz;
    return GPIO_Speed_50MHz;
}

static GPIOMode_TypeDef gpio_mode(const xy_hal_gpio_config_t* config) {
    if (config->mode == XY_HAL_GPIO_MODE_ANALOG)
        return GPIO_Mode_AIN;
    if (config->mode == XY_HAL_GPIO_MODE_INPUT) {
        if (config->pull == XY_HAL_GPIO_PULL_UP)
            return GPIO_Mode_IPU;
        if (config->pull == XY_HAL_GPIO_PULL_DOWN)
            return GPIO_Mode_IPD;
        return GPIO_Mode_IN_FLOATING;
    }
    if (config->mode == XY_HAL_GPIO_MODE_OUTPUT) {
        return config->otype == XY_HAL_GPIO_OTYPE_OD ? GPIO_Mode_Out_OD : GPIO_Mode_Out_PP;
    }
    if (config->mode == XY_HAL_GPIO_MODE_AF) {
        return config->otype == XY_HAL_GPIO_OTYPE_OD ? GPIO_Mode_AF_OD : GPIO_Mode_AF_PP;
    }
    return GPIO_Mode_IN_FLOATING;
}

static xy_hal_error_t gpio_apply(xy_hal_gpio_port_t port, uint8_t pin,
                                 const xy_hal_gpio_config_t* config) {
    GPIO_InitTypeDef init = {0};
    int index;

    if (gpio_validate(port, pin) != XY_HAL_OK || config == NULL ||
        config->mode > XY_HAL_GPIO_MODE_ANALOG || config->pull > XY_HAL_GPIO_PULL_DOWN ||
        config->otype > XY_HAL_GPIO_OTYPE_OD || config->speed > XY_HAL_GPIO_SPEED_VERY_HIGH) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    gpio_enable_clock(port);
    init.GPIO_Pin = (uint16_t)(1U << pin);
    init.GPIO_Mode = gpio_mode(config);
    init.GPIO_Speed = gpio_speed(config->speed);
    GPIO_Init(port, &init);

    index = gpio_port_index(port);
    gpio_state[index][pin].mode = config->mode;
    gpio_state[index][pin].pull = config->pull;
    gpio_state[index][pin].otype = config->otype;
    gpio_state[index][pin].speed = config->speed;
    gpio_state[index][pin].alternate = config->alternate;
    gpio_state[index][pin].configured = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t* config) {
    return gpio_apply(port, pin, config);
}

xy_hal_error_t xy_hal_gpio_deinit(xy_hal_gpio_port_t port, uint8_t pin) {
    GPIO_InitTypeDef init = {0};
    int index;
    if (gpio_validate(port, pin) != XY_HAL_OK)
        return XY_HAL_ERROR_INVALID_PARAM;
    init.GPIO_Pin = (uint16_t)(1U << pin);
    init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    init.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(port, &init);
    index = gpio_port_index(port);
    gpio_state[index][pin].configured = 0U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value) {
    if (gpio_validate(port, pin) != XY_HAL_OK || value > 1U)
        return XY_HAL_ERROR_INVALID_PARAM;
    GPIO_WriteBit(port, (uint16_t)(1U << pin), value ? Bit_SET : Bit_RESET);
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin) {
    if (gpio_validate(port, pin) != XY_HAL_OK)
        return XY_HAL_ERROR_INVALID_PARAM;
    return GPIO_ReadInputDataBit(port, (uint16_t)(1U << pin)) ? 1 : 0;
}

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_port_t port, uint8_t pin) {
    uint16_t mask;
    if (gpio_validate(port, pin) != XY_HAL_OK)
        return XY_HAL_ERROR_INVALID_PARAM;
    mask = (uint16_t)(1U << pin);
    GPIO_WriteBit(port, mask, GPIO_ReadOutputDataBit(port, mask) ? Bit_RESET : Bit_SET);
    return XY_HAL_OK;
}

static xy_hal_error_t gpio_update(xy_hal_gpio_port_t port, uint8_t pin, xy_hal_gpio_mode_t* mode,
                                  xy_hal_gpio_pull_t* pull, xy_hal_gpio_otype_t* otype,
                                  xy_hal_gpio_speed_t* speed) {
    xy_hal_gpio_config_t config;
    int index;
    if (gpio_validate(port, pin) != XY_HAL_OK)
        return XY_HAL_ERROR_INVALID_PARAM;
    index = gpio_port_index(port);
    if (!gpio_state[index][pin].configured)
        return XY_HAL_ERROR_NOT_INIT;
    config.mode = mode ? *mode : gpio_state[index][pin].mode;
    config.pull = pull ? *pull : gpio_state[index][pin].pull;
    config.otype = otype ? *otype : gpio_state[index][pin].otype;
    config.speed = speed ? *speed : gpio_state[index][pin].speed;
    config.alternate = gpio_state[index][pin].alternate;
    return gpio_apply(port, pin, &config);
}

xy_hal_error_t xy_hal_gpio_set_mode(xy_hal_gpio_port_t port, uint8_t pin, xy_hal_gpio_mode_t mode) {
    return gpio_update(port, pin, &mode, NULL, NULL, NULL);
}

int32_t xy_hal_gpio_get_mode(xy_hal_gpio_port_t port, uint8_t pin) {
    int index = gpio_port_index(port);
    if (index < 0 || pin >= XY_WCH_GPIO_PIN_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    return gpio_state[index][pin].configured ? (int32_t)gpio_state[index][pin].mode
                                             : XY_HAL_ERROR_NOT_INIT;
}

xy_hal_error_t xy_hal_gpio_set_pull(xy_hal_gpio_port_t port, uint8_t pin, xy_hal_gpio_pull_t pull) {
    return gpio_update(port, pin, NULL, &pull, NULL, NULL);
}

int32_t xy_hal_gpio_get_pull(xy_hal_gpio_port_t port, uint8_t pin) {
    int index = gpio_port_index(port);
    if (index < 0 || pin >= XY_WCH_GPIO_PIN_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    return gpio_state[index][pin].configured ? (int32_t)gpio_state[index][pin].pull
                                             : XY_HAL_ERROR_NOT_INIT;
}

xy_hal_error_t xy_hal_gpio_set_otype(xy_hal_gpio_port_t port, uint8_t pin,
                                     xy_hal_gpio_otype_t otype) {
    return gpio_update(port, pin, NULL, NULL, &otype, NULL);
}

int32_t xy_hal_gpio_get_otype(xy_hal_gpio_port_t port, uint8_t pin) {
    int index = gpio_port_index(port);
    if (index < 0 || pin >= XY_WCH_GPIO_PIN_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    return gpio_state[index][pin].configured ? (int32_t)gpio_state[index][pin].otype
                                             : XY_HAL_ERROR_NOT_INIT;
}

xy_hal_error_t xy_hal_gpio_set_speed(xy_hal_gpio_port_t port, uint8_t pin,
                                     xy_hal_gpio_speed_t speed) {
    return gpio_update(port, pin, NULL, NULL, NULL, &speed);
}

int32_t xy_hal_gpio_get_speed(xy_hal_gpio_port_t port, uint8_t pin) {
    int index = gpio_port_index(port);
    if (index < 0 || pin >= XY_WCH_GPIO_PIN_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    return gpio_state[index][pin].configured ? (int32_t)gpio_state[index][pin].speed
                                             : XY_HAL_ERROR_NOT_INIT;
}

xy_hal_error_t xy_hal_gpio_set_alternate(xy_hal_gpio_port_t port, uint8_t pin, uint8_t alternate) {
    int index = gpio_port_index(port);
    if (index < 0 || pin >= XY_WCH_GPIO_PIN_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!gpio_state[index][pin].configured)
        return XY_HAL_ERROR_NOT_INIT;
    gpio_state[index][pin].alternate = alternate;
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_get_alternate(xy_hal_gpio_port_t port, uint8_t pin) {
    int index = gpio_port_index(port);
    if (index < 0 || pin >= XY_WCH_GPIO_PIN_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    return gpio_state[index][pin].configured ? gpio_state[index][pin].alternate
                                             : XY_HAL_ERROR_NOT_INIT;
}

xy_hal_error_t xy_hal_gpio_write_batch(xy_hal_gpio_port_t port, uint16_t pin_mask,
                                       uint16_t value_mask) {
    if (gpio_port_index(port) < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    GPIO_Write(port, (uint16_t)((GPIO_ReadOutputData(port) & ~pin_mask) | (value_mask & pin_mask)));
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read_batch(xy_hal_gpio_port_t port, uint16_t pin_mask) {
    if (gpio_port_index(port) < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    return (int32_t)(GPIO_ReadInputData(port) & pin_mask);
}

#define GPIO_UNSUPPORTED_BODY(name, args)                                                          \
    xy_hal_error_t name args {                                                                     \
        return XY_HAL_ERROR_NOT_SUPPORTED;                                                         \
    }

GPIO_UNSUPPORTED_BODY(xy_hal_gpio_attach_irq,
                      (xy_hal_gpio_port_t port, uint8_t pin, xy_hal_gpio_irq_mode_t mode,
                       xy_hal_gpio_irq_handler_t handler, void* arg))
GPIO_UNSUPPORTED_BODY(xy_hal_gpio_detach_irq, (xy_hal_gpio_port_t port, uint8_t pin))
GPIO_UNSUPPORTED_BODY(xy_hal_gpio_irq_enable, (xy_hal_gpio_port_t port, uint8_t pin))
GPIO_UNSUPPORTED_BODY(xy_hal_gpio_irq_disable, (xy_hal_gpio_port_t port, uint8_t pin))
GPIO_UNSUPPORTED_BODY(xy_hal_gpio_clear_irq_status, (xy_hal_gpio_port_t port, uint8_t pin))
GPIO_UNSUPPORTED_BODY(xy_hal_gpio_control,
                      (xy_hal_gpio_port_t port, uint8_t pin, uint32_t cmd, void* args))
GPIO_UNSUPPORTED_BODY(xy_hal_gpio_set_sleep_state,
                      (xy_hal_gpio_port_t port, uint8_t pin, uint8_t sleep_state))
GPIO_UNSUPPORTED_BODY(xy_hal_gpio_get_driver_info, (xy_hal_gpio_port_t port, void* info))
GPIO_UNSUPPORTED_BODY(xy_hal_gpio_pinmux_config,
                      (xy_hal_gpio_port_t port, uint8_t pin, const void* config))
GPIO_UNSUPPORTED_BODY(xy_hal_gpio_set_drive_strength,
                      (xy_hal_gpio_port_t port, uint8_t pin, uint8_t strength))
GPIO_UNSUPPORTED_BODY(xy_hal_gpio_set_slew_rate,
                      (xy_hal_gpio_port_t port, uint8_t pin, uint8_t slew_rate))

int32_t xy_hal_gpio_get_irq_status(xy_hal_gpio_port_t port, uint8_t pin) {
    (void)port;
    (void)pin;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_gpio_get_sleep_state(xy_hal_gpio_port_t port, uint8_t pin) {
    (void)port;
    (void)pin;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_gpio_get_drive_strength(xy_hal_gpio_port_t port, uint8_t pin) {
    (void)port;
    (void)pin;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_gpio_get_slew_rate(xy_hal_gpio_port_t port, uint8_t pin) {
    (void)port;
    (void)pin;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
void xy_hal_gpio_irq_handler(xy_hal_gpio_port_t port, uint8_t pin) {
    (void)port;
    (void)pin;
}

#else
#error "WCH GPIO backend requires MCU_CH32"
#endif
