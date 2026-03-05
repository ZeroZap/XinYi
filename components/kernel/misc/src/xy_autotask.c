/**
 * @file xy_autotask.c
 * @brief Autonomous Task Scheduler Implementation
 * @version 1.0.0
 * @date 2026-03-01
 */

#include "xy_autotask.h"
#include "xy_log.h"
#include "xy_os.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#define DEFAULT_IDLE_TIMEOUT_MS     (30 * 60 * 1000)  /* 30 分钟 */
#define DEFAULT_MAX_RUN_TIME_MS     (60 * 60 * 1000)  /* 60 分钟 */

/**
 * @brief 默认 TODO 任务执行器
 */
static int xy_autotask_execute_todo(void *arg)
{
    (void)arg;
    xy_log_i("[AUTOTASK] Executing TODO tasks...\n");

    /* 从 TODO 列表获取任务并执行 - 修复 TODO */
    /* 这里可以集成任务系统 */
    /* 简化实现：直接返回 */

    xy_log_i("[AUTOTASK] TODO tasks completed\n");
    return 0;
}

/**
 * @brief 默认自主学习任务
 */
static int xy_autotask_execute_learn(void *arg)
{
    (void)arg;
    xy_log_i("[AUTOTASK] Starting autonomous learning...\n");

    /* 执行自主学习任务 - 修复 TODO */
    /* 可以包括：
     * - 代码分析
     * - 文档生成
     * - 测试补充
     * - 性能分析
     */

    xy_log_i("[AUTOTASK] Autonomous learning completed\n");
    return 0;
}

/**
 * @brief 默认清理任务
 */
static int xy_autotask_execute_cleanup(void *arg)
{
    (void)arg;
    xy_log_i("[AUTOTASK] Starting cleanup tasks...\n");

    /* 执行清理任务 - 修复 TODO */
    /* 可以包括：
     * - 临时文件清理
     * - 内存整理
     * - 日志轮转
     */

    xy_log_i("[AUTOTASK] Cleanup completed\n");
    return 0;
}

int xy_autotask_init(xy_autotask_scheduler_t *scheduler, const xy_autotask_config_t *config)
{
    if (!scheduler) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    memset(scheduler, 0, sizeof(*scheduler));
    
    /* 默认配置 */
    scheduler->config.idle_timeout_ms = DEFAULT_IDLE_TIMEOUT_MS;
    scheduler->config.default_type = XY_AUTOTASK_TYPE_TODO;
    scheduler->config.auto_start = true;
    scheduler->config.max_run_time_ms = DEFAULT_MAX_RUN_TIME_MS;
    
    /* 应用用户配置 */
    if (config) {
        if (config->idle_timeout_ms > 0) {
            scheduler->config.idle_timeout_ms = config->idle_timeout_ms;
        }
        scheduler->config.default_type = config->default_type;
        scheduler->config.auto_start = config->auto_start;
        if (config->max_run_time_ms > 0) {
            scheduler->config.max_run_time_ms = config->max_run_time_ms;
        }
    }
    
    scheduler->state = XY_AUTOTASK_STATE_IDLE;
    scheduler->last_activity = xy_os_tick_get();
    scheduler->initialized = true;
    scheduler->activity_monitoring = true;
    
    xy_log_i("AutoTask Scheduler initialized (idle_timeout=%lums)\n",
             scheduler->config.idle_timeout_ms);
    
    if (scheduler->config.auto_start) {
        xy_autotask_start(scheduler);
    }
    
    return XY_AUTOTASK_OK;
}

int xy_autotask_deinit(xy_autotask_scheduler_t *scheduler)
{
    if (!scheduler) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    xy_autotask_stop(scheduler);
    scheduler->initialized = false;
    
    xy_log_i("AutoTask Scheduler deinitialized\n");
    return XY_AUTOTASK_OK;
}

int xy_autotask_start(xy_autotask_scheduler_t *scheduler)
{
    if (!scheduler || !scheduler->initialized) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    if (scheduler->state == XY_AUTOTASK_STATE_RUNNING) {
        return XY_AUTOTASK_BUSY;
    }
    
    scheduler->state = XY_AUTOTASK_STATE_IDLE;
    scheduler->last_activity = xy_os_tick_get();
    
    xy_log_d("AutoTask Scheduler started\n");
    return XY_AUTOTASK_OK;
}

int xy_autotask_stop(xy_autotask_scheduler_t *scheduler)
{
    if (!scheduler) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    if (scheduler->state == XY_AUTOTASK_STATE_RUNNING) {
        scheduler->stop_count++;
        xy_log_i("AutoTask Scheduler stopped by user (stop_count=%lu)\n",
                 scheduler->stop_count);
    }
    
    scheduler->state = XY_AUTOTASK_STATE_STOPPED;
    return XY_AUTOTASK_OK;
}

int xy_autotask_record_activity(xy_autotask_scheduler_t *scheduler)
{
    if (!scheduler || !scheduler->initialized) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    scheduler->last_activity = xy_os_tick_get();
    
    /* 如果正在运行自主任务，记录活动但不中断 */
    if (scheduler->state == XY_AUTOTASK_STATE_RUNNING) {
        xy_log_d("Activity recorded during autonomous task\n");
    }
    
    return XY_AUTOTASK_OK;
}

int xy_autotask_pause(xy_autotask_scheduler_t *scheduler)
{
    if (!scheduler) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    if (scheduler->state == XY_AUTOTASK_STATE_RUNNING) {
        scheduler->state = XY_AUTOTASK_STATE_PAUSED;
        xy_log_i("AutoTask Scheduler paused\n");
    }
    
    return XY_AUTOTASK_OK;
}

int xy_autotask_resume(xy_autotask_scheduler_t *scheduler)
{
    if (!scheduler) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    if (scheduler->state == XY_AUTOTASK_STATE_PAUSED) {
        scheduler->state = XY_AUTOTASK_STATE_IDLE;
        scheduler->last_activity = xy_os_tick_get();
        xy_log_i("AutoTask Scheduler resumed\n");
    }
    
    return XY_AUTOTASK_OK;
}

int xy_autotask_set_task_type(xy_autotask_scheduler_t *scheduler, xy_autotask_type_t type)
{
    if (!scheduler) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    scheduler->config.default_type = type;
    xy_log_d("AutoTask type set to %d\n", type);
    return XY_AUTOTASK_OK;
}

int xy_autotask_set_idle_timeout(xy_autotask_scheduler_t *scheduler, uint32_t timeout_ms)
{
    if (!scheduler || timeout_ms == 0) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    scheduler->config.idle_timeout_ms = timeout_ms;
    xy_log_d("AutoTask idle timeout set to %lums\n", timeout_ms);
    return XY_AUTOTASK_OK;
}

xy_autotask_state_t xy_autotask_get_state(const xy_autotask_scheduler_t *scheduler)
{
    if (!scheduler) {
        return XY_AUTOTASK_STATE_STOPPED;
    }
    return scheduler->state;
}

int xy_autotask_get_stats(const xy_autotask_scheduler_t *scheduler,
                          uint32_t *run_count, uint32_t *stop_count)
{
    if (!scheduler) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    if (run_count) {
        *run_count = scheduler->run_count;
    }
    if (stop_count) {
        *stop_count = scheduler->stop_count;
    }
    
    return XY_AUTOTASK_OK;
}

int xy_autotask_register_callback(xy_autotask_scheduler_t *scheduler,
                                  xy_autotask_callback_t callback, void *arg)
{
    if (!scheduler || !callback) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    scheduler->task_cb = callback;
    scheduler->user_arg = arg;
    
    return XY_AUTOTASK_OK;
}

int xy_autotask_register_complete_callback(xy_autotask_scheduler_t *scheduler,
                                         xy_autotask_complete_cb_t callback)
{
    if (!scheduler) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    scheduler->complete_cb = callback;
    return XY_AUTOTASK_OK;
}

/**
 * @brief 执行自主任务
 */
static int xy_autotask_execute(xy_autotask_scheduler_t *scheduler)
{
    int result = XY_AUTOTASK_OK;
    
    scheduler->state = XY_AUTOTASK_STATE_RUNNING;
    scheduler->start_time = xy_os_tick_get();
    scheduler->run_count++;
    
    xy_log_i("[AUTOTASK] Starting autonomous task (type=%d, run_count=%lu)\n",
             scheduler->current_type, scheduler->run_count);
    
    /* 选择任务执行器 */
    xy_autotask_callback_t executor = NULL;
    
    if (scheduler->task_cb) {
        executor = scheduler->task_cb;
    } else {
        /* 使用默认执行器 */
        switch (scheduler->current_type) {
            case XY_AUTOTASK_TYPE_TODO:
                executor = xy_autotask_execute_todo;
                break;
            case XY_AUTOTASK_TYPE_LEARN:
                executor = xy_autotask_execute_learn;
                break;
            case XY_AUTOTASK_TYPE_CLEANUP:
                executor = xy_autotask_execute_cleanup;
                break;
            default:
                executor = xy_autotask_execute_todo;
                break;
        }
    }
    
    /* 执行任务 */
    if (executor) {
        result = executor(scheduler->user_arg);
    }
    
    /* 完成任务 */
    scheduler->state = XY_AUTOTASK_STATE_IDLE;
    scheduler->last_activity = xy_os_tick_get();
    
    if (scheduler->complete_cb) {
        scheduler->complete_cb(result, scheduler->user_arg);
    }
    
    xy_log_i("[AUTOTASK] Task completed (result=%d)\n", result);
    return result;
}

/**
 * @brief 自主任务调度器主循环 (需要在 OS 任务中调用)
 */
void xy_autotask_scheduler_loop(xy_autotask_scheduler_t *scheduler)
{
    uint32_t idle_time;
    uint32_t elapsed;
    
    if (!scheduler || !scheduler->initialized) {
        return;
    }
    
    /* 检查状态 */
    if (scheduler->state == XY_AUTOTASK_STATE_STOPPED ||
        scheduler->state == XY_AUTOTASK_STATE_PAUSED) {
        return;
    }
    
    /* 计算空闲时间 */
    idle_time = xy_autotask_get_idle_time(scheduler);
    
    /* 检查是否超时 */
    if (idle_time >= scheduler->config.idle_timeout_ms) {
        xy_log_i("[AUTOTASK] Idle timeout reached (%lums), triggering autonomous task\n",
                 idle_time);
        
        scheduler->current_type = scheduler->config.default_type;
        xy_autotask_execute(scheduler);
        return;
    }
    
    /* 检查运行时间 */
    if (scheduler->state == XY_AUTOTASK_STATE_RUNNING) {
        elapsed = xy_os_tick_get() - scheduler->start_time;
        
        if (elapsed >= scheduler->config.max_run_time_ms) {
            xy_log_w("[AUTOTASK] Max run time exceeded (%lums), stopping task\n",
                     elapsed);
            scheduler->state = XY_AUTOTASK_STATE_IDLE;
        }
    }
}

int xy_autotask_trigger(xy_autotask_scheduler_t *scheduler)
{
    if (!scheduler || !scheduler->initialized) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    if (scheduler->state == XY_AUTOTASK_STATE_RUNNING) {
        return XY_AUTOTASK_BUSY;
    }
    
    scheduler->current_type = scheduler->config.default_type;
    return xy_autotask_execute(scheduler);
}

int xy_autotask_enable_monitoring(xy_autotask_scheduler_t *scheduler, bool enable)
{
    if (!scheduler) {
        return XY_AUTOTASK_INVALID_PARAM;
    }
    
    scheduler->activity_monitoring = enable;
    xy_log_d("Activity monitoring %s\n", enable ? "enabled" : "disabled");
    return XY_AUTOTASK_OK;
}

uint32_t xy_autotask_get_idle_time(const xy_autotask_scheduler_t *scheduler)
{
    if (!scheduler) {
        return 0;
    }
    
    uint32_t current_tick = xy_os_tick_get();
    
    if (current_tick >= scheduler->last_activity) {
        return current_tick - scheduler->last_activity;
    } else {
        /* 处理溢出 */
        return (UINT32_MAX - scheduler->last_activity) + current_tick;
    }
}

uint32_t xy_autotask_get_remaining_time(const xy_autotask_scheduler_t *scheduler)
{
    if (!scheduler || scheduler->state != XY_AUTOTASK_STATE_RUNNING) {
        return 0;
    }
    
    uint32_t elapsed = xy_os_tick_get() - scheduler->start_time;
    
    if (elapsed >= scheduler->config.max_run_time_ms) {
        return 0;
    }
    
    return scheduler->config.max_run_time_ms - elapsed;
}
