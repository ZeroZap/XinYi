#include "xy_actuator_rgb_led.h"

#define XY_ACTUATOR_RGB_COLOR_MASK 0x07U

static actuator_err_t map_gpio_error(xy_hal_error_t error)
{
    switch (error) {
    case XY_HAL_OK:
        return ACTUATOR_EOK;
    case XY_HAL_ERROR_INVALID_PARAM:
        return ACTUATOR_EINVAL;
    case XY_HAL_ERROR_TIMEOUT:
        return ACTUATOR_ETIMEOUT;
    case XY_HAL_ERROR_BUSY:
        return ACTUATOR_EBUSY;
    case XY_HAL_ERROR_NOT_SUPPORTED:
        return ACTUATOR_ENOSYS;
    default:
        return ACTUATOR_EIO;
    }
}

static uint8_t inactive_level(const xy_actuator_rgb_led_t *led)
{
    return led->active_low ? 1U : 0U;
}

static uint8_t channel_level(const xy_actuator_rgb_led_t *led, bool enabled)
{
    return (uint8_t)(enabled != led->active_low);
}

static void attempt_safe_off(xy_actuator_rgb_led_t *led)
{
    uint8_t level;

    if (led == NULL || led->port == NULL) {
        return;
    }
    level = inactive_level(led);
    if (led->red_pin <= 15U) {
        (void)xy_hal_gpio_write(led->port, led->red_pin, level);
    }
    if (led->green_pin <= 15U) {
        (void)xy_hal_gpio_write(led->port, led->green_pin, level);
    }
    if (led->blue_pin <= 15U) {
        (void)xy_hal_gpio_write(led->port, led->blue_pin, level);
    }
    led->color = XY_ACTUATOR_RGB_OFF;
}

static actuator_err_t write_color(xy_actuator_rgb_led_t *led, xy_actuator_rgb_color_t color)
{
    actuator_err_t result;

    /* Disable all channels before enabling a new combination. */
    result = map_gpio_error(xy_hal_gpio_write(led->port, led->red_pin, inactive_level(led)));
    if (result != ACTUATOR_EOK) {
        attempt_safe_off(led);
        return result;
    }
    result = map_gpio_error(xy_hal_gpio_write(led->port, led->green_pin, inactive_level(led)));
    if (result != ACTUATOR_EOK) {
        attempt_safe_off(led);
        return result;
    }
    result = map_gpio_error(xy_hal_gpio_write(led->port, led->blue_pin, inactive_level(led)));
    if (result != ACTUATOR_EOK) {
        attempt_safe_off(led);
        return result;
    }

    if ((color & XY_ACTUATOR_RGB_RED) != 0U) {
        result = map_gpio_error(xy_hal_gpio_write(led->port, led->red_pin,
                                                 channel_level(led, true)));
        if (result != ACTUATOR_EOK) {
            attempt_safe_off(led);
            return result;
        }
    }
    if ((color & XY_ACTUATOR_RGB_GREEN) != 0U) {
        result = map_gpio_error(xy_hal_gpio_write(led->port, led->green_pin,
                                                 channel_level(led, true)));
        if (result != ACTUATOR_EOK) {
            attempt_safe_off(led);
            return result;
        }
    }
    if ((color & XY_ACTUATOR_RGB_BLUE) != 0U) {
        result = map_gpio_error(xy_hal_gpio_write(led->port, led->blue_pin,
                                                 channel_level(led, true)));
        if (result != ACTUATOR_EOK) {
            attempt_safe_off(led);
            return result;
        }
    }
    led->color = color;
    return ACTUATOR_EOK;
}

actuator_err_t xy_actuator_rgb_led_init(xy_actuator_rgb_led_t *led)
{
    xy_hal_gpio_config_t config = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_NONE,
        .otype = XY_HAL_GPIO_OTYPE_PP,
        .speed = XY_HAL_GPIO_SPEED_LOW,
        .alternate = 0U,
    };
    const uint8_t pins[3] = {led == NULL ? 0U : led->red_pin,
                             led == NULL ? 0U : led->green_pin,
                             led == NULL ? 0U : led->blue_pin};

    if (led == NULL || led->port == NULL || led->red_pin > 15U || led->green_pin > 15U ||
        led->blue_pin > 15U || led->red_pin == led->green_pin || led->red_pin == led->blue_pin ||
        led->green_pin == led->blue_pin || led->delay_ms == NULL) {
        return ACTUATOR_EINVAL;
    }

    led->initialized = false;
    attempt_safe_off(led);
    for (size_t i = 0U; i < 3U; ++i) {
        actuator_err_t result = map_gpio_error(xy_hal_gpio_init(led->port, pins[i], &config));
        if (result != ACTUATOR_EOK) {
            attempt_safe_off(led);
            return result;
        }
    }
    attempt_safe_off(led);
    led->initialized = true;
    return ACTUATOR_EOK;
}

actuator_err_t xy_actuator_rgb_led_set(xy_actuator_rgb_led_t *led,
                                       xy_actuator_rgb_color_t color)
{
    if (led == NULL || !led->initialized || ((uint32_t)color & ~XY_ACTUATOR_RGB_COLOR_MASK) != 0U) {
        return ACTUATOR_EINVAL;
    }
    return write_color(led, color);
}

actuator_err_t xy_actuator_rgb_led_off(xy_actuator_rgb_led_t *led)
{
    return xy_actuator_rgb_led_set(led, XY_ACTUATOR_RGB_OFF);
}

actuator_err_t xy_actuator_rgb_led_play(xy_actuator_rgb_led_t *led,
                                        const xy_actuator_rgb_led_step_t *steps,
                                        size_t step_count)
{
    actuator_err_t result;

    if (led == NULL || !led->initialized || steps == NULL || step_count == 0U) {
        return ACTUATOR_EINVAL;
    }
    for (size_t i = 0U; i < step_count; ++i) {
        if (steps[i].duration_ms == 0U ||
            ((uint32_t)steps[i].color & ~XY_ACTUATOR_RGB_COLOR_MASK) != 0U) {
            return ACTUATOR_EINVAL;
        }
    }
    for (size_t i = 0U; i < step_count; ++i) {
        result = write_color(led, steps[i].color);
        if (result != ACTUATOR_EOK) {
            attempt_safe_off(led);
            return result;
        }
        led->delay_ms(steps[i].duration_ms);
    }
    result = write_color(led, XY_ACTUATOR_RGB_OFF);
    if (result != ACTUATOR_EOK) {
        attempt_safe_off(led);
    }
    return result;
}
