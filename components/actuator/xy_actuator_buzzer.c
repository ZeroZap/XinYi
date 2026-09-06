#include "xy_actuator_buzzer.h"

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

static actuator_err_t write_state(xy_actuator_buzzer_t *buzzer, bool on)
{
    uint8_t level = (uint8_t)(on == buzzer->active_high);
    actuator_err_t result = map_gpio_error(xy_hal_gpio_write(buzzer->port, buzzer->pin, level));
    if (result == ACTUATOR_EOK) {
        buzzer->is_on = on;
    }
    return result;
}

static void attempt_safe_off(xy_actuator_buzzer_t *buzzer)
{
    if (buzzer != NULL && buzzer->port != NULL && buzzer->pin <= 15U) {
        (void)xy_hal_gpio_write(buzzer->port, buzzer->pin,
                                buzzer->active_high ? 0U : 1U);
        buzzer->is_on = false;
    }
}

actuator_err_t xy_actuator_buzzer_init(xy_actuator_buzzer_t *buzzer)
{
    xy_hal_gpio_config_t config = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_NONE,
        .otype = XY_HAL_GPIO_OTYPE_PP,
        .speed = XY_HAL_GPIO_SPEED_LOW,
        .alternate = 0U,
    };
    actuator_err_t result;

    if (buzzer == NULL || buzzer->port == NULL || buzzer->pin > 15U ||
        buzzer->delay_ms == NULL) {
        return ACTUATOR_EINVAL;
    }

    buzzer->initialized = false;
    attempt_safe_off(buzzer);
    result = map_gpio_error(xy_hal_gpio_init(buzzer->port, buzzer->pin, &config));
    if (result != ACTUATOR_EOK) {
        attempt_safe_off(buzzer);
        return result;
    }
    result = write_state(buzzer, false);
    if (result != ACTUATOR_EOK) {
        attempt_safe_off(buzzer);
        return result;
    }
    buzzer->initialized = true;
    return ACTUATOR_EOK;
}

actuator_err_t xy_actuator_buzzer_on(xy_actuator_buzzer_t *buzzer)
{
    if (buzzer == NULL || !buzzer->initialized) {
        return ACTUATOR_EINVAL;
    }
    return write_state(buzzer, true);
}

actuator_err_t xy_actuator_buzzer_off(xy_actuator_buzzer_t *buzzer)
{
    if (buzzer == NULL || !buzzer->initialized) {
        return ACTUATOR_EINVAL;
    }
    return write_state(buzzer, false);
}

actuator_err_t xy_actuator_buzzer_pulse(xy_actuator_buzzer_t *buzzer, uint32_t duration_ms)
{
    xy_actuator_buzzer_step_t step = {true, duration_ms};
    return xy_actuator_buzzer_play(buzzer, &step, 1U);
}

actuator_err_t xy_actuator_buzzer_play(xy_actuator_buzzer_t *buzzer,
                                       const xy_actuator_buzzer_step_t *steps,
                                       size_t step_count)
{
    actuator_err_t result;

    if (buzzer == NULL || !buzzer->initialized || steps == NULL || step_count == 0U) {
        return ACTUATOR_EINVAL;
    }
    for (size_t i = 0U; i < step_count; ++i) {
        if (steps[i].duration_ms == 0U) {
            return ACTUATOR_EINVAL;
        }
    }

    for (size_t i = 0U; i < step_count; ++i) {
        result = write_state(buzzer, steps[i].on);
        if (result != ACTUATOR_EOK) {
            attempt_safe_off(buzzer);
            return result;
        }
        buzzer->delay_ms(steps[i].duration_ms);
    }

    result = write_state(buzzer, false);
    if (result != ACTUATOR_EOK) {
        attempt_safe_off(buzzer);
    }
    return result;
}
