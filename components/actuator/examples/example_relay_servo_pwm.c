#include "xy_actuator.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern const actuator_ops_t relay_default_ops;

static actuator_value_t g_last_relay_write;
static actuator_value_t g_last_servo_write;
static actuator_value_t g_last_pwm_write;

static actuator_err_t example_write(actuator_device_t *dev, const actuator_value_t *value)
{
    if (dev == NULL || value == NULL) {
        return ACTUATOR_EINVAL;
    }

    switch (dev->type) {
    case ACTUATOR_TYPE_RELAY:
        g_last_relay_write = *value;
        break;
    case ACTUATOR_TYPE_SERVO:
        g_last_servo_write = *value;
        break;
    case ACTUATOR_TYPE_PWM:
        g_last_pwm_write = *value;
        break;
    default:
        return ACTUATOR_EINVAL;
    }

    return ACTUATOR_EOK;
}

static const actuator_ops_t example_ops = {
    .write = example_write,
};

int main(void)
{
    actuator_device_t relay = ACTUATOR_DEVICE_INIT("relay_demo", ACTUATOR_TYPE_RELAY,
                                                   &relay_default_ops, NULL, NULL);
    actuator_device_t servo = ACTUATOR_DEVICE_INIT("servo_demo", ACTUATOR_TYPE_SERVO, &example_ops,
                                                   NULL, NULL);
    actuator_device_t pwm = ACTUATOR_DEVICE_INIT("pwm_demo", ACTUATOR_TYPE_PWM,
                                                 &example_ops, NULL, NULL);

    servo.config.servo_min_angle = -90.0f;
    servo.config.servo_max_angle = 90.0f;
    servo.config.servo_pwm_min = 500U;
    servo.config.servo_pwm_max = 2500U;
    servo.config.servo_speed = 90U;

    if (actuator_register(&relay) != ACTUATOR_EOK) {
        return 1;
    }
    if (actuator_register(&servo) != ACTUATOR_EOK) {
        return 2;
    }
    if (actuator_register(&pwm) != ACTUATOR_EOK) {
        return 3;
    }

    if (relay_on(&relay) != ACTUATOR_EOK || relay.value.relay.state != RELAY_STATE_ON) {
        return 4;
    }

    if (servo_set_angle(&servo, 45.0f) != ACTUATOR_EOK) {
        return 5;
    }
    if (servo.value.servo.current_angle < 44.9f || servo.value.servo.current_angle > 45.1f) {
        return 6;
    }

    if (pwm_set_duty(&pwm, 32768U) != ACTUATOR_EOK || pwm.value.pwm.duty != 32768U ||
        g_last_pwm_write.pwm.duty != 32768U) {
        return 7;
    }
    if (pwm_set_frequency(&pwm, 1000U) != ACTUATOR_EOK || pwm.config.pwm_freq != 1000U) {
        return 8;
    }

    if (actuator_all_off() != ACTUATOR_EOK) {
        return 9;
    }
    if (relay.value.relay.state != RELAY_STATE_OFF || pwm.value.pwm.duty != 0U) {
        return 10;
    }

    if (actuator_unregister(&pwm) != ACTUATOR_EOK || actuator_unregister(&servo) != ACTUATOR_EOK ||
        actuator_unregister(&relay) != ACTUATOR_EOK) {
        return 11;
    }

    (void)g_last_relay_write;
    (void)g_last_servo_write;
    return actuator_get_count() == 0U ? 0 : 12;
}
