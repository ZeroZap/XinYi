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
FAKE_VALUE_FUNC(actuator_err_t, mock_config, actuator_device_t *, const actuator_config_t *);
FAKE_VALUE_FUNC(actuator_err_t, mock_get_config, actuator_device_t *, actuator_config_t *);
FAKE_VALUE_FUNC(actuator_status_t, mock_get_status, actuator_device_t *);
FAKE_VALUE_FUNC(bool, mock_is_ready, actuator_device_t *);

FAKE_VALUE_FUNC(actuator_err_t, relay_type_init, actuator_device_t *);
FAKE_VALUE_FUNC(actuator_err_t, relay_type_deinit, actuator_device_t *);
FAKE_VALUE_FUNC(actuator_err_t, relay_type_set, actuator_device_t *, uint8_t);
FAKE_VALUE_FUNC(actuator_err_t, relay_type_get, actuator_device_t *, uint8_t *);
FAKE_VALUE_FUNC(actuator_err_t, relay_type_toggle, actuator_device_t *);
FAKE_VALUE_FUNC(actuator_err_t, relay_type_pulse, actuator_device_t *, uint32_t);

FAKE_VALUE_FUNC(actuator_err_t, servo_type_init, actuator_device_t *);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_deinit, actuator_device_t *);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_set_angle, actuator_device_t *, float);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_get_angle, actuator_device_t *, float *);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_set_range, actuator_device_t *, float, float);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_set_speed, actuator_device_t *, uint32_t);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_sweep, actuator_device_t *, float, float, uint32_t);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_stop, actuator_device_t *);
FAKE_VALUE_FUNC(actuator_err_t, servo_type_center, actuator_device_t *);

FAKE_VALUE_FUNC(actuator_err_t, pwm_write, actuator_device_t *, const actuator_value_t *);
FAKE_VALUE_FUNC(actuator_err_t, sleep_backend, actuator_device_t *);
FAKE_VALUE_FUNC(actuator_err_t, wakeup_backend, actuator_device_t *);

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
    RESET_FAKE(mock_config);
    RESET_FAKE(mock_get_config);
    RESET_FAKE(mock_get_status);
    RESET_FAKE(mock_is_ready);
    RESET_FAKE(relay_type_init);
    RESET_FAKE(relay_type_deinit);
    RESET_FAKE(relay_type_set);
    RESET_FAKE(relay_type_get);
    RESET_FAKE(relay_type_toggle);
    RESET_FAKE(relay_type_pulse);
    RESET_FAKE(servo_type_init);
    RESET_FAKE(servo_type_deinit);
    RESET_FAKE(servo_type_set_angle);
    RESET_FAKE(servo_type_get_angle);
    RESET_FAKE(servo_type_set_range);
    RESET_FAKE(servo_type_set_speed);
    RESET_FAKE(servo_type_sweep);
    RESET_FAKE(servo_type_stop);
    RESET_FAKE(servo_type_center);
    RESET_FAKE(pwm_write);
    RESET_FAKE(sleep_backend);
    RESET_FAKE(wakeup_backend);
    FFF_RESET_HISTORY();

    mock_init_fake.custom_fake = mock_init_impl;
    mock_deinit_fake.custom_fake = mock_deinit_impl;
    mock_write_fake.custom_fake = mock_write_impl;
    mock_read_fake.custom_fake = mock_read_impl;
    mock_enable_fake.custom_fake = mock_enable_impl;
    pwm_write_fake.custom_fake = mock_write_impl;
}

static const relay_ops_t mock_relay_ops = {
    .init = relay_type_init,
    .deinit = relay_type_deinit,
    .set = relay_type_set,
    .get = relay_type_get,
    .toggle = relay_type_toggle,
    .pulse = relay_type_pulse,
};

static const servo_ops_t mock_servo_ops = {
    .init = servo_type_init,
    .deinit = servo_type_deinit,
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
    .config = mock_config,
    .get_config = mock_get_config,
    .get_status = mock_get_status,
    .is_ready = mock_is_ready,
};

static const actuator_ops_t mock_relay_wrapper_ops = {
    .type_ops = &mock_relay_ops,
};

static const relay_ops_t mock_relay_set_only_ops = {
    .set = relay_type_set,
};

static const actuator_ops_t mock_relay_set_only_wrapper_ops = {
    .type_ops = &mock_relay_set_only_ops,
};

static const actuator_ops_t mock_servo_wrapper_ops = {
    .type_ops = &mock_servo_ops,
};

static const actuator_ops_t mock_pwm_ops = {
    .write = pwm_write,
};

static const actuator_ops_t mock_pm_ops = {
    .sleep = sleep_backend,
    .wakeup = wakeup_backend,
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

static void test_unregister_missing_device_preserves_registry(void)
{
    reset_mock();

    actuator_device_t relay = ACTUATOR_DEVICE_INIT("act_keep", ACTUATOR_TYPE_RELAY, &mock_ops, NULL, NULL);
    actuator_device_t missing = ACTUATOR_DEVICE_INIT("act_missing", ACTUATOR_TYPE_RELAY, &mock_ops, NULL, NULL);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_register(&relay));
    TEST_ASSERT_EQUAL_UINT8(1U, actuator_get_count());

    TEST_ASSERT_EQUAL(ACTUATOR_ENODEV, actuator_unregister(&missing));
    TEST_ASSERT_EQUAL_UINT8(1U, actuator_get_count());
    TEST_ASSERT_EQUAL_PTR(&relay, actuator_find("act_keep"));
    TEST_ASSERT_NULL(actuator_find("act_missing"));
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_IDLE, missing.status);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_unregister(&relay));
    TEST_ASSERT_EQUAL_UINT8(0U, actuator_get_count());
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

static void test_generic_config_and_status_dispatch_contracts(void)
{
    reset_mock();

    actuator_device_t relay = ACTUATOR_DEVICE_INIT("act_config", ACTUATOR_TYPE_RELAY, &mock_ops, NULL, NULL);
    actuator_config_t config = {0};
    actuator_config_t out_config = {0};

    config.channel = 3U;
    config.active_high = true;
    relay.config.channel = 1U;
    relay.status = ACTUATOR_STATUS_READY;

    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, actuator_config(NULL, &config));
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, actuator_config(&relay, NULL));

    mock_config_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, actuator_config(&relay, &config));
    TEST_ASSERT_EQUAL_UINT(1, mock_config_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, mock_config_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&config, mock_config_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT8(1U, relay.config.channel);

    mock_config_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_config(&relay, &config));
    TEST_ASSERT_EQUAL_UINT(2, mock_config_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(3U, relay.config.channel);
    TEST_ASSERT_TRUE(relay.config.active_high);

    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, actuator_get_config(NULL, &out_config));
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, actuator_get_config(&relay, NULL));

    mock_get_config_fake.return_val = ACTUATOR_EIO;
    out_config.channel = 0xA5U;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, actuator_get_config(&relay, &out_config));
    TEST_ASSERT_EQUAL_UINT(1, mock_get_config_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, mock_get_config_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&out_config, mock_get_config_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT8(0xA5U, out_config.channel);

    actuator_device_t fallback = ACTUATOR_DEVICE_INIT("act_config_fallback", ACTUATOR_TYPE_RELAY, NULL, NULL, NULL);
    fallback.config.channel = 7U;
    fallback.status = ACTUATOR_STATUS_DISABLED;
    out_config.channel = 0U;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_get_config(&fallback, &out_config));
    TEST_ASSERT_EQUAL_UINT8(7U, out_config.channel);
    TEST_ASSERT_FALSE(actuator_is_ready(&fallback));

    mock_get_status_fake.return_val = ACTUATOR_STATUS_BUSY;
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_BUSY, actuator_get_status(&relay));
    TEST_ASSERT_EQUAL_UINT(1, mock_get_status_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, mock_get_status_fake.arg0_val);

    mock_is_ready_fake.return_val = true;
    TEST_ASSERT_TRUE(actuator_is_ready(&relay));
    TEST_ASSERT_EQUAL_UINT(1, mock_is_ready_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, mock_is_ready_fake.arg0_val);
    TEST_ASSERT_FALSE(actuator_is_ready(NULL));
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

    relay_type_init_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, relay_init(&relay));
    TEST_ASSERT_EQUAL_UINT(1, relay_type_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, relay_type_init_fake.arg0_val);

    relay_type_init_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_init(&relay));
    TEST_ASSERT_EQUAL_UINT(2, relay_type_init_fake.call_count);

    relay.value.relay.state = RELAY_STATE_ON;
    relay.status = ACTUATOR_STATUS_READY;
    relay_type_deinit_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, relay_deinit(&relay));
    TEST_ASSERT_EQUAL_UINT(1, relay_type_deinit_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, relay_type_deinit_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_ON, relay.value.relay.state);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_READY, relay.status);

    relay_type_deinit_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_deinit(&relay));
    TEST_ASSERT_EQUAL_UINT(2, relay_type_deinit_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_OFF, relay.value.relay.state);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_IDLE, relay.status);

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

    servo_type_init_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, servo_init(&servo));
    TEST_ASSERT_EQUAL_UINT(1, servo_type_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&servo, servo_type_init_fake.arg0_val);

    servo_type_init_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_init(&servo));
    TEST_ASSERT_EQUAL_UINT(2, servo_type_init_fake.call_count);

    servo.status = ACTUATOR_STATUS_READY;
    servo_type_deinit_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, servo_deinit(&servo));
    TEST_ASSERT_EQUAL_UINT(1, servo_type_deinit_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&servo, servo_type_deinit_fake.arg0_val);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_READY, servo.status);

    servo_type_deinit_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_deinit(&servo));
    TEST_ASSERT_EQUAL_UINT(2, servo_type_deinit_fake.call_count);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_IDLE, servo.status);

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
    TEST_ASSERT_EQUAL_UINT(1, servo_type_stop_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&servo, servo_type_stop_fake.arg0_val);

    servo.value.servo.current_angle = 18.0f;
    servo.value.servo.target_angle = 18.0f;
    servo_type_center_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, servo_center(&servo));
    TEST_ASSERT_EQUAL_UINT(1, servo_type_center_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&servo, servo_type_center_fake.arg0_val);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 18.0f, servo.value.servo.current_angle);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 18.0f, servo.value.servo.target_angle);

    servo_type_center_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_center(&servo));
    TEST_ASSERT_EQUAL_UINT(2, servo_type_center_fake.call_count);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, servo.value.servo.current_angle);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, servo.value.servo.target_angle);
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

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_unregister(&pwm));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_unregister(&relay));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_unregister(&servo));
}

static void test_default_relay_pulse_propagates_fallback_write_failures(void)
{
    reset_mock();

    actuator_device_t relay = ACTUATOR_DEVICE_INIT(
        "relay_pulse_fail", ACTUATOR_TYPE_RELAY, &mock_relay_set_only_wrapper_ops, NULL, NULL);
    relay.value.relay.state = RELAY_STATE_OFF;
    relay.status = ACTUATOR_STATUS_READY;

    relay_type_set_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, relay_pulse(&relay, 10));
    TEST_ASSERT_EQUAL_UINT(1, relay_type_set_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, relay_type_set_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_ON, relay_type_set_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_OFF, relay.value.relay.state);

    relay_type_set_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, relay_pulse(&relay, 10));
    TEST_ASSERT_EQUAL_UINT(3, relay_type_set_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_OFF, relay.value.relay.state);
}

static actuator_err_t fail_on_second_relay_set(actuator_device_t *dev, uint8_t state)
{
    TEST_ASSERT_NOT_NULL(dev);
    if (relay_type_set_fake.call_count == 2U) {
        return ACTUATOR_EIO;
    }

    dev->value.relay.state = state;
    return ACTUATOR_EOK;
}

static void test_default_relay_pulse_reports_off_failure_after_on_success(void)
{
    reset_mock();

    actuator_device_t relay = ACTUATOR_DEVICE_INIT(
        "relay_pulse_second_fail", ACTUATOR_TYPE_RELAY, &mock_relay_set_only_wrapper_ops, NULL,
        NULL);
    relay.value.relay.state = RELAY_STATE_OFF;

    relay_type_set_fake.custom_fake = fail_on_second_relay_set;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, relay_pulse(&relay, 10));
    TEST_ASSERT_EQUAL_UINT(2, relay_type_set_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, relay_type_set_fake.arg0_history[0]);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_ON, relay_type_set_fake.arg1_history[0]);
    TEST_ASSERT_EQUAL_PTR(&relay, relay_type_set_fake.arg0_history[1]);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_OFF, relay_type_set_fake.arg1_history[1]);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_ON, relay.value.relay.state);
}

static actuator_err_t fail_on_second_servo_write(actuator_device_t *dev, const actuator_value_t *value)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(value);
    if (mock_write_fake.call_count == 2U) {
        return ACTUATOR_EIO;
    }
    return mock_write_impl(dev, value);
}

static void test_default_servo_sweep_stops_on_fallback_write_failure(void)
{
    reset_mock();

    actuator_device_t servo = ACTUATOR_DEVICE_INIT("servo_sweep_fail", ACTUATOR_TYPE_SERVO, &mock_ops, NULL, NULL);
    servo.config.servo_min_angle = -90.0f;
    servo.config.servo_max_angle = 90.0f;
    servo.config.servo_pwm_min = 500;
    servo.config.servo_pwm_max = 2500;
    servo.value.servo.current_angle = -5.0f;
    servo.value.servo.target_angle = -5.0f;

    mock_write_fake.custom_fake = fail_on_second_servo_write;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, servo_sweep(&servo, 0.0f, 2.0f, 5));
    TEST_ASSERT_EQUAL_UINT(2, mock_write_fake.call_count);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, servo.value.servo.current_angle);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, servo.value.servo.target_angle);

    mock_write_fake.custom_fake = mock_write_impl;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, servo_sweep(&servo, 0.0f, 2.0f, 5));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, servo.value.servo.current_angle);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, servo.value.servo.target_angle);
}

static void test_batch_helpers_report_backend_failures(void)
{
    reset_mock();

    actuator_device_t relay = ACTUATOR_DEVICE_INIT(
        "batch_fail_relay", ACTUATOR_TYPE_RELAY, &mock_relay_wrapper_ops, NULL, NULL);

    relay.value.relay.state = RELAY_STATE_ON;
    relay_type_set_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_register(&relay));

    TEST_ASSERT_EQUAL(ACTUATOR_EIO, actuator_all_off());
    TEST_ASSERT_EQUAL_UINT(1, relay_type_set_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, relay_type_set_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_OFF, relay_type_set_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_ON, relay.value.relay.state);

    relay_type_set_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, actuator_emergency_stop_all());
    TEST_ASSERT_EQUAL_UINT(2, relay_type_set_fake.call_count);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_unregister(&relay));
}

static void test_reset_and_emergency_stop_fallbacks_propagate_type_ops_results(void)
{
    reset_mock();

    actuator_device_t relay = ACTUATOR_DEVICE_INIT(
        "reset_relay", ACTUATOR_TYPE_RELAY, &mock_relay_wrapper_ops, NULL, NULL);
    actuator_device_t servo = ACTUATOR_DEVICE_INIT(
        "reset_servo", ACTUATOR_TYPE_SERVO, &mock_servo_wrapper_ops, NULL, NULL);

    relay.value.relay.state = RELAY_STATE_ON;
    relay_type_set_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, actuator_reset(&relay));
    TEST_ASSERT_EQUAL_UINT(1, relay_type_set_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_OFF, relay_type_set_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_ON, relay.value.relay.state);

    relay_type_set_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_reset(&relay));
    TEST_ASSERT_EQUAL_UINT(2, relay_type_set_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(RELAY_STATE_OFF, relay.value.relay.state);

    servo.value.servo.current_angle = 12.0f;
    servo.value.servo.target_angle = 12.0f;
    servo_type_center_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, actuator_reset(&servo));
    TEST_ASSERT_EQUAL_UINT(1, servo_type_center_fake.call_count);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.0f, servo.value.servo.current_angle);

    servo_type_center_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_reset(&servo));
    TEST_ASSERT_EQUAL_UINT(2, servo_type_center_fake.call_count);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, servo.value.servo.current_angle);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_emergency_stop(&servo));
    TEST_ASSERT_EQUAL_UINT(2, servo_type_center_fake.call_count);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, servo.value.servo.current_angle);
}

static void test_pwm_set_duty_preserves_state_on_backend_failure(void)
{
    reset_mock();

    actuator_device_t pwm = ACTUATOR_DEVICE_INIT("pwm_fail", ACTUATOR_TYPE_PWM, &mock_pwm_ops, NULL, NULL);
    pwm.value.pwm.duty = 1234U;

    pwm_write_fake.custom_fake = NULL;
    pwm_write_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, pwm_set_duty(&pwm, 4321U));
    TEST_ASSERT_EQUAL_UINT(1, pwm_write_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&pwm, pwm_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT16(4321U, pwm_write_fake.arg1_val->pwm.duty);
    TEST_ASSERT_EQUAL_UINT16(1234U, pwm.value.pwm.duty);

    pwm_write_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, pwm_set_duty(&pwm, 4321U));
    TEST_ASSERT_EQUAL_UINT(2, pwm_write_fake.call_count);
    TEST_ASSERT_EQUAL_UINT16(4321U, pwm.value.pwm.duty);
}

static void test_sleep_wakeup_dispatch_updates_status_only_on_success(void)
{
    reset_mock();

    actuator_device_t relay = ACTUATOR_DEVICE_INIT("pm_relay", ACTUATOR_TYPE_RELAY, &mock_pm_ops, NULL, NULL);
    relay.status = ACTUATOR_STATUS_READY;

    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, actuator_sleep(NULL));
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, actuator_wakeup(NULL));

    sleep_backend_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, actuator_sleep(&relay));
    TEST_ASSERT_EQUAL_UINT(1, sleep_backend_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, sleep_backend_fake.arg0_val);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_READY, relay.status);

    sleep_backend_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_sleep(&relay));
    TEST_ASSERT_EQUAL_UINT(2, sleep_backend_fake.call_count);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_DISABLED, relay.status);

    wakeup_backend_fake.return_val = ACTUATOR_EIO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, actuator_wakeup(&relay));
    TEST_ASSERT_EQUAL_UINT(1, wakeup_backend_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&relay, wakeup_backend_fake.arg0_val);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_DISABLED, relay.status);

    wakeup_backend_fake.return_val = ACTUATOR_EOK;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_wakeup(&relay));
    TEST_ASSERT_EQUAL_UINT(2, wakeup_backend_fake.call_count);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_READY, relay.status);

    actuator_device_t fallback = ACTUATOR_DEVICE_INIT("pm_fallback", ACTUATOR_TYPE_RELAY, NULL, NULL, NULL);
    fallback.status = ACTUATOR_STATUS_READY;
    TEST_ASSERT_EQUAL(ACTUATOR_ENOSYS, actuator_sleep(&fallback));
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_READY, fallback.status);
    TEST_ASSERT_EQUAL(ACTUATOR_ENOSYS, actuator_wakeup(&fallback));
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_READY, fallback.status);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_strings_and_helpers);
    RUN_TEST(test_registration_lifecycle_and_generic_io);
    RUN_TEST(test_unregister_missing_device_preserves_registry);
    RUN_TEST(test_generic_write_preserves_cached_value_on_backend_failure);
    RUN_TEST(test_generic_config_and_status_dispatch_contracts);
    RUN_TEST(test_missing_ops_and_default_relay);
    RUN_TEST(test_type_specific_relay_ops_preserve_state_on_failure);
    RUN_TEST(test_type_specific_servo_ops_do_not_update_local_state_on_failure);
    RUN_TEST(test_default_servo_pwm_and_batch_helpers);
    RUN_TEST(test_default_relay_pulse_propagates_fallback_write_failures);
    RUN_TEST(test_default_relay_pulse_reports_off_failure_after_on_success);
    RUN_TEST(test_default_servo_sweep_stops_on_fallback_write_failure);
    RUN_TEST(test_batch_helpers_report_backend_failures);
    RUN_TEST(test_reset_and_emergency_stop_fallbacks_propagate_type_ops_results);
    RUN_TEST(test_pwm_set_duty_preserves_state_on_backend_failure);
    RUN_TEST(test_sleep_wakeup_dispatch_updates_status_only_on_success);
    return UNITY_END();
}
