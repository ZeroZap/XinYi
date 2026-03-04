/**
 * @file xy_sysmon.c
 * @brief System Monitor Implementation
 * @version 1.0.0
 * @date 2026-03-01 上午
 */

#include "xy_sysmon.h"
#include "xy_log.h"
#include "xy_os.h"
#include <string.h>
#include <stdio.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

static bool g_initialized = false;
static uint32_t g_start_tick = 0;

/* 内存统计 */
extern uint8_t *_heap_start;
extern uint8_t *_heap_end;

int xy_sysmon_init(void)
{
    if (g_initialized) {
        return XY_SYSMON_OK;
    }
    
    g_start_tick = xy_os_tick_get();
    
    xy_log_i("System Monitor initialized\n");
    g_initialized = true;
    
    return XY_SYSMON_OK;
}

int xy_sysmon_get_stats(xy_sys_stats_t *stats)
{
    if (!stats) {
        return XY_SYSMON_INVALID_PARAM;
    }
    
    memset(stats, 0, sizeof(*stats));
    
    /* CPU 使用率 (简化实现) */
    stats->cpu_usage = 0.0F;
    
    /* 内存统计 */
    stats->heap_total = (uint32_t)(_heap_end - _heap_start);
    
    /* 简化的堆使用统计 */
#ifdef CONFIG_HEAP_STATS
    extern uint32_t xy_heap_get_used(void);
    stats->heap_used = xy_heap_get_used();
#else
    stats->heap_used = 0;
#endif
    stats->heap_max_used = stats->heap_used;
    stats->heap_usage = stats->heap_total > 0 ? 
                        stats->heap_used * 100.0F / stats->heap_total : 0.0F;
    
    /* 栈统计 (当前任务) */
    stats->stack_total = 0;  /* TODO: 获取栈大小 */
    stats->stack_used = 0;
    stats->stack_peak = 0;
    stats->stack_usage = 0.0F;
    
    /* 任务统计 */
    stats->task_count = 0;  /* TODO: 获取任务数 */
    stats->task_max = 0;
    
    /* 系统信息 */
    stats->uptime = xy_os_tick_get() - g_start_tick;
    stats->tick_rate = XY_OS_TICK_RATE;
    
    return XY_SYSMON_OK;
}

float xy_sysmon_get_cpu_usage(void)
{
    /* 简化实现：返回 0 表示空闲 */
    /* 实际应通过空闲任务统计实现 */
    return 0.0F;
}

float xy_sysmon_get_heap_usage(void)
{
    xy_sys_stats_t stats;
    xy_sysmon_get_stats(&stats);
    return stats.heap_usage;
}

float xy_sysmon_get_stack_usage(void)
{
    xy_sys_stats_t stats;
    xy_sysmon_get_stats(&stats);
    return stats.stack_usage;
}

uint32_t xy_sysmon_get_uptime(void)
{
    return xy_os_tick_get() - g_start_tick;
}

uint32_t xy_sysmon_get_task_count(void)
{
    xy_sys_stats_t stats;
    xy_sysmon_get_stats(&stats);
    return stats.task_count;
}

int xy_sysmon_register_alarm(const char *name, float threshold, xy_sysmon_alarm_cb callback)
{
    /* TODO: 实现告警注册 */
    (void)name;
    (void)threshold;
    (void)callback;
    return XY_SYSMON_OK;
}

void xy_sysmon_print_status(void)
{
    xy_sys_stats_t stats;
    xy_sysmon_get_stats(&stats);
    
    xy_log_i("=== System Status ===\n");
    xy_log_i("CPU Usage: %.1f%%\n", stats.cpu_usage);
    xy_log_i("Heap: %lu/%lu bytes (%.1f%%)\n", 
             stats.heap_used, stats.heap_total, stats.heap_usage);
    xy_log_i("Stack: %lu/%lu bytes (%.1f%%)\n",
             stats.stack_used, stats.stack_total, stats.stack_usage);
    xy_log_i("Tasks: %lu\n", stats.task_count);
    xy_log_i("Uptime: %lu ms\n", stats.uptime);
}

void xy_sysmon_print_tasks(void)
{
    xy_log_i("=== Task List ===\n");
    /* TODO: 实现任务列表打印 */
}
