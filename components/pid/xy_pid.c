/**
 * @file xy_pid.c
 * @brief PID Controller Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_pid.h"
#include <string.h>

/* ==================== Helper Functions ==================== */

static pid_fixed_t clamp(pid_fixed_t value, pid_fixed_t min, pid_fixed_t max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

/* ==================== Implementation ==================== */

int xy_pid_init(xy_pid_t *pid, pid_fixed_t Kp, pid_fixed_t Ki, pid_fixed_t Kd,
                uint32_t sample_time_ms)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }

    memset(pid, 0, sizeof(*pid));

    pid->gain.Kp = Kp;
    pid->gain.Ki = Ki;
    pid->gain.Kd = Kd;
    pid->setpoint = 0;
    pid->integral = 0;
    pid->prev_error = 0;
    pid->output = 0;
    pid->integral_limit = 0;
    pid->output_limit = 0;
    pid->sample_time_ms = sample_time_ms;
    pid->anti_windup = true;
    pid->initialized = true;

    return XY_PID_OK;
}

int xy_pid_deinit(xy_pid_t *pid)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }

    pid->initialized = false;
    return XY_PID_OK;
}

int xy_pid_set_gains(xy_pid_t *pid, pid_fixed_t Kp, pid_fixed_t Ki, pid_fixed_t Kd)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }

    pid->gain.Kp = Kp;
    pid->gain.Ki = Ki;
    pid->gain.Kd = Kd;

    return XY_PID_OK;
}

int xy_pid_set_setpoint(xy_pid_t *pid, pid_fixed_t setpoint)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }

    pid->setpoint = setpoint;
    return XY_PID_OK;
}

int xy_pid_set_output_limit(xy_pid_t *pid, pid_fixed_t min_limit, pid_fixed_t max_limit)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }

    pid->output_limit = max_limit - min_limit;
    return XY_PID_OK;
}

int xy_pid_set_integral_limit(xy_pid_t *pid, pid_fixed_t limit)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }

    pid->integral_limit = limit;
    return XY_PID_OK;
}

int xy_pid_compute(xy_pid_t *pid, pid_fixed_t measurement, xy_pid_output_t *output)
{
    if (!pid || !output) {
        return XY_PID_INVALID_PARAM;
    }

    if (!pid->initialized) {
        return XY_PID_ERROR;
    }

    /* 计算误差 */
    pid_fixed_t error = pid->setpoint - measurement;

    /* 比例项 */
    pid_fixed_t p_term = pid->gain.Kp * error / PID_FIXED_ONE;

    /* 积分项 */
    pid->integral += error;

    /* 积分限幅 */
    if (pid->integral_limit > 0) {
        pid->integral = clamp(pid->integral, -pid->integral_limit, pid->integral_limit);
    }

    pid_fixed_t i_term = pid->gain.Ki * pid->integral / PID_FIXED_ONE;

    /* 微分项 */
    pid_fixed_t derivative = error - pid->prev_error;
    pid_fixed_t d_term = pid->gain.Kd * derivative / PID_FIXED_ONE;

    /* 计算总输出 */
    pid_fixed_t raw_output = p_term + i_term + d_term;

    /* 输出限幅 */
    output->saturated = false;
    if (pid->output_limit > 0) {
        pid_fixed_t limited = clamp(raw_output, -pid->output_limit, pid->output_limit);
        if (limited != raw_output) {
            output->saturated = true;
            /* 抗积分饱和 */
            if (pid->anti_windup) {
                pid->integral -= error;
            }
        }
        raw_output = limited;
    }

    /* 更新状态 */
    pid->output = raw_output;
    pid->prev_error = error;

    /* 填充输出结构 */
    output->output = raw_output;
    output->error = error;
    output->integral = pid->integral;
    output->derivative = derivative;

    return XY_PID_OK;
}

int xy_pid_reset(xy_pid_t *pid)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }

    pid->integral = 0;
    pid->prev_error = 0;
    pid->output = 0;

    return XY_PID_OK;
}

int xy_pid_set_ant_windup(xy_pid_t *pid, bool enable)
{
    if (!pid) {
        return XY_PID_INVALID_PARAM;
    }

    pid->anti_windup = enable;
    return XY_PID_OK;
}

int xy_pid_get_state(xy_pid_t *pid, xy_pid_output_t *output)
{
    if (!pid || !output) {
        return XY_PID_INVALID_PARAM;
    }

    output->output = pid->output;
    output->error = pid->setpoint - pid->output;
    output->integral = pid->integral;
    output->derivative = 0;
    output->saturated = false;

    return XY_PID_OK;
}
