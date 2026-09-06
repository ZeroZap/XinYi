#include "xy_actuator_motor.h"

#define XY_ACTUATOR_MOTOR_MAX_STEP_MS 5000U

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

static bool mode_valid(xy_actuator_motor_mode_t mode)
{
    return (unsigned)mode <= (unsigned)XY_ACTUATOR_MOTOR_BRAKE;
}

static void mode_levels(xy_actuator_motor_mode_t mode, uint8_t *ina, uint8_t *inb)
{
    *ina = (mode == XY_ACTUATOR_MOTOR_FORWARD || mode == XY_ACTUATOR_MOTOR_BRAKE) ? 1U : 0U;
    *inb = (mode == XY_ACTUATOR_MOTOR_REVERSE || mode == XY_ACTUATOR_MOTOR_BRAKE) ? 1U : 0U;
}

static void attempt_standby(xy_actuator_motor_t *motor)
{
    if (motor == NULL) {
        return;
    }
    if (motor->ina_port != NULL && motor->ina_pin <= 15U) {
        (void)xy_hal_gpio_write(motor->ina_port, motor->ina_pin, 0U);
    }
    if (motor->inb_port != NULL && motor->inb_pin <= 15U) {
        (void)xy_hal_gpio_write(motor->inb_port, motor->inb_pin, 0U);
    }
    motor->mode = XY_ACTUATOR_MOTOR_STANDBY;
}

static actuator_err_t write_mode(xy_actuator_motor_t *motor, xy_actuator_motor_mode_t mode)
{
    uint8_t ina;
    uint8_t inb;
    actuator_err_t result;

    mode_levels(mode, &ina, &inb);
    result = map_gpio_error(xy_hal_gpio_write(motor->ina_port, motor->ina_pin, ina));
    if (result != ACTUATOR_EOK) {
        attempt_standby(motor);
        return result;
    }
    result = map_gpio_error(xy_hal_gpio_write(motor->inb_port, motor->inb_pin, inb));
    if (result != ACTUATOR_EOK) {
        attempt_standby(motor);
        return result;
    }
    motor->mode = mode;
    return ACTUATOR_EOK;
}

actuator_err_t xy_actuator_motor_init(xy_actuator_motor_t *motor)
{
    const xy_hal_gpio_config_t config = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_DOWN,
        .otype = XY_HAL_GPIO_OTYPE_PP,
        .speed = XY_HAL_GPIO_SPEED_LOW,
        .alternate = 0U,
    };
    actuator_err_t result;

    if (motor == NULL || motor->ina_port == NULL || motor->inb_port == NULL ||
        motor->ina_pin > 15U || motor->inb_pin > 15U || motor->delay_ms == NULL) {
        return ACTUATOR_EINVAL;
    }

    motor->initialized = false;
    attempt_standby(motor);
    result = map_gpio_error(xy_hal_gpio_init(motor->ina_port, motor->ina_pin, &config));
    if (result != ACTUATOR_EOK) {
        attempt_standby(motor);
        return result;
    }
    result = map_gpio_error(xy_hal_gpio_init(motor->inb_port, motor->inb_pin, &config));
    if (result != ACTUATOR_EOK) {
        attempt_standby(motor);
        return result;
    }
    result = write_mode(motor, XY_ACTUATOR_MOTOR_STANDBY);
    if (result != ACTUATOR_EOK) {
        return result;
    }
    motor->initialized = true;
    return ACTUATOR_EOK;
}

actuator_err_t xy_actuator_motor_set_mode(xy_actuator_motor_t *motor,
                                          xy_actuator_motor_mode_t mode)
{
    actuator_err_t result;

    if (motor == NULL || !motor->initialized || !mode_valid(mode)) {
        return ACTUATOR_EINVAL;
    }
    if (mode == motor->mode) {
        return ACTUATOR_EOK;
    }
    if (motor->mode != XY_ACTUATOR_MOTOR_STANDBY && mode != XY_ACTUATOR_MOTOR_STANDBY) {
        result = write_mode(motor, XY_ACTUATOR_MOTOR_STANDBY);
        if (result != ACTUATOR_EOK) {
            return result;
        }
        if (motor->break_before_make_ms > 0U) {
            motor->delay_ms(motor->break_before_make_ms);
        }
    }
    return write_mode(motor, mode);
}

actuator_err_t xy_actuator_motor_standby(xy_actuator_motor_t *motor)
{
    return xy_actuator_motor_set_mode(motor, XY_ACTUATOR_MOTOR_STANDBY);
}

actuator_err_t xy_actuator_motor_forward_pulse(xy_actuator_motor_t *motor,
                                               uint32_t duration_ms)
{
    const xy_actuator_motor_step_t step = {XY_ACTUATOR_MOTOR_FORWARD, duration_ms};
    return xy_actuator_motor_play(motor, &step, 1U);
}

actuator_err_t xy_actuator_motor_play(xy_actuator_motor_t *motor,
                                      const xy_actuator_motor_step_t *steps,
                                      size_t step_count)
{
    actuator_err_t result;

    if (motor == NULL || !motor->initialized || steps == NULL || step_count == 0U) {
        return ACTUATOR_EINVAL;
    }
    for (size_t i = 0U; i < step_count; ++i) {
        if (!mode_valid(steps[i].mode) || steps[i].duration_ms == 0U ||
            steps[i].duration_ms > XY_ACTUATOR_MOTOR_MAX_STEP_MS) {
            return ACTUATOR_EINVAL;
        }
    }
    for (size_t i = 0U; i < step_count; ++i) {
        result = xy_actuator_motor_set_mode(motor, steps[i].mode);
        if (result != ACTUATOR_EOK) {
            attempt_standby(motor);
            return result;
        }
        motor->delay_ms(steps[i].duration_ms);
    }
    result = xy_actuator_motor_standby(motor);
    if (result != ACTUATOR_EOK) {
        attempt_standby(motor);
    }
    return result;
}