#include <math.h>
#include <stdint.h>

#include "unity.h"
#include "xy_pid.h"

static uint32_t g_tick_ms;

uint32_t xy_os_tick_get(void)
{
    return g_tick_ms;
}

void xy_log_char(char ch)
{
    (void)ch;
}

static xy_pid_config_t default_config(void)
{
    xy_pid_config_t config = {
        .kp = 2.0F,
        .ki = 0.5F,
        .kd = 0.25F,
        .output_min = -100.0F,
        .output_max = 100.0F,
        .integral_min = -20.0F,
        .integral_max = 20.0F,
        .derivative_filter = 0.1F,
    };
    return config;
}

static void test_pid_init_and_tuning(void)
{
    xy_pid_t pid;
    xy_pid_config_t config = default_config();
    xy_pid_config_t before;

    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_init(NULL, &config));
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_init(&pid, NULL));
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &config));

    TEST_ASSERT_EQUAL(XY_PID_MODE_MANUAL, pid.mode);
    TEST_ASSERT_TRUE(pid.first_run);
    TEST_ASSERT_TRUE(pid.anti_windup);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 2.0F, pid.config.kp);

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_set_tuning(&pid, 3.0F, 0.75F, 0.5F));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 3.0F, pid.config.kp);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.75F, pid.config.ki);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.5F, pid.config.kd);

    before = pid.config;
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_set_tuning(NULL, 1.0F, 0.75F, 0.5F));
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_set_tuning(&pid, -1.0F, 0.75F, 0.5F));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, before.kp, pid.config.kp);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, before.ki, pid.config.ki);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, before.kd, pid.config.kd);

    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_set_tuning(&pid, 1.0F, -0.1F, 0.5F));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, before.kp, pid.config.kp);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, before.ki, pid.config.ki);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, before.kd, pid.config.kd);

    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_set_tuning(&pid, 1.0F, 0.1F, -0.5F));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, before.kp, pid.config.kp);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, before.ki, pid.config.ki);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, before.kd, pid.config.kd);
}

static void test_pid_compute_and_limits(void)
{
    xy_pid_t pid;
    xy_pid_config_t config = default_config();
    xy_pid_config_t before;
    float output = 123.0F;

    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_compute(NULL, 4.0F, &output));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 123.0F, output);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_compute(&pid, 4.0F, NULL));
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_set_setpoint(NULL, 10.0F));
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_set_input(NULL, 4.0F));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, xy_pid_get_error(NULL));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, xy_pid_get_integral(NULL));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, xy_pid_get_derivative(NULL));

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &config));
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_set_setpoint(&pid, 10.0F));
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_set_input(&pid, 3.0F));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 3.0F, pid.input);

    g_tick_ms = 100U;
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_compute(&pid, 4.0F, &output));
    TEST_ASSERT_EQUAL_FLOAT(0.0F, output);
    TEST_ASSERT_FALSE(pid.first_run);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 6.0F, xy_pid_get_error(&pid));

    g_tick_ms = 110U;
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_compute(&pid, 4.0F, &output));
    TEST_ASSERT_GREATER_THAN_FLOAT(0.0F, output);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(100.0F, output);
    TEST_ASSERT_EQUAL_UINT32(1U, pid.update_count);
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(0.0F, xy_pid_get_derivative(&pid));

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_set_output_limits(&pid, -5.0F, 5.0F));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, -5.0F, pid.config.integral_min);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 5.0F, pid.config.integral_max);
    g_tick_ms = 120U;
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_compute(&pid, -100.0F, &output));
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(5.0F, output);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 5.0F, xy_pid_get_integral(&pid));

    before = pid.config;
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_set_output_limits(NULL, -5.0F, 5.0F));
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_set_output_limits(&pid, 5.0F, 5.0F));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, before.output_min, pid.config.output_min);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, before.output_max, pid.config.output_max);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, before.integral_min, pid.config.integral_min);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, before.integral_max, pid.config.integral_max);
}

static void test_pid_modes_and_filters(void)
{
    xy_pid_t pid;
    xy_pid_config_t config = default_config();

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &config));
    TEST_ASSERT_EQUAL(XY_PID_MODE_MANUAL, xy_pid_get_mode(&pid));
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_set_mode(&pid, XY_PID_MODE_AUTO));
    TEST_ASSERT_EQUAL(XY_PID_MODE_AUTO, xy_pid_get_mode(&pid));
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_set_mode(&pid, (xy_pid_mode_t)99));
    TEST_ASSERT_EQUAL(XY_PID_MODE_AUTO, xy_pid_get_mode(&pid));
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_set_mode(NULL, XY_PID_MODE_MANUAL));
    TEST_ASSERT_EQUAL(XY_PID_MODE_MANUAL, xy_pid_get_mode(NULL));

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_enable_anti_windup(&pid, false));
    TEST_ASSERT_FALSE(pid.anti_windup);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, -20.0F, pid.config.integral_min);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 20.0F, pid.config.integral_max);
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_set_output_limits(&pid, -3.0F, 3.0F));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, -20.0F, pid.config.integral_min);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 20.0F, pid.config.integral_max);
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_enable_anti_windup(&pid, true));
    TEST_ASSERT_TRUE(pid.anti_windup);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, -3.0F, pid.config.integral_min);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 3.0F, pid.config.integral_max);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_enable_anti_windup(NULL, true));

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_enable_derivative_filter(&pid, true, 0.25F));
    TEST_ASSERT_TRUE(pid.derivative_filter);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.25F, pid.config.derivative_filter);
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_enable_derivative_filter(&pid, false, 0.0F));
    TEST_ASSERT_FALSE(pid.derivative_filter);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.25F, pid.config.derivative_filter);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_enable_derivative_filter(NULL, true, 0.25F));
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_enable_derivative_filter(&pid, true, 1.5F));
    TEST_ASSERT_FALSE(pid.derivative_filter);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.25F, pid.config.derivative_filter);
}

static void test_pid_reset(void)
{
    xy_pid_t pid;
    xy_pid_config_t config = default_config();
    float output;

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &config));
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_set_setpoint(&pid, 10.0F));
    g_tick_ms = 200U;
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_compute(&pid, 5.0F, &output));
    g_tick_ms = 210U;
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_compute(&pid, 5.0F, &output));
    TEST_ASSERT_GREATER_THAN_UINT32(0U, pid.update_count);

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_reset(&pid));
    TEST_ASSERT_EQUAL_FLOAT(0.0F, pid.setpoint);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, pid.input);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, pid.output);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, pid.error);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, pid.error_prev);
    TEST_ASSERT_EQUAL_FLOAT(0.0F, pid.integral_raw);
    TEST_ASSERT_TRUE(pid.first_run);
    TEST_ASSERT_EQUAL_UINT32(0U, pid.update_count);
    TEST_ASSERT_EQUAL(XY_PID_INVALID_PARAM, xy_pid_reset(NULL));
}

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pid_init_and_tuning);
    RUN_TEST(test_pid_compute_and_limits);
    RUN_TEST(test_pid_modes_and_filters);
    RUN_TEST(test_pid_reset);
    return UNITY_END();
}
