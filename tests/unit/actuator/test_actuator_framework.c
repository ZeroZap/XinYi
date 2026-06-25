#include "unity.h"

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

void setUp(void)
{
}

void tearDown(void)
{
}

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
    TEST_ASSERT_NOT_NULL(dev);
    g_init_calls++;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_deinit(actuator_device_t *dev)
{
    TEST_ASSERT_NOT_NULL(dev);
    g_deinit_calls++;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_write(actuator_device_t *dev, const actuator_value_t *value)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(value);
    g_write_calls++;
    g_last_write = *value;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_read(actuator_device_t *dev, actuator_value_t *value)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(value);
    *value = dev->value;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_enable(actuator_device_t *dev, bool enable)
{
    TEST_ASSERT_NOT_NULL(dev);
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
    TEST_ASSERT_EQUAL_INT(1, g_init_calls);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_enable(&relay, false));
    TEST_ASSERT_EQUAL_INT(1, g_enable_calls);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_DISABLED, relay.status);

    actuator_value_t value = {0};
    value.relay.state = RELAY_STATE_ON;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_write(&relay, &value));
    TEST_ASSERT_EQUAL_INT(1, g_write_calls);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, g_last_write.relay.state);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_READY, relay.status);

    actuator_value_t readback = {0};
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_read(&relay, &readback));
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, readback.relay.state);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_deinit(&relay));
    TEST_ASSERT_EQUAL_INT(1, g_deinit_calls);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_IDLE, relay.status);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_unregister(&relay));
    TEST_ASSERT_NULL(actuator_find("act_relay"));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, actuator_unregister(&servo));
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
    RUN_TEST(test_missing_ops_and_default_relay);
    RUN_TEST(test_default_servo_pwm_and_batch_helpers);
    return UNITY_END();
}
