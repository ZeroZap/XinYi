/**
 * @file test_actuator_core.c
 * @brief Actuator Core Tests - Device registration, find, foreach
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"
#include "xy_actuator.h"

/* ==================== Mock Operations ==================== */
static actuator_err_t mock_init(actuator_device_t *dev)
{
    (void)dev;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_deinit(actuator_device_t *dev)
{
    (void)dev;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_write(actuator_device_t *dev, const actuator_value_t *value)
{
    (void)dev;
    (void)value;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_read(actuator_device_t *dev, actuator_value_t *value)
{
    (void)dev;
    (void)value;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_enable(actuator_device_t *dev, bool enable)
{
    (void)dev;
    (void)enable;
    return ACTUATOR_EOK;
}

static actuator_status_t mock_get_status(actuator_device_t *dev)
{
    (void)dev;
    return ACTUATOR_STATUS_READY;
}

static bool mock_is_ready(actuator_device_t *dev)
{
    (void)dev;
    return true;
}

static const actuator_ops_t mock_ops = {
    .init = mock_init,
    .deinit = mock_deinit,
    .write = mock_write,
    .read = mock_read,
    .enable = mock_enable,
    .get_status = mock_get_status,
    .is_ready = mock_is_ready,
};

/* ==================== Test Devices ==================== */
static actuator_device_t test_relay = {
    .name = "test_relay_1",
    .type = ACTUATOR_TYPE_RELAY,
    .ops = &mock_ops,
    .status = ACTUATOR_STATUS_IDLE,
};

static actuator_device_t test_servo = {
    .name = "test_servo_1",
    .type = ACTUATOR_TYPE_SERVO,
    .ops = &mock_ops,
    .status = ACTUATOR_STATUS_IDLE,
};

static actuator_device_t test_pwm = {
    .name = "test_pwm_1",
    .type = ACTUATOR_TYPE_PWM,
    .ops = &mock_ops,
    .status = ACTUATOR_STATUS_IDLE,
};

static actuator_device_t test_led = {
    .name = "test_led_1",
    .type = ACTUATOR_TYPE_LED,
    .ops = &mock_ops,
    .status = ACTUATOR_STATUS_IDLE,
};

/* ==================== Setup/Teardown ==================== */
void setUp(void)
{
    /* Unregister all devices before each test */
    actuator_unregister(&test_relay);
    actuator_unregister(&test_servo);
    actuator_unregister(&test_pwm);
    actuator_unregister(&test_led);
}

void tearDown(void)
{
    /* Clean up after each test */
    actuator_unregister(&test_relay);
    actuator_unregister(&test_servo);
    actuator_unregister(&test_pwm);
    actuator_unregister(&test_led);
}

/* ==================== Registration Tests ==================== */
void test_actuator_register_single(void)
{
    actuator_err_t ret = actuator_register(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_READY, test_relay.status);
}

void test_actuator_register_multiple(void)
{
    actuator_err_t ret;

    ret = actuator_register(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);

    ret = actuator_register(&test_servo);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);

    ret = actuator_register(&test_pwm);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);

    TEST_ASSERT_EQUAL(3, actuator_get_count());
}

void test_actuator_register_null(void)
{
    actuator_err_t ret = actuator_register(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_actuator_register_duplicate(void)
{
    actuator_err_t ret;

    ret = actuator_register(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);

    /* Try to register same device again */
    ret = actuator_register(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_actuator_register_same_name(void)
{
    actuator_device_t another_relay = {
        .name = "test_relay_1",  /* Same name */
        .type = ACTUATOR_TYPE_RELAY,
        .ops = &mock_ops,
        .status = ACTUATOR_STATUS_IDLE,
    };

    actuator_err_t ret;

    ret = actuator_register(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);

    ret = actuator_register(&another_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);  /* Should fail due to duplicate name */
}

/* ==================== Unregister Tests ==================== */
void test_actuator_unregister_single(void)
{
    actuator_err_t ret;

    actuator_register(&test_relay);
    TEST_ASSERT_EQUAL(1, actuator_get_count());

    ret = actuator_unregister(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(0, actuator_get_count());
}

void test_actuator_unregister_null(void)
{
    actuator_err_t ret = actuator_unregister(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_actuator_unregister_not_found(void)
{
    actuator_err_t ret = actuator_unregister(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);  /* Unregistering unregistered device is OK */
}

/* ==================== Find Tests ==================== */
void test_actuator_find_by_name(void)
{
    actuator_device_t *found;

    actuator_register(&test_relay);
    actuator_register(&test_servo);

    found = actuator_find("test_relay_1");
    TEST_ASSERT_EQUAL_PTR(&test_relay, found);

    found = actuator_find("test_servo_1");
    TEST_ASSERT_EQUAL_PTR(&test_servo, found);

    found = actuator_find("nonexistent");
    TEST_ASSERT_NULL(found);
}

void test_actuator_find_by_type(void)
{
    actuator_device_t *found;

    actuator_register(&test_relay);
    actuator_register(&test_servo);
    actuator_register(&test_pwm);

    found = actuator_find_by_type(ACTUATOR_TYPE_RELAY);
    TEST_ASSERT_EQUAL_PTR(&test_relay, found);

    found = actuator_find_by_type(ACTUATOR_TYPE_SERVO);
    TEST_ASSERT_EQUAL_PTR(&test_servo, found);

    found = actuator_find_by_type(ACTUATOR_TYPE_LED);
    TEST_ASSERT_NULL(found);
}

void test_actuator_find_null(void)
{
    actuator_device_t *found = actuator_find(NULL);
    TEST_ASSERT_NULL(found);
}

/* ==================== Count Tests ==================== */
void test_actuator_get_count(void)
{
    TEST_ASSERT_EQUAL(0, actuator_get_count());

    actuator_register(&test_relay);
    TEST_ASSERT_EQUAL(1, actuator_get_count());

    actuator_register(&test_servo);
    TEST_ASSERT_EQUAL(2, actuator_get_count());

    actuator_register(&test_pwm);
    TEST_ASSERT_EQUAL(3, actuator_get_count());

    actuator_unregister(&test_servo);
    TEST_ASSERT_EQUAL(2, actuator_get_count());
}

void test_actuator_get_count_by_type(void)
{
    actuator_register(&test_relay);
    actuator_register(&test_servo);
    actuator_register(&test_pwm);
    actuator_register(&test_led);

    TEST_ASSERT_EQUAL(1, actuator_get_count_by_type(ACTUATOR_TYPE_RELAY));
    TEST_ASSERT_EQUAL(1, actuator_get_count_by_type(ACTUATOR_TYPE_SERVO));
    TEST_ASSERT_EQUAL(1, actuator_get_count_by_type(ACTUATOR_TYPE_PWM));
    TEST_ASSERT_EQUAL(1, actuator_get_count_by_type(ACTUATOR_TYPE_LED));
    TEST_ASSERT_EQUAL(0, actuator_get_count_by_type(ACTUATOR_TYPE_MOTOR_DC));
}

/* ==================== Device Init/Deinit Tests ==================== */
void test_actuator_init(void)
{
    actuator_err_t ret = actuator_init(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

void test_actuator_init_null(void)
{
    actuator_err_t ret = actuator_init(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_actuator_deinit(void)
{
    actuator_register(&test_relay);
    actuator_err_t ret = actuator_deinit(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

void test_actuator_deinit_null(void)
{
    actuator_err_t ret = actuator_deinit(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Enable Tests ==================== */
void test_actuator_enable(void)
{
    actuator_err_t ret = actuator_enable(&test_relay, true);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

void test_actuator_enable_null(void)
{
    actuator_err_t ret = actuator_enable(NULL, true);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Status Tests ==================== */
void test_actuator_get_status(void)
{
    actuator_register(&test_relay);
    actuator_status_t status = actuator_get_status(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_READY, status);
}

void test_actuator_get_status_null(void)
{
    actuator_status_t status = actuator_get_status(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_STATUS_ERROR, status);
}

/* ==================== Type String Tests ==================== */
void test_actuator_type_str(void)
{
    TEST_ASSERT_EQUAL_STRING("Relay", actuator_type_str(ACTUATOR_TYPE_RELAY));
    TEST_ASSERT_EQUAL_STRING("Servo", actuator_type_str(ACTUATOR_TYPE_SERVO));
    TEST_ASSERT_EQUAL_STRING("DC Motor", actuator_type_str(ACTUATOR_TYPE_MOTOR_DC));
    TEST_ASSERT_EQUAL_STRING("Stepper Motor", actuator_type_str(ACTUATOR_TYPE_MOTOR_STEPPER));
    TEST_ASSERT_EQUAL_STRING("Solenoid", actuator_type_str(ACTUATOR_TYPE_SOLENOID));
    TEST_ASSERT_EQUAL_STRING("PWM", actuator_type_str(ACTUATOR_TYPE_PWM));
    TEST_ASSERT_EQUAL_STRING("LED", actuator_type_str(ACTUATOR_TYPE_LED));
    TEST_ASSERT_EQUAL_STRING("Buzzer", actuator_type_str(ACTUATOR_TYPE_BUZZER));
    TEST_ASSERT_EQUAL_STRING("Valve", actuator_type_str(ACTUATOR_TYPE_VALVE));
    TEST_ASSERT_EQUAL_STRING("Unknown", actuator_type_str(ACTUATOR_TYPE_CUSTOM));
}

void test_actuator_status_str(void)
{
    TEST_ASSERT_EQUAL_STRING("Idle", actuator_status_str(ACTUATOR_STATUS_IDLE));
    TEST_ASSERT_EQUAL_STRING("Ready", actuator_status_str(ACTUATOR_STATUS_READY));
    TEST_ASSERT_EQUAL_STRING("Busy", actuator_status_str(ACTUATOR_STATUS_BUSY));
    TEST_ASSERT_EQUAL_STRING("Error", actuator_status_str(ACTUATOR_STATUS_ERROR));
    TEST_ASSERT_EQUAL_STRING("Disabled", actuator_status_str(ACTUATOR_STATUS_DISABLED));
}

void test_actuator_err_str(void)
{
    TEST_ASSERT_EQUAL_STRING("OK", actuator_err_str(ACTUATOR_EOK));
    TEST_ASSERT_EQUAL_STRING("Error", actuator_err_str(ACTUATOR_ERROR));
    TEST_ASSERT_EQUAL_STRING("Invalid argument", actuator_err_str(ACTUATOR_EINVAL));
    TEST_ASSERT_EQUAL_STRING("Device not found", actuator_err_str(ACTUATOR_ENODEV));
    TEST_ASSERT_EQUAL_STRING("Device busy", actuator_err_str(ACTUATOR_EBUSY));
    TEST_ASSERT_EQUAL_STRING("Timeout", actuator_err_str(ACTUATOR_ETIMEOUT));
    TEST_ASSERT_EQUAL_STRING("Out of memory", actuator_err_str(ACTUATOR_ENOMEM));
    TEST_ASSERT_EQUAL_STRING("Not implemented", actuator_err_str(ACTUATOR_ENOSYS));
    TEST_ASSERT_EQUAL_STRING("I/O error", actuator_err_str(ACTUATOR_EIO));
    TEST_ASSERT_EQUAL_STRING("Limit exceeded", actuator_err_str(ACTUATOR_ELIMIT));
    TEST_ASSERT_EQUAL_STRING("Hardware error", actuator_err_str(ACTUATOR_EHW));
}

/* ==================== Write/Read Tests ==================== */
void test_actuator_write(void)
{
    actuator_value_t value = { .relay.state = RELAY_STATE_ON };
    actuator_err_t ret = actuator_write(&test_relay, &value);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

void test_actuator_write_null(void)
{
    actuator_value_t value = { .relay.state = RELAY_STATE_ON };
    actuator_err_t ret = actuator_write(NULL, &value);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_actuator_read(void)
{
    actuator_value_t value;
    actuator_err_t ret = actuator_read(&test_relay, &value);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

void test_actuator_read_null(void)
{
    actuator_err_t ret = actuator_read(NULL, NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Reset/Emergency Stop Tests ==================== */
void test_actuator_reset(void)
{
    actuator_err_t ret = actuator_reset(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

void test_actuator_reset_null(void)
{
    actuator_err_t ret = actuator_reset(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_actuator_emergency_stop(void)
{
    actuator_err_t ret = actuator_emergency_stop(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

void test_actuator_emergency_stop_null(void)
{
    actuator_err_t ret = actuator_emergency_stop(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Main ==================== */
int main(void)
{
    UNITY_BEGIN();

    /* Registration Tests */
    RUN_TEST(test_actuator_register_single);
    RUN_TEST(test_actuator_register_multiple);
    RUN_TEST(test_actuator_register_null);
    RUN_TEST(test_actuator_register_duplicate);
    RUN_TEST(test_actuator_register_same_name);

    /* Unregister Tests */
    RUN_TEST(test_actuator_unregister_single);
    RUN_TEST(test_actuator_unregister_null);
    RUN_TEST(test_actuator_unregister_not_found);

    /* Find Tests */
    RUN_TEST(test_actuator_find_by_name);
    RUN_TEST(test_actuator_find_by_type);
    RUN_TEST(test_actuator_find_null);

    /* Count Tests */
    RUN_TEST(test_actuator_get_count);
    RUN_TEST(test_actuator_get_count_by_type);

    /* Init/Deinit Tests */
    RUN_TEST(test_actuator_init);
    RUN_TEST(test_actuator_init_null);
    RUN_TEST(test_actuator_deinit);
    RUN_TEST(test_actuator_deinit_null);

    /* Enable Tests */
    RUN_TEST(test_actuator_enable);
    RUN_TEST(test_actuator_enable_null);

    /* Status Tests */
    RUN_TEST(test_actuator_get_status);
    RUN_TEST(test_actuator_get_status_null);

    /* String Tests */
    RUN_TEST(test_actuator_type_str);
    RUN_TEST(test_actuator_status_str);
    RUN_TEST(test_actuator_err_str);

    /* Write/Read Tests */
    RUN_TEST(test_actuator_write);
    RUN_TEST(test_actuator_write_null);
    RUN_TEST(test_actuator_read);
    RUN_TEST(test_actuator_read_null);

    /* Reset/Emergency Stop Tests */
    RUN_TEST(test_actuator_reset);
    RUN_TEST(test_actuator_reset_null);
    RUN_TEST(test_actuator_emergency_stop);
    RUN_TEST(test_actuator_emergency_stop_null);

    return UNITY_END();
}
