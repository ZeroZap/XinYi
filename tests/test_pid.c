/**
 * @file test_pid.c
 * @brief PID Controller Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* PID header */
#include "xy_pid.h"

/* ==================== Test Fixtures ==================== */

static xy_pid_t test_pid;
static xy_pid_output_t test_output;

void setUp(void)
{
    memset(&test_pid, 0, sizeof(test_pid));
    memset(&test_output, 0, sizeof(test_output));
}

void tearDown(void)
{
    xy_pid_deinit(&test_pid);
}

/* ==================== Helper Functions ==================== */

static void pid_assert_ok(int result)
{
    TEST_ASSERT_EQUAL(XY_PID_OK, result);
}

/* ==================== PID Init Tests ==================== */

void test_pid_init(void)
{
    int result;

    /* Test valid initialization */
    result = xy_pid_init(&test_pid, 1000, 500, 200, 10);
    pid_assert_ok(result);
    TEST_ASSERT_TRUE(test_pid.initialized);
    TEST_ASSERT_EQUAL(1000, test_pid.gain.Kp);
    TEST_ASSERT_EQUAL(500, test_pid.gain.Ki);
    TEST_ASSERT_EQUAL(200, test_pid.gain.Kd);
    TEST_ASSERT_EQUAL(10, test_pid.sample_time_ms);
}

void test_pid_init_invalid_params(void)
{
    int result;

    /* Test NULL pointer */
    result = xy_pid_init(NULL, 1000, 500, 200, 10);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, result);
}

void test_pid_deinit(void)
{
    xy_pid_init(&test_pid, 1000, 500, 200, 10);

    int result = xy_pid_deinit(&test_pid);
    pid_assert_ok(result);
    TEST_ASSERT_FALSE(test_pid.initialized);
}

void test_pid_deinit_invalid_params(void)
{
    int result = xy_pid_deinit(NULL);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, result);
}

/* ==================== PID Gain Tests ==================== */

void test_pid_set_gains(void)
{
    xy_pid_init(&test_pid, 0, 0, 0, 10);

    int result = xy_pid_set_gains(&test_pid, 2000, 1000, 500);
    pid_assert_ok(result);

    TEST_ASSERT_EQUAL(2000, test_pid.gain.Kp);
    TEST_ASSERT_EQUAL(1000, test_pid.gain.Ki);
    TEST_ASSERT_EQUAL(500, test_pid.gain.Kd);
}

void test_pid_set_gains_invalid_params(void)
{
    int result = xy_pid_set_gains(NULL, 1000, 500, 200);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, result);
}

/* ==================== PID Setpoint Tests ==================== */

void test_pid_set_setpoint(void)
{
    xy_pid_init(&test_pid, 1000, 500, 200, 10);

    int result = xy_pid_set_setpoint(&test_pid, 5000);
    pid_assert_ok(result);
    TEST_ASSERT_EQUAL(5000, test_pid.setpoint);
}

void test_pid_set_setpoint_invalid_params(void)
{
    int result = xy_pid_set_setpoint(NULL, 5000);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, result);
}

/* ==================== PID Limit Tests ==================== */

void test_pid_set_output_limit(void)
{
    xy_pid_init(&test_pid, 1000, 500, 200, 10);

    int result = xy_pid_set_output_limit(&test_pid, 0, 10000);
    pid_assert_ok(result);
    TEST_ASSERT_EQUAL(10000, test_pid.output_limit);
}

void test_pid_set_integral_limit(void)
{
    xy_pid_init(&test_pid, 1000, 500, 200, 10);

    int result = xy_pid_set_integral_limit(&test_pid, 5000);
    pid_assert_ok(result);
    TEST_ASSERT_EQUAL(5000, test_pid.integral_limit);
}

/* ==================== PID Compute Tests ==================== */

void test_pid_compute_basic(void)
{
    xy_pid_init(&test_pid, 1000, 0, 0, 10); /* P-only controller */
    xy_pid_set_setpoint(&test_pid, 1000);

    int result = xy_pid_compute(&test_pid, 500, &test_output);
    pid_assert_ok(result);

    /* Error = 1000 - 500 = 500 */
    /* P_term = 1000 * 500 / 1024 = ~488 (fixed point) */
    TEST_ASSERT_EQUAL(500, test_output.error);
    TEST_ASSERT_TRUE(test_output.output > 0);
}

void test_pid_compute_with_integral(void)
{
    xy_pid_init(&test_pid, 1000, 100, 0, 10); /* PI controller */
    xy_pid_set_setpoint(&test_pid, 1000);

    /* First computation */
    xy_pid_compute(&test_pid, 500, &test_output);
    pid_fixed_t first_integral = test_output.integral;

    /* Second computation - integral should accumulate */
    xy_pid_compute(&test_pid, 500, &test_output);
    TEST_ASSERT_TRUE(test_output.integral > first_integral);
}

void test_pid_compute_with_derivative(void)
{
    xy_pid_init(&test_pid, 1000, 0, 500, 10); /* PD controller */
    xy_pid_set_setpoint(&test_pid, 1000);

    /* First computation */
    xy_pid_compute(&test_pid, 500, &test_output);
    pid_fixed_t first_derivative = test_output.derivative;

    /* Second computation with same error - derivative should be 0 */
    xy_pid_compute(&test_pid, 500, &test_output);
    TEST_ASSERT_EQUAL(0, test_output.derivative);
    TEST_ASSERT_NOT_EQUAL(first_derivative, test_output.derivative);
}

void test_pid_compute_invalid_params(void)
{
    int result;

    /* NULL pid */
    result = xy_pid_compute(NULL, 500, &test_output);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, result);

    /* NULL output */
    xy_pid_init(&test_pid, 1000, 500, 200, 10);
    result = xy_pid_compute(&test_pid, 500, NULL);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, result);
}

void test_pid_compute_not_initialized(void)
{
    /* Don't initialize PID */
    int result = xy_pid_compute(&test_pid, 500, &test_output);
    TEST_ASSERT_EQUAL(XY_PID_ERROR, result);
}

/* ==================== PID Reset Tests ==================== */

void test_pid_reset(void)
{
    xy_pid_init(&test_pid, 1000, 500, 200, 10);
    xy_pid_set_setpoint(&test_pid, 1000);

    /* Compute to accumulate integral */
    xy_pid_compute(&test_pid, 500, &test_output);
    pid_fixed_t integral_before = test_pid.integral;

    /* Reset */
    int result = xy_pid_reset(&test_pid);
    pid_assert_ok(result);

    TEST_ASSERT_EQUAL(0, test_pid.integral);
    TEST_ASSERT_EQUAL(0, test_pid.prev_error);
    TEST_ASSERT_EQUAL(0, test_pid.output);
}

void test_pid_reset_invalid_params(void)
{
    int result = xy_pid_reset(NULL);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, result);
}

/* ==================== PID Anti Windup Tests ==================== */

void test_pid_set_ant_windup(void)
{
    xy_pid_init(&test_pid, 1000, 500, 200, 10);

    /* Enable anti-windup */
    int result = xy_pid_set_ant_windup(&test_pid, true);
    pid_assert_ok(result);
    TEST_ASSERT_TRUE(test_pid.anti_windup);

    /* Disable anti-windup */
    result = xy_pid_set_ant_windup(&test_pid, false);
    pid_assert_ok(result);
    TEST_ASSERT_FALSE(test_pid.anti_windup);
}

void test_pid_set_ant_windup_invalid_params(void)
{
    int result = xy_pid_set_ant_windup(NULL, true);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, result);
}

/* ==================== PID State Tests ==================== */

void test_pid_get_state(void)
{
    xy_pid_init(&test_pid, 1000, 500, 200, 10);
    xy_pid_set_setpoint(&test_pid, 1000);

    int result = xy_pid_get_state(&test_pid, &test_output);
    pid_assert_ok(result);

    TEST_ASSERT_EQUAL(0, test_output.output);
    TEST_ASSERT_EQUAL(1000, test_output.error);
}

void test_pid_get_state_invalid_params(void)
{
    int result;

    /* NULL pid */
    result = xy_pid_get_state(NULL, &test_output);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, result);

    /* NULL output */
    xy_pid_init(&test_pid, 1000, 500, 200, 10);
    result = xy_pid_get_state(&test_pid, NULL);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, result);
}

/* ==================== PID Saturation Tests ==================== */

void test_pid_output_saturation(void)
{
    xy_pid_init(&test_pid, 10000, 0, 0, 10); /* High Kp */
    xy_pid_set_setpoint(&test_pid, 1000);
    xy_pid_set_output_limit(&test_pid, 0, 5000); /* Limit to 5000 */

    /* Large error should cause saturation */
    xy_pid_compute(&test_pid, 0, &test_output);

    TEST_ASSERT_TRUE(test_output.saturated);
    TEST_ASSERT_TRUE(test_output.output <= 5000);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Init Tests */
    RUN_TEST(test_pid_init);
    RUN_TEST(test_pid_init_invalid_params);
    RUN_TEST(test_pid_deinit);
    RUN_TEST(test_pid_deinit_invalid_params);

    /* Gain Tests */
    RUN_TEST(test_pid_set_gains);
    RUN_TEST(test_pid_set_gains_invalid_params);

    /* Setpoint Tests */
    RUN_TEST(test_pid_set_setpoint);
    RUN_TEST(test_pid_set_setpoint_invalid_params);

    /* Limit Tests */
    RUN_TEST(test_pid_set_output_limit);
    RUN_TEST(test_pid_set_integral_limit);

    /* Compute Tests */
    RUN_TEST(test_pid_compute_basic);
    RUN_TEST(test_pid_compute_with_integral);
    RUN_TEST(test_pid_compute_with_derivative);
    RUN_TEST(test_pid_compute_invalid_params);
    RUN_TEST(test_pid_compute_not_initialized);

    /* Reset Tests */
    RUN_TEST(test_pid_reset);
    RUN_TEST(test_pid_reset_invalid_params);

    /* Anti Windup Tests */
    RUN_TEST(test_pid_set_ant_windup);
    RUN_TEST(test_pid_set_ant_windup_invalid_params);

    /* State Tests */
    RUN_TEST(test_pid_get_state);
    RUN_TEST(test_pid_get_state_invalid_params);

    /* Saturation Tests */
    RUN_TEST(test_pid_output_saturation);

    return UNITY_END();
}
