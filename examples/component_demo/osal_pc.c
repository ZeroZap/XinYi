/**
 * @file osal_pc.c
 * @brief Minimal OSAL Implementation for PC Demo
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "osal_pc.h"

/* PC 模拟实现 - 简化版 OSAL */

static uint32_t g_tick = 0;

/**
 * @brief 获取系统 tick (毫秒)
 */
uint32_t xy_os_tick_get(void)
{
    static uint32_t start = 0;
    if (start == 0) {
        start = (uint32_t)time(NULL) * 1000;
    }
    return ((uint32_t)time(NULL) * 1000) - start + g_tick;
}

/**
 * @brief 延时 (毫秒)
 */
void xy_os_delay(uint32_t ms)
{
    #ifdef _WIN32
    Sleep(ms);
    #else
    usleep(ms * 1000);
    #endif
    g_tick += ms;
}

/**
 * @brief 内核初始化
 */
int xy_os_kernel_init(void)
{
    return 0;
}

/**
 * @brief 启动调度器
 */
int xy_os_kernel_start(void)
{
    return 0;
}

/**
 * @brief 创建任务
 */
int xy_os_thread_create(xy_os_thread_t *thread, const char *name,
                        xy_os_thread_func_t func, void *arg,
                        uint8_t priority, uint16_t stack_size)
{
    (void)name;
    (void)priority;
    (void)stack_size;
    
    if (!thread || !func) {
        return -1;
    }
    
    /* PC 演示：直接调用函数 */
    func(arg);
    thread->handle = (void *)1;
    return 0;
}

/**
 * @brief 删除任务
 */
int xy_os_thread_delete(xy_os_thread_t *thread)
{
    if (!thread) return -1;
    thread->handle = NULL;
    return 0;
}

/**
 * @brief 创建互斥量
 */
int xy_os_mutex_create(xy_os_mutex_t *mutex)
{
    if (!mutex) return -1;
    mutex->handle = (void *)1;
    return 0;
}

/**
 * @brief 获取互斥量
 */
int xy_os_mutex_lock(xy_os_mutex_t *mutex, uint32_t timeout)
{
    (void)timeout;
    if (!mutex) return -1;
    return 0;
}

/**
 * @brief 释放互斥量
 */
int xy_os_mutex_unlock(xy_os_mutex_t *mutex)
{
    if (!mutex) return -1;
    return 0;
}

/**
 * @brief 创建信号量
 */
int xy_os_sem_create(xy_os_sem_t *sem, uint16_t initial, uint16_t max)
{
    (void)initial;
    (void)max;
    if (!sem) return -1;
    sem->handle = (void *)1;
    return 0;
}

/**
 * @brief 获取信号量
 */
int xy_os_sem_take(xy_os_sem_t *sem, uint32_t timeout)
{
    (void)timeout;
    if (!sem) return -1;
    return 0;
}

/**
 * @brief 释放信号量
 */
int xy_os_sem_put(xy_os_sem_t *sem)
{
    if (!sem) return -1;
    return 0;
}

/**
 * @brief 创建消息队列
 */
int xy_os_msgqueue_create(xy_os_msgqueue_t *mq, uint16_t count, uint16_t size)
{
    (void)count;
    (void)size;
    if (!mq) return -1;
    mq->handle = (void *)1;
    return 0;
}

/**
 * @brief 发送消息
 */
int xy_os_msgqueue_put(xy_os_msgqueue_t *mq, const void *msg, uint32_t timeout)
{
    (void)msg;
    (void)timeout;
    if (!mq) return -1;
    return 0;
}

/**
 * @brief 接收消息
 */
int xy_os_msgqueue_get(xy_os_msgqueue_t *mq, void *msg, uint32_t timeout)
{
    (void)timeout;
    if (!mq) return -1;
    return 0;
}
