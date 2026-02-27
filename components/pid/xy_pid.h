/**
 * @file xy_pid.h
 * @brief PID Controller Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_PID_H
#define XY_PID_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef PID_USE_FLOAT
#define PID_USE_FLOAT 0  /* 0=定点数，1=浮点数 */
#endif

/* ==================== Error Codes ==================== */

#define XY_PID_OK               0
#define XY_PID_ERROR            (-1)
#define XY_PID_INVALID_PARAM    (-2)

/* ==================== Data Structures ==================== */

#if PID_USE_FLOAT
typedef float pid_fixed_t;
#else
typedef int32_t pid_fixed_t;
#define PID_FIXED_SHIFT   16
#define PID_FIXED_ONE     (1 << PID_FIXED_SHIFT)
#endif

/**
 * @brief PID 参数结构
 */
typedef struct {
    pid_fixed_t Kp;  /* 比例增益 */
    pid_fixed_t Ki;  /* 积分增益 */
    pid_fixed_t Kd;  /* 微分增益 */
} xy_pid_gain_t;

/**
 * @brief PID 控制器结构
 */
typedef struct {
    xy_pid_gain_t gain;         /* PID 增益 */
    pid_fixed_t setpoint;       /* 设定点 */
    pid_fixed_t integral;       /* 积分累积 */
    pid_fixed_t prev_error;     /* 上次误差 */
    pid_fixed_t output;         /* 当前输出 */
    pid_fixed_t integral_limit; /* 积分限幅 */
    pid_fixed_t output_limit;   /* 输出限幅 */
    uint32_t sample_time_ms;    /* 采样时间 (ms) */
    bool anti_windup;           /* 抗积分饱和 */
    bool initialized;           /* 初始化标志 */
} xy_pid_t;

/**
 * @brief PID 输出结构
 */
typedef struct {
    pid_fixed_t output;         /* 控制器输出 */
    pid_fixed_t error;          /* 当前误差 */
    pid_fixed_t integral;       /* 积分项 */
    pid_fixed_t derivative;     /* 微分项 */
    bool saturated;             /* 是否饱和 */
} xy_pid_output_t;

/* ==================== PID Operations ==================== */

/**
 * @brief 初始化 PID 控制器
 * @param pid PID 控制器句柄
 * @param Kp 比例增益
 * @param Ki 积分增益
 * @param Kd 微分增益
 * @param sample_time_ms 采样时间 (ms)
 * @return XY_PID_OK 成功
 */
int xy_pid_init(xy_pid_t *pid, pid_fixed_t Kp, pid_fixed_t Ki, pid_fixed_t Kd,
                uint32_t sample_time_ms);

/**
 * @brief 反初始化 PID 控制器
 * @param pid PID 控制器句柄
 * @return XY_PID_OK 成功
 */
int xy_pid_deinit(xy_pid_t *pid);

/**
 * @brief 设置 PID 增益
 * @param pid PID 控制器句柄
 * @param Kp 比例增益
 * @param Ki 积分增益
 * @param Kd 微分增益
 * @return XY_PID_OK 成功
 */
int xy_pid_set_gains(xy_pid_t *pid, pid_fixed_t Kp, pid_fixed_t Ki, pid_fixed_t Kd);

/**
 * @brief 设置设定点
 * @param pid PID 控制器句柄
 * @param setpoint 设定点
 * @return XY_PID_OK 成功
 */
int xy_pid_set_setpoint(xy_pid_t *pid, pid_fixed_t setpoint);

/**
 * @brief 设置输出限幅
 * @param pid PID 控制器句柄
 * @param min_limit 最小输出
 * @param max_limit 最大输出
 * @return XY_PID_OK 成功
 */
int xy_pid_set_output_limit(xy_pid_t *pid, pid_fixed_t min_limit, pid_fixed_t max_limit);

/**
 * @brief 设置积分限幅
 * @param pid PID 控制器句柄
 * @param limit 积分限幅值
 * @return XY_PID_OK 成功
 */
int xy_pid_set_integral_limit(xy_pid_t *pid, pid_fixed_t limit);

/**
 * @brief 执行 PID 计算
 * @param pid PID 控制器句柄
 * @param measurement 当前测量值
 * @param output PID 输出结构
 * @return XY_PID_OK 成功
 */
int xy_pid_compute(xy_pid_t *pid, pid_fixed_t measurement, xy_pid_output_t *output);

/**
 * @brief 重置 PID 控制器
 * @param pid PID 控制器句柄
 * @return XY_PID_OK 成功
 */
int xy_pid_reset(xy_pid_t *pid);

/**
 * @brief 启用/禁用抗积分饱和
 * @param pid PID 控制器句柄
 * @param enable true 启用
 * @return XY_PID_OK 成功
 */
int xy_pid_set_ant_windup(xy_pid_t *pid, bool enable);

/**
 * @brief 获取 PID 状态
 * @param pid PID 控制器句柄
 * @param output 输出结构
 * @return XY_PID_OK 成功
 */
int xy_pid_get_state(xy_pid_t *pid, xy_pid_output_t *output);

/* ==================== Helper Macros ==================== */

#if !PID_USE_FLOAT
/* 定点数转换宏 */
#define PID_FLOAT_TO_FIXED(f)   ((pid_fixed_t)((f) * PID_FIXED_ONE))
#define PID_FIXED_TO_FLOAT(f)   ((float)(f) / PID_FIXED_ONE)
#else
#define PID_FLOAT_TO_FIXED(f)   ((pid_fixed_t)(f))
#define PID_FIXED_TO_FLOAT(f)   ((pid_fixed_t)(f))
#endif

#ifdef __cplusplus
}
#endif

#endif /* XY_PID_H */
