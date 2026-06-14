#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

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

    assert(xy_pid_init(NULL, &config) == XY_PID_INVALID_PARAM);
    assert(xy_pid_init(&pid, NULL) == XY_PID_INVALID_PARAM);
    assert(xy_pid_init(&pid, &config) == XY_PID_OK);

    assert(pid.mode == XY_PID_MODE_MANUAL);
    assert(pid.first_run == true);
    assert(pid.anti_windup == true);
    assert(fabsf(pid.config.kp - 2.0F) < 0.001F);

    assert(xy_pid_set_tuning(&pid, 3.0F, 0.75F, 0.5F) == XY_PID_OK);
    assert(fabsf(pid.config.kp - 3.0F) < 0.001F);
    assert(fabsf(pid.config.ki - 0.75F) < 0.001F);
    assert(fabsf(pid.config.kd - 0.5F) < 0.001F);
}

static void test_pid_compute_and_limits(void)
{
    xy_pid_t pid;
    xy_pid_config_t config = default_config();
    float output = 123.0F;

    assert(xy_pid_init(&pid, &config) == XY_PID_OK);
    assert(xy_pid_set_setpoint(&pid, 10.0F) == XY_PID_OK);

    g_tick_ms = 100U;
    assert(xy_pid_compute(&pid, 4.0F, &output) == XY_PID_OK);
    assert(output == 0.0F);
    assert(pid.first_run == false);
    assert(fabsf(xy_pid_get_error(&pid) - 6.0F) < 0.001F);

    g_tick_ms = 110U;
    assert(xy_pid_compute(&pid, 4.0F, &output) == XY_PID_OK);
    assert(output > 0.0F);
    assert(output <= 100.0F);
    assert(pid.update_count == 1U);

    assert(xy_pid_set_output_limits(&pid, -5.0F, 5.0F) == XY_PID_OK);
    g_tick_ms = 120U;
    assert(xy_pid_compute(&pid, -100.0F, &output) == XY_PID_OK);
    assert(output <= 5.0F);

    assert(xy_pid_set_output_limits(&pid, 5.0F, 5.0F) == XY_PID_INVALID_PARAM);
}

static void test_pid_modes_and_filters(void)
{
    xy_pid_t pid;
    xy_pid_config_t config = default_config();

    assert(xy_pid_init(&pid, &config) == XY_PID_OK);
    assert(xy_pid_get_mode(&pid) == XY_PID_MODE_MANUAL);
    assert(xy_pid_set_mode(&pid, XY_PID_MODE_AUTO) == XY_PID_OK);
    assert(xy_pid_get_mode(&pid) == XY_PID_MODE_AUTO);

    assert(xy_pid_enable_anti_windup(&pid, false) == XY_PID_OK);
    assert(pid.anti_windup == false);
    assert(xy_pid_enable_anti_windup(NULL, true) == XY_PID_INVALID_PARAM);

    assert(xy_pid_enable_derivative_filter(&pid, true, 0.25F) == XY_PID_OK);
    assert(pid.derivative_filter == true);
    assert(fabsf(pid.config.derivative_filter - 0.25F) < 0.001F);
    assert(xy_pid_enable_derivative_filter(&pid, true, 1.5F) == XY_PID_INVALID_PARAM);
}

static void test_pid_reset(void)
{
    xy_pid_t pid;
    xy_pid_config_t config = default_config();
    float output;

    assert(xy_pid_init(&pid, &config) == XY_PID_OK);
    assert(xy_pid_set_setpoint(&pid, 10.0F) == XY_PID_OK);
    g_tick_ms = 200U;
    assert(xy_pid_compute(&pid, 5.0F, &output) == XY_PID_OK);
    g_tick_ms = 210U;
    assert(xy_pid_compute(&pid, 5.0F, &output) == XY_PID_OK);
    assert(pid.update_count > 0U);

    assert(xy_pid_reset(&pid) == XY_PID_OK);
    assert(pid.setpoint == 0.0F);
    assert(pid.input == 0.0F);
    assert(pid.output == 0.0F);
    assert(pid.error == 0.0F);
    assert(pid.error_prev == 0.0F);
    assert(pid.integral_raw == 0.0F);
    assert(pid.first_run == true);
    assert(pid.update_count == 0U);
    assert(xy_pid_reset(NULL) == XY_PID_INVALID_PARAM);
}

int main(void)
{
    test_pid_init_and_tuning();
    test_pid_compute_and_limits();
    test_pid_modes_and_filters();
    test_pid_reset();
    puts("PID core tests passed");
    return 0;
}
