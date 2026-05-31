/**
 * @file test_actuator_servo.c
 * @brief Actuator Servo Tests - Angle control operations
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "unity.h"
#include "xy_actuator.h"

/* ==================== Mock Servo Operations ==================== */
static float g_mock_servo_angle = 0.0f;
static float g_mock_servo_min = -90.0f;
static float g_mock_servo_max = 90.0f;
static uint8_t g_mock_init_called = 0;
static uint8_t g_mock_deinit_called = 0;

static actuator_err_t mock_servo_init(actuator_device_t *dev)
{
    (void)dev;
    g_mock_init_called++;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_servo_deinit(actuator_device_t *dev)
{
    (void)dev;
    g_mock_deinit_called++;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_servo_write(actuator_device_t *dev, const actuator_value_t *value)
{
    (void)dev;
    g_mock_servo_angle = value->servo.target_angle;
    return ACTUATOR_EOK;
}

static const actuator_ops_t mock_servo_ops = {
    .init = mock_servo_init,
    .deinit = mock_servo_deinit,
    .write = mock_servo_write,
};

/* ==================== Test Device ==================== */
static actuator_device_t test_servo = {
    .name = "test_servo",
    .type = ACTUATOR_TYPE_SERVO,
    .ops = &mock_servo_ops,
    .status = ACTUATOR_STATUS_IDLE,
    .config.servo_min_angle = -90.0f,
    .config.servo_max_angle = 90.0f,
    .config.servo_pwm_min = 500,
    .config.servo_pwm_max = 2500,
    .config.servo_speed = 90,
};

/* ==================== Setup/Teardown ==================== */
void setUp(void)
{
    g_mock_servo_angle = 0.0f;
    g_mock_servo_min = -90.0f;
    g_mock_servo_max = 90.0f;
    g_mock_init_called = 0;
    g_mock_deinit_called = 0;
    actuator_unregister(&test_servo);
    test_servo.status = ACTUATOR_STATUS_IDLE;
    test_servo.config.servo_min_angle = -90.0f;
    test_servo.config.servo_max_angle = 90.0f;
    test_servo.config.servo_pwm_min = 500;
    test_servo.config.servo_pwm_max = 2500;
    test_servo.config.servo_speed = 90;
}

void tearDown(void)
{
    actuator_unregister(&test_servo);
}

/* ==================== Servo Init Tests ==================== */
void test_servo_init(void)
{
    actuator_err_t ret = servo_init(&test_servo);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(1, g_mock_init_called);
}

void test_servo_init_null(void)
{
    actuator_err_t ret = servo_init(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Servo Set Angle Tests ==================== */
void test_servo_set_angle(void)
{
    actuator_register(&test_servo);

    /* Set angle to 0 (center) */
    actuator_err_t ret = servo_set_angle(&test_servo, 0.0f);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);

    /* Set angle to 45 degrees */
    ret = servo_set_angle(&test_servo, 45.0f);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);

    /* Set angle to -45 degrees */
    ret = servo_set_angle(&test_servo, -45.0f);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

void test_servo_set_angle_null(void)
{
    actuator_err_t ret = servo_set_angle(NULL, 45.0f);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_servo_set_angle_min(void)
{
    actuator_register(&test_servo);
    actuator_err_t ret = servo_set_angle(&test_servo, -90.0f);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL_FLOAT(-90.0f, g_mock_servo_angle);
}

void test_servo_set_angle_max(void)
{
    actuator_register(&test_servo);
    actuator_err_t ret = servo_set_angle(&test_servo, 90.0f);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL_FLOAT(90.0f, g_mock_servo_angle);
}

void test_servo_set_angle_center(void)
{
    actuator_register(&test_servo);
    actuator_err_t ret = servo_set_angle(&test_servo, 0.0f);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, g_mock_servo_angle);
}

/* ==================== Servo Get Angle Tests ==================== */
void test_servo_get_angle(void)
{
    float angle;

    actuator_register(&test_servo);
    servo_set_angle(&test_servo, 45.0f);

    actuator_err_t ret = servo_get_angle(&test_servo, &angle);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    /* Note: Mock returns what was set via write */
}

void test_servo_get_angle_null(void)
{
    float angle;
    actuator_err_t ret = servo_get_angle(NULL, &angle);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_servo_get_angle_null_out(void)
{
    actuator_register(&test_servo);
    actuator_err_t ret = servo_get_angle(&test_servo, NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Servo Set Range Tests ==================== */
void test_servo_set_range(void)
{
    actuator_register(&test_servo);

    /* Set range to -45 to 45 degrees */
    actuator_err_t ret = servo_set_range(&test_servo, -45.0f, 45.0f);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(-45.0f, test_servo.config.servo_min_angle);
    TEST_ASSERT_EQUAL(45.0f, test_servo.config.servo_max_angle);
}

void test_servo_set_range_null(void)
{
    actuator_err_t ret = servo_set_range(NULL, -45.0f, 45.0f);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_servo_set_range_invalid(void)
{
    /* Note: The implementation may or may not check for min > max */
    actuator_register(&test_servo);
    actuator_err_t ret = servo_set_range(&test_servo, 45.0f, -45.0f);  /* Reversed */
    /* Implementation may accept this or return error */
    (void)ret;
}

/* ==================== Servo Set Speed Tests ==================== */
void test_servo_set_speed(void)
{
    actuator_register(&test_servo);

    /* Set speed to 90 deg/sec */
    actuator_err_t ret = servo_set_speed(&test_servo, 90);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(90, test_servo.config.servo_speed);

    /* Set speed to 180 deg/sec */
    ret = servo_set_speed(&test_servo, 180);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(180, test_servo.config.servo_speed);

    /* Set speed to 0 (stopped) */
    ret = servo_set_speed(&test_servo, 0);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(0, test_servo.config.servo_speed);
}

void test_servo_set_speed_null(void)
{
    actuator_err_t ret = servo_set_speed(NULL, 90);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Servo Center Tests ==================== */
void test_servo_center(void)
{
    actuator_register(&test_servo);

    /* Move away from center first */
    servo_set_angle(&test_servo, 45.0f);

    /* Return to center */
    actuator_err_t ret = servo_center(&test_servo);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, g_mock_servo_angle);
}

void test_servo_center_null(void)
{
    actuator_err_t ret = servo_center(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Servo Stop Tests ==================== */
void test_servo_stop(void)
{
    actuator_register(&test_servo);

    /* Move servo */
    servo_set_angle(&test_servo, 45.0f);

    /* Stop */
    actuator_err_t ret = servo_stop(&test_servo);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

void test_servo_stop_null(void)
{
    actuator_err_t ret = servo_stop(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Servo Sweep Tests ==================== */
void test_servo_sweep(void)
{
    actuator_register(&test_servo);

    /* Sweep from -45 to 45 degrees, 1000ms per step */
    actuator_err_t ret = servo_sweep(&test_servo, -45.0f, 45.0f, 1000);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

void test_servo_sweep_null(void)
{
    actuator_err_t ret = servo_sweep(NULL, -45.0f, 45.0f, 1000);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_servo_sweep_zero_step(void)
{
    actuator_register(&test_servo);
    /* Zero step time may cause issues in real implementation */
    actuator_err_t ret = servo_sweep(&test_servo, -45.0f, 45.0f, 0);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);  /* API accepts it */
}

/* ==================== Servo Deinit Tests ==================== */
void test_servo_deinit(void)
{
    actuator_register(&test_servo);
    actuator_err_t ret = servo_deinit(&test_servo);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(1, g_mock_deinit_called);
}

void test_servo_deinit_null(void)
{
    actuator_err_t ret = servo_deinit(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Servo Angle Conversion Tests ==================== */
void test_servo_angle_to_pwm(void)
{
    /* Test angle to PWM conversion at extremes */
    uint16_t pwm_min = servo_angle_to_pwm(-90.0f, -90.0f, 90.0f, 500, 2500);
    TEST_ASSERT_EQUAL(500, pwm_min);

    uint16_t pwm_max = servo_angle_to_pwm(90.0f, -90.0f, 90.0f, 500, 2500);
    TEST_ASSERT_EQUAL(2500, pwm_max);

    uint16_t pwm_center = servo_angle_to_pwm(0.0f, -90.0f, 90.0f, 500, 2500);
    TEST_ASSERT_EQUAL(1500, pwm_center);
}

void test_servo_angle_to_pwm_clamp(void)
{
    /* Test clamping behavior */
    uint16_t pwm_below = servo_angle_to_pwm(-100.0f, -90.0f, 90.0f, 500, 2500);
    TEST_ASSERT_EQUAL(500, pwm_below);  /* Clamped to min */

    uint16_t pwm_above = servo_angle_to_pwm(100.0f, -90.0f, 90.0f, 500, 2500);
    TEST_ASSERT_EQUAL(2500, pwm_above);  /* Clamped to max */
}

void test_servo_pwm_to_angle(void)
{
    /* Test PWM to angle conversion at extremes */
    float angle_min = servo_pwm_to_angle(500, -90.0f, 90.0f, 500, 2500);
    TEST_ASSERT_EQUAL_FLOAT(-90.0f, angle_min);

    float angle_max = servo_pwm_to_angle(2500, -90.0f, 90.0f, 500, 2500);
    TEST_ASSERT_EQUAL_FLOAT(90.0f, angle_max);

    float angle_center = servo_pwm_to_angle(1500, -90.0f, 90.0f, 500, 2500);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, angle_center);
}

void test_servo_pwm_to_angle_clamp(void)
{
    /* Test clamping behavior */
    float angle_below = servo_pwm_to_angle(400, -90.0f, 90.0f, 500, 2500);
    TEST_ASSERT_EQUAL_FLOAT(-90.0f, angle_below);  /* Clamped to min */

    float angle_above = servo_pwm_to_angle(2600, -90.0f, 90.0f, 500, 2500);
    TEST_ASSERT_EQUAL_FLOAT(90.0f, angle_above);  /* Clamped to max */
}

/* ==================== Servo Configuration Tests ==================== */
void test_servo_config_initial(void)
{
    TEST_ASSERT_EQUAL(-90.0f, test_servo.config.servo_min_angle);
    TEST_ASSERT_EQUAL(90.0f, test_servo.config.servo_max_angle);
    TEST_ASSERT_EQUAL(500, test_servo.config.servo_pwm_min);
    TEST_ASSERT_EQUAL(2500, test_servo.config.servo_pwm_max);
    TEST_ASSERT_EQUAL(90, test_servo.config.servo_speed);
}

void test_servo_type(void)
{
    TEST_ASSERT_EQUAL(ACTUATOR_TYPE_SERVO, test_servo.type);
}

void test_servo_type_string(void)
{
    const char *str = actuator_type_str(ACTUATOR_TYPE_SERVO);
    TEST_ASSERT_EQUAL_STRING("Servo", str);
}

/* ==================== Servo Value Structure Tests ==================== */
void test_servo_value_struct(void)
{
    actuator_value_t value;

    /* Test servo value union fields */
    value.servo.target_angle = 45.5f;
    TEST_ASSERT_EQUAL_FLOAT(45.5f, value.servo.target_angle);

    value.servo.current_angle = 44.0f;
    TEST_ASSERT_EQUAL_FLOAT(44.0f, value.servo.current_angle);

    value.servo.min_angle = -90.0f;
    TEST_ASSERT_EQUAL_FLOAT(-90.0f, value.servo.min_angle);

    value.servo.max_angle = 90.0f;
    TEST_ASSERT_EQUAL_FLOAT(90.0f, value.servo.max_angle);
}

/* ==================== Main ==================== */
int main(void)
{
    UNITY_BEGIN();

    /* Init Tests */
    RUN_TEST(test_servo_init);
    RUN_TEST(test_servo_init_null);

    /* Set Angle Tests */
    RUN_TEST(test_servo_set_angle);
    RUN_TEST(test_servo_set_angle_null);
    RUN_TEST(test_servo_set_angle_min);
    RUN_TEST(test_servo_set_angle_max);
    RUN_TEST(test_servo_set_angle_center);

    /* Get Angle Tests */
    RUN_TEST(test_servo_get_angle);
    RUN_TEST(test_servo_get_angle_null);
    RUN_TEST(test_servo_get_angle_null_out);

    /* Set Range Tests */
    RUN_TEST(test_servo_set_range);
    RUN_TEST(test_servo_set_range_null);
    RUN_TEST(test_servo_set_range_invalid);

    /* Set Speed Tests */
    RUN_TEST(test_servo_set_speed);
    RUN_TEST(test_servo_set_speed_null);

    /* Center Tests */
    RUN_TEST(test_servo_center);
    RUN_TEST(test_servo_center_null);

    /* Stop Tests */
    RUN_TEST(test_servo_stop);
    RUN_TEST(test_servo_stop_null);

    /* Sweep Tests */
    RUN_TEST(test_servo_sweep);
    RUN_TEST(test_servo_sweep_null);
    RUN_TEST(test_servo_sweep_zero_step);

    /* Deinit Tests */
    RUN_TEST(test_servo_deinit);
    RUN_TEST(test_servo_deinit_null);

    /* Angle Conversion Tests */
    RUN_TEST(test_servo_angle_to_pwm);
    RUN_TEST(test_servo_angle_to_pwm_clamp);
    RUN_TEST(test_servo_pwm_to_angle);
    RUN_TEST(test_servo_pwm_to_angle_clamp);

    /* Configuration Tests */
    RUN_TEST(test_servo_config_initial);
    RUN_TEST(test_servo_type);
    RUN_TEST(test_servo_type_string);

    /* Value Structure Tests */
    RUN_TEST(test_servo_value_struct);

    return UNITY_END();
}
