#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "xy_actuator.h"

extern const actuator_ops_t relay_default_ops;
extern const actuator_ops_t servo_default_ops;

static int g_init_calls;
static int g_deinit_calls;
static int g_write_calls;
static int g_enable_calls;
static actuator_value_t g_last_write;

static void reset_mock(void)
{
    g_init_calls = 0;
    g_deinit_calls = 0;
    g_write_calls = 0;
    g_enable_calls = 0;
    memset(&g_last_write, 0, sizeof(g_last_write));
}

static actuator_err_t mock_init(actuator_device_t *dev)
{
    assert(dev != NULL);
    g_init_calls++;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_deinit(actuator_device_t *dev)
{
    assert(dev != NULL);
    g_deinit_calls++;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_write(actuator_device_t *dev, const actuator_value_t *value)
{
    assert(dev != NULL);
    assert(value != NULL);
    g_write_calls++;
    g_last_write = *value;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_read(actuator_device_t *dev, actuator_value_t *value)
{
    assert(dev != NULL);
    assert(value != NULL);
    *value = dev->value;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_enable(actuator_device_t *dev, bool enable)
{
    assert(dev != NULL);
    g_enable_calls++;
    dev->status = enable ? ACTUATOR_STATUS_READY : ACTUATOR_STATUS_DISABLED;
    return ACTUATOR_EOK;
}

static const actuator_ops_t mock_ops = {
    .init = mock_init,
    .deinit = mock_deinit,
    .write = mock_write,
    .read = mock_read,
    .enable = mock_enable,
};

static void test_strings_and_helpers(void)
{
    assert(strcmp(actuator_type_str(ACTUATOR_TYPE_RELAY), "Relay") == 0);
    assert(strcmp(actuator_type_str(ACTUATOR_TYPE_SERVO), "Servo") == 0);
    assert(strcmp(actuator_type_str((actuator_type_t)0x7e), "Unknown") == 0);

    assert(strcmp(actuator_status_str(ACTUATOR_STATUS_READY), "Ready") == 0);
    assert(strcmp(actuator_status_str((actuator_status_t)0x7e), "Unknown") == 0);

    assert(strcmp(actuator_err_str(ACTUATOR_EOK), "OK") == 0);
    assert(strcmp(actuator_err_str(ACTUATOR_ENOSYS), "Not implemented") == 0);
    assert(strcmp(actuator_err_str((actuator_err_t)-99), "Unknown") == 0);

    assert(servo_angle_to_pwm(-90.0f, -90.0f, 90.0f, 500, 2500) == 500);
    assert(servo_angle_to_pwm(0.0f, -90.0f, 90.0f, 500, 2500) == 1500);
    assert(servo_angle_to_pwm(90.0f, -90.0f, 90.0f, 500, 2500) == 2500);
    assert(fabsf(servo_pwm_to_angle(1500, -90.0f, 90.0f, 500, 2500)) < 0.01f);
}

static void test_registration_lifecycle_and_generic_io(void)
{
    reset_mock();

    actuator_device_t relay = ACTUATOR_DEVICE_INIT("act_relay", ACTUATOR_TYPE_RELAY, &mock_ops, NULL, NULL);
    actuator_device_t servo = ACTUATOR_DEVICE_INIT("act_servo", ACTUATOR_TYPE_SERVO, &mock_ops, NULL, NULL);

    assert(actuator_register(NULL) == ACTUATOR_EINVAL);
    assert(actuator_register(&relay) == ACTUATOR_EOK);
    assert(relay.id == 0);
    assert(relay.status == ACTUATOR_STATUS_READY);
    assert(actuator_register(&relay) == ACTUATOR_EINVAL);
    assert(actuator_register(&servo) == ACTUATOR_EOK);
    assert(actuator_get_count() >= 2);
    assert(actuator_get_count_by_type(ACTUATOR_TYPE_RELAY) >= 1);
    assert(actuator_find("act_relay") == &relay);
    assert(actuator_find_by_type(ACTUATOR_TYPE_SERVO) == &servo);

    assert(actuator_init(&relay) == ACTUATOR_EOK);
    assert(g_init_calls == 1);
    assert(actuator_enable(&relay, false) == ACTUATOR_EOK);
    assert(g_enable_calls == 1);
    assert(relay.status == ACTUATOR_STATUS_DISABLED);

    actuator_value_t value = {0};
    value.relay.state = RELAY_STATE_ON;
    assert(actuator_write(&relay, &value) == ACTUATOR_EOK);
    assert(g_write_calls == 1);
    assert(g_last_write.relay.state == RELAY_STATE_ON);
    assert(relay.status == ACTUATOR_STATUS_READY);

    actuator_value_t readback = {0};
    assert(actuator_read(&relay, &readback) == ACTUATOR_EOK);
    assert(readback.relay.state == RELAY_STATE_ON);

    assert(actuator_deinit(&relay) == ACTUATOR_EOK);
    assert(g_deinit_calls == 1);
    assert(relay.status == ACTUATOR_STATUS_IDLE);
    assert(actuator_unregister(&relay) == ACTUATOR_EOK);
    assert(actuator_find("act_relay") == NULL);
    assert(actuator_unregister(&servo) == ACTUATOR_EOK);
}

static void test_missing_ops_and_default_relay(void)
{
    actuator_device_t missing = ACTUATOR_DEVICE_INIT("missing_ops", ACTUATOR_TYPE_PWM, NULL, NULL, NULL);
    actuator_value_t value = {0};

    assert(actuator_write(NULL, &value) == ACTUATOR_EINVAL);
    assert(actuator_write(&missing, NULL) == ACTUATOR_EINVAL);
    assert(actuator_write(&missing, &value) == ACTUATOR_ENOSYS);
    assert(actuator_read(&missing, &value) == ACTUATOR_ENOSYS);

    actuator_device_t relay = ACTUATOR_DEVICE_INIT("relay_default", ACTUATOR_TYPE_RELAY, &relay_default_ops, NULL, NULL);
    relay.config.active_high = true;

    assert(relay_init(&relay) == ACTUATOR_EOK);
    assert(relay_get(&relay, &value.relay.state) == ACTUATOR_EOK);
    assert(value.relay.state == RELAY_STATE_OFF);
    assert(relay_on(&relay) == ACTUATOR_EOK);
    assert(relay_get(&relay, &value.relay.state) == ACTUATOR_EOK);
    assert(value.relay.state == RELAY_STATE_ON);
    assert(relay_toggle(&relay) == ACTUATOR_EOK);
    assert(relay_get(&relay, &value.relay.state) == ACTUATOR_EOK);
    assert(value.relay.state == RELAY_STATE_OFF);
    assert(relay_pulse(&relay, 10) == ACTUATOR_EOK);
    assert(relay_get(&relay, &value.relay.state) == ACTUATOR_EOK);
    assert(value.relay.state == RELAY_STATE_OFF);
}

static void test_default_servo_pwm_and_batch_helpers(void)
{
    actuator_device_t relay = ACTUATOR_DEVICE_INIT("batch_relay", ACTUATOR_TYPE_RELAY, &relay_default_ops, NULL, NULL);
    actuator_device_t servo = ACTUATOR_DEVICE_INIT("servo_default", ACTUATOR_TYPE_SERVO, &servo_default_ops, NULL, NULL);
    actuator_device_t pwm = ACTUATOR_DEVICE_INIT("pwm_default", ACTUATOR_TYPE_PWM, NULL, NULL, NULL);

    servo.config.servo_min_angle = -90.0f;
    servo.config.servo_max_angle = 90.0f;
    servo.config.servo_pwm_min = 500;
    servo.config.servo_pwm_max = 2500;

    assert(actuator_register(&relay) == ACTUATOR_EOK);
    assert(actuator_register(&servo) == ACTUATOR_EOK);
    assert(actuator_register(&pwm) == ACTUATOR_EOK);

    assert(servo_init(&servo) == ACTUATOR_EOK);
    assert(servo_set_angle(&servo, 200.0f) == ACTUATOR_EOK);
    assert(fabsf(servo.value.servo.current_angle - 90.0f) < 0.01f);
    assert(servo_set_range(&servo, -45.0f, 45.0f) == ACTUATOR_EOK);
    assert(servo_set_range(&servo, 45.0f, -45.0f) == ACTUATOR_EINVAL);
    assert(servo_set_speed(&servo, 120) == ACTUATOR_EOK);
    assert(servo_center(&servo) == ACTUATOR_EOK);
    assert(fabsf(servo.value.servo.current_angle) < 0.01f);

    assert(pwm_set_duty(&pwm, 1234) == ACTUATOR_EOK);
    assert(pwm.value.pwm.duty == 1234);
    assert(pwm_set_frequency(&pwm, 1000) == ACTUATOR_EOK);
    assert(pwm.config.pwm_freq == 1000);

    assert(relay_on(&relay) == ACTUATOR_EOK);
    assert(actuator_all_off() == ACTUATOR_EOK);
    assert(relay.value.relay.state == RELAY_STATE_OFF);
    assert(pwm.value.pwm.duty == 0);
    assert(actuator_emergency_stop_all() == ACTUATOR_EOK);

    assert(actuator_unregister(&relay) == ACTUATOR_EOK);
    assert(actuator_unregister(&servo) == ACTUATOR_EOK);
    assert(actuator_unregister(&pwm) == ACTUATOR_EOK);
}

int main(void)
{
    test_strings_and_helpers();
    test_registration_lifecycle_and_generic_io();
    test_missing_ops_and_default_relay();
    test_default_servo_pwm_and_batch_helpers();
    return 0;
}
