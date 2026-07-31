#include "unity.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "fff.h"
#include "xy_actuator.h"

DEFINE_FFF_GLOBALS;

extern const actuator_ops_t relay_default_ops;
extern const actuator_ops_t servo_default_ops;

static actuator_value_t g_last_write;

FAKE_VALUE_FUNC(actuator_err_t, mock_init, actuator_device_t *);
FAKE_VALUE_FUNC(actuator_err_t, mock_deinit, actuator_device_t *);
FAKE_VALUE_FUNC(actuator_err_t, mock_write, actuator_device_t *, const actuator_value_t *);
FAKE_VALUE_FUNC(actuator_err_t, mock_read, actuator_device_t *, actuator_value_t *);
FAKE_VALUE_FUNC(actuator_err_t, mock_enable, actuator_device_t *, bool);

FAKE_VALUE_FUNC(actuator_err_t, relay_type_set, actuator_device_t *, uint8_t);
FAKE_VALUE_FUNC(actuator_err_t, relay_type_get, actuator_device_t *, uint8_t *);
FAKE_VALUE_FUNC(actuator_err_t, relay_type_toggle, actuator_device_t *);
FAKE_VALUE_FUNC(actuator_err_t, relay_type_pulse, actuator_device_t *, uint32_t);

FAKE_VALUE_FUNC(actuator_err_t, servo_type_set_angle, actuator_device_t *, float);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_get_angle, actuator_device_t *, float *);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_set_range, actuator_device_t *, float, float);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_set_speed, actuator_device_t *, uint32_t);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_sweep, actuator_device_t *, float, float, uint32_t);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_stop, actuator_device_t *);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_center, actuator_device_t *);

void setUp(void)
{
}

void tearDown(void)
{
}

static actuator_err_t mock_init_impl(actuator_device_t *dev)
{
    TEST_ASSERT_NOT_NULL(dev);
    return ACTUATOR_EOK;
}

static actuator_err_t mock_deinit_impl(actuator_device_t *dev)
{
    TEST_ASSERT_NOT_NULL(dev);
    return ACTUATOR_EOK;
}

static actuator_err_t mock_write_impl(actuator_device_t *dev, const actuator_value_t *value)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(value);
    g_last_write = *value;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_read_impl(actuator_device_t *dev, actuator_value_t *value)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(value);
    *value = dev->value;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_enable_impl(actuator_device_t *dev, bool enable)
{
    TEST_ASSERT_NOT_NULL(dev);
    dev->status = enable ? ACTUATOR_STATUS_READY : ACTUATOR_STATUS_DISABLED;
    return ACTUATOR_EOK;
}

static void reset_mock(void)
{
    memset(&g_last_write, 0, sizeof(g_last_write));
    RESET_FAKE(mock_init);
    RESET_FAKE(mock_deinit);
    RESET_FAKE(mock_write);
    RESET_FAKE(mock_read);
    RESET_FAKE(mock_enable);
    RESET_FAKE(relay_type_set);
    RESET_FAKE(relay_type_get);
    RESET_FAKE(relay_type_toggle);
    RESET_FAKE(relay_type_pulse);
    RESET_FAKE(servo_type_set_angle);
    RESET_FAKE(servo_type_get_angle);
    RESET_FAKE(servo_type_set_range);
    RESET_FAKE(servo_type_set_speed);
    RESET_FAKE(servo_type_sweep);
    RESET_FAKE(servo_type_stop);
    RESET_FAKE(servo_type_center);
    FFF_RESET_HISTORY();

    mock_init_fake.custom_fake = mock_init_impl;
    mock_deinit_fake.custom_fake = mock_deinit_impl;
    mock_write_fake.custom_fake = mock_write_impl;
    mock_read_fake.custom_fake = mock_read_impl;
    mock_enable_fake.custom_fake = mock_enable_impl;
}

static const relay_ops_t mock_relay_ops = {
    .set = relay_type_set,
    .get = relay_type_get,
    .toggle = relay_type_toggle,
    .pulse = relay_type_pulse,
};

static const servo_ops_t mock_servo_ops = {
    .set_angle = servo_type_set_angle,
    .get_angle = servo_type_get_angle,
    .set_range = servo_type_set_range,
    .set_speed = servo_type_set_speed,
    .sweep = servo_type_sweep,
    .stop = servo_type_stop,
    .center = servo_type_center,
};

static const actuator_ops_t mock_ops = {
    .init = mock_init,
    .deinit = mock_deinit,
    .write = mock_write,
    .read = mock_read,
    .enable = mock_enable,
};

static const actuator_ops_t mock_relay_wrapper_ops = {
    .type_ops = &mock_relay_ops,
};

static const actuator_ops_t mock_servo_wrapper_ops = {
    .type_ops = &mock_servo_ops,
};

static void test_strings_and_helpers(void)
{
    TEST_ASSERT_EQUAL_STRING("Relay", actuator_type_str(ACTUATOR_TYPE_RELAY));
    TEST_ASSERT_EQUAL_STRING("Servo", actuator_type_str(ACTUATOR_TYPE_SERVO));
    TEST_ASSERT_EQUAL_STRING("Unknown", actuator_type_str((actuator_type_t)0x7e));

    TEST_ASSERT_EQUAL_STRING("Ready", actuator_status_str(ACTUATOR_STATUS_READY));
    TEST_ASSERT_EQUAL_STRING("Unknown", actuator_status_str((actuator_status_t)0x7e));

    TEST_ASSERT_EQUAL_STRING("OK", actuator_err_str(ACTUATOR_EOK));
    TEST_ASSERT_EQUAL_STRING("Not implemented", actuator_err_str(ACTUATOR_ENOSYS));
    TEST_ASSERT_EQUAL_STRING("Unknown", actuator_err_str((actuator_err_t)-99));

    TEST_ASSERT_EQUAL_UINT32(500, servo_angle_to_pwm(-90.0f, -90.0f, 90.0f, 500, 2500));
    TEST_ASSERT_EQUAL_UINT32(1500, servo_angle_to_pwm(0.0f, -90.0f, 90.0f, 500, 2500));
    TEST_ASSERT_EQUAL_UINT32(2500, servo_angle_to_pwm(90.0f, -90.0f, 90.0f, 500, 2500));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, servo_pwm_to_angle(1500, -90.0f, 90.0f, 500, 2500));
}

static void test_registration_lifecycle_and_generic_io(void)
{
    reset_mock();

    actuator_device_t relay = ACTUATOR_DEVICE_INIT("act_relay", ACTUATOR_TYPE_RELAY, &mock_ops, NULL, NULL);
    actuator_device_t servo = ACTUATOR_DEVICE_INIT("act_servo", ACTUATOR_TYPE_SERVO, &mock_ops, NULL, NULL);

    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, actuator_register(NULL));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_register(&relay));
    TEST_ASSERT_EQUAL_UINT8(0, relay.id);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_READY, relay.status);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, actuator_register(&relay));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_register(&servo));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(2, actuator_get_count());
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(1, actuator_get_count_by_type(ACTUATOR_TYPE_RELAY));
    TEST_ASSERT_EQUAL_PTR(&relay, actuator_find("act_relay"));
    TEST_ASSERT_EQUAL_PTR(&servo, actuator_find_by_type(ACTUATOR_TYPE_SERVO));

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_init(&relay));
    TEST_ASSERT_EQUAL_UINT(1, mock_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, mock_init_fake.arg0_val);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_enable(&relay, false));
    TEST_ASSERT_EQUAL_UINT(1, mock_enable_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, mock_enable_fake.arg0_val);
    TEST_ASSERT_FALSE(mock_enable_fake.arg1_val);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_DISABLED, relay.status);

    actuator_value_t value = {0};
    value.relay.state = RELAY_STATE_ON;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_write(&relay, &value));
    TEST_ASSERT_EQUAL_UINT(1, mock_write_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, mock_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&value, mock_write_fake.arg1_val);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, g_last_write.relay.state);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_READY, relay.status);

    actuator_value_t readback = {0};
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_read(&relay, &readback));
    TEST_ASSERT_EQUAL_UINT(1, mock_read_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, mock_read_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&readback, mock_read_fake.arg1_val);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, readback.relay.state);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_deinit(&relay));
    TEST_ASSERT_EQUAL_UINT(1, mock_deinit_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, mock_deinit_fake.arg0_val);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_IDLE, relay.status);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_unregister(&relay));
    TEST_ASSERT_NULL(actuator_find("act_relay"));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_unregister(&servo));
}

static void test_generic_write_preserves_cached_value_on_backend_failure(void)
{
    reset_mock();

    actuator_device_t relay = ACTUATOR_DEVICE_INIT("act_write_fail", ACTUATOR_TYPE_RELAY, &mock_ops, NULL, NULL);
    actuator_value_t value = {0};
    relay.value.relay.state = RELAY_STATE_OFF;
    relay.status = ACTUATOR_STATUS_READY;
    value.relay.state = RELAY_STATE_ON;

    mock_write_fake.custom_fake = NULL;
    mock_write_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, actuator_write(&relay, &value));
    TEST_ASSERT_EQUAL_UINT(1, mock_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_OFF, relay.value.relay.state);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_ERROR, relay.status);

    mock_write_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_write(&relay, &value));
    TEST_ASSERT_EQUAL_UINT(2, mock_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_ON, relay.value.relay.state);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_READY, relay.status);
}

static void test_missing_ops_and_default_relay(void)
{
    actuator_device_t missing = ACTUATOR_DEVICE_INIT("missing_ops", ACTUATOR_TYPE_PWM, NULL, NULL, NULL);
    actuator_value_t value = {0};

    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, actuator_write(NULL, &value));
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, actuator_write(&missing, NULL));
    TEST_ASSERT_EQUAL(ACTUATOR_ENOSYS, actuator_write(&missing, &value));
    TEST_ASSERT_EQUAL(ACTUATOR_ENOSYS, actuator_read(&missing, &value));

    actuator_device_t relay = ACTUATOR_DEVICE_INIT("relay_default", ACTUATOR_TYPE_RELAY, &relay_default_ops, NULL, NULL);
    relay.config.active_high = true;

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_init(&relay));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_get(&relay, &value.relay.state));
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, value.relay.state);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_on(&relay));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_get(&relay, &value.relay.state));
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, value.relay.state);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_toggle(&relay));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_get(&relay, &value.relay.state));
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, value.relay.state);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_pulse(&relay, 10));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_get(&relay, &value.relay.state));
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, value.relay.state);
}

static void test_type_specific_relay_ops_preserve_state_on_failure(void)
{
    reset_mock();

    actuator_device_t relay = ACTUATOR_DEVICE_INIT(
        "relay_type_ops", ACTUATOR_TYPE_RELAY, &mock_relay_wrapper_ops, NULL, NULL);
    uint8_t state = 0xA5;

    relay.value.relay.state = RELAY_STATE_OFF;
    relay_type_set_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, relay_set(&relay, RELAY_STATE_ON));
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_OFF, relay.value.relay.state);
    TEST_ASSERT_EQUAL_UINT(1, relay_type_set_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, relay_type_set_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_ON, relay_type_set_fake.arg1_val);

    relay_type_set_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_on(&relay));
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_ON, relay.value.relay.state);

    relay_type_toggle_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, relay_toggle(&relay));
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_ON, relay.value.relay.state);
    TEST_ASSERT_EQUAL_UINT(1, relay_type_toggle_fake.call_count);

    relay_type_toggle_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_toggle(&relay));
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_OFF, relay.value.relay.state);

    relay_type_get_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, relay_get(&relay, &state));
    TEST_ASSERT_EQUAL_UINT8(0xA5, state);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_OFF, relay.value.relay.state);

    state = RELAY_STATE_ON;
    relay_type_get_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_get(&relay, &state));
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_ON, relay.value.relay.state);

    relay_type_pulse_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_pulse(&relay, 25));
    TEST_ASSERT_EQUAL_UINT(1, relay_type_pulse_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, relay_type_pulse_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT32(25, relay_type_pulse_fake.arg1_val);
}

static void test_type_specific_servo_ops_do_not_update_local_state_on_failure(void)
{
    reset_mock();

    actuator_device_t servo = ACTUATOR_DEVICE_INIT(
        "servo_type_ops", ACTUATOR_TYPE_SERVO, &mock_servo_wrapper_ops, NULL, NULL);
    float angle = -123.0f;

    servo.config.servo_min_angle = -45.0f;
    servo.config.servo_max_angle = 45.0f;
    servo.value.servo.current_angle = 5.0f;
    servo.value.servo.target_angle = 5.0f;

    servo_type_set_angle_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, servo_set_angle(&servo, 90.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, servo.value.servo.current_angle);
    TEST_ASSERT_EQUAL_UINT(1, servo_type_set_angle_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&servo, servo_type_set_angle_fake.arg0_val);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 45.0f, servo_type_set_angle_fake.arg1_val);

    servo_type_set_angle_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_set_angle(&servo, -90.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -45.0f, servo.value.servo.current_angle);

    servo_type_get_angle_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, servo_get_angle(&servo, &angle));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -123.0f, angle);

    angle = 12.5f;
    servo_type_get_angle_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_get_angle(&servo, &angle));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.5f, servo.value.servo.current_angle);

    servo_type_set_range_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, servo_set_range(&servo, -30.0f, 30.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -45.0f, servo.config.servo_min_angle);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 45.0f, servo.config.servo_max_angle);

    servo_type_set_range_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_set_range(&servo, -30.0f, 30.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -30.0f, servo.config.servo_min_angle);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 30.0f, servo.config.servo_max_angle);

    servo_type_set_speed_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, servo_set_speed(&servo, 200));
    TEST_ASSERT_EQUAL_UINT32(0, servo.config.servo_speed);

    servo_type_set_speed_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_set_speed(&servo, 200));
    TEST_ASSERT_EQUAL_UINT32(200, servo.config.servo_speed);

    servo_type_sweep_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, servo_sweep(&servo, -10.0f, 10.0f, 20));
    TEST_ASSERT_EQUAL_UINT(1, servo_type_sweep_fake.call_count);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -10.0f, servo_type_sweep_fake.arg1_val);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, servo_type_sweep_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT32(20, servo_type_sweep_fake.arg3_val);

    servo_type_stop_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, servo_stop(&servo));
    servo_type_center_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_center(&servo));
    TEST_ASSERT_EQUAL_UINT(1, servo_type_center_fake.call_count);
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

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_register(&relay));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_register(&servo));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_register(&pwm));

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_init(&servo));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_set_angle(&servo, 200.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, servo.value.servo.current_angle);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_set_range(&servo, -45.0f, 45.0f));
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, servo_set_range(&servo, 45.0f, -45.0f));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_set_speed(&servo, 120));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_center(&servo));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, servo.value.servo.current_angle);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, pwm_set_duty(&pwm, 1234));
    TEST_ASSERT_EQUAL_UINT16(1234, pwm.value.pwm.duty);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, pwm_set_frequency(&pwm, 1000));
    TEST_ASSERT_EQUAL_UINT32(1000, pwm.config.pwm_freq);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_on(&relay));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_all_off());
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, relay.value.relay.state);
    TEST_ASSERT_EQUAL_UINT16(0, pwm.value.pwm.duty);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_emergency_stop_all());

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_unregister(&relay));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_unregister(&servo));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_unregister(&pwm));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_strings_and_helpers);
    RUN_TEST(test_registration_lifecycle_and_generic_io);
    RUN_TEST(test_generic_write_preserves_cached_value_on_backend_failure);
    RUN_TEST(test_missing_ops_and_default_relay);
    RUN_TEST(test_type_specific_relay_ops_preserve_state_on_failure);
    RUN_TEST(test_type_specific_servo_ops_do_not_update_local_state_on_failure);
    RUN_TEST(test_default_servo_pwm_and_batch_helpers);
    return UNITY_END();
}
