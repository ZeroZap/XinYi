/**
 * @file demo_osal.c
 * @brief OSAL Component Demo
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include "xy_os.h"
#include "xy_log.h"

#ifdef DEMO_OSAL

/* 演示任务函数 */
static void worker_task_1(void *arg)
{
    (void)arg;
    int count = 0;
    
    while (count < 5) {
        xy_log_d("Worker 1: iteration %d\n", count++);
        xy_os_delay(100);
    }
}

static void worker_task_2(void *arg)
{
    (void)arg;
    int count = 0;
    
    while (count < 5) {
        xy_log_d("Worker 2: iteration %d\n", count++);
        xy_os_delay(150);
    }
}

/**
 * @brief 初始化 OSAL 演示
 */
int demo_osal_init(void)
{
    xy_log_i("OSAL backend initialized\n");
    return 0;
}

/**
 * @brief 运行 OSAL 演示
 */
void demo_osal_run(void)
{
    xy_os_thread_t thread1, thread2;
    xy_os_mutex_t mutex;
    xy_os_sem_t sem;
    
    /* 演示任务创建 */
    xy_log_i("Creating tasks...\n");
    
    xy_os_thread_create(&thread1, "worker_1", worker_task_1, NULL, 3, 512);
    xy_log_d("Task 'worker_1' created (priority=3)\n");
    
    xy_os_thread_create(&thread2, "worker_2", worker_task_2, NULL, 3, 512);
    xy_log_d("Task 'worker_2' created (priority=3)\n");
    
    /* 演示同步原语 */
    xy_log_i("Creating synchronization primitives...\n");
    
    xy_os_mutex_create(&mutex);
    xy_log_d("Mutex created\n");
    
    xy_os_sem_create(&sem, 0, 10);
    xy_log_d("Semaphore created (initial=0, max=10)\n");
    
    /* 演示信号量操作 */
    xy_log_i("Testing semaphore...\n");
    xy_os_sem_put(&sem);
    xy_log_d("Semaphore put\n");
    
    int ret = xy_os_sem_take(&sem, 100);
    if (ret == 0) {
        xy_log_d("Semaphore taken successfully\n");
    }
    
    /* 演示互斥量操作 */
    xy_log_i("Testing mutex...\n");
    xy_os_mutex_lock(&mutex, 100);
    xy_log_d("Mutex locked\n");
    
    xy_os_mutex_unlock(&mutex);
    xy_log_d("Mutex unlocked\n");
    
    /* 演示系统信息 */
    xy_log_i("System tick: %lu\n", xy_os_tick_get());
    
    xy_log_i("Tasks running...\n");
    
    /* 短暂运行任务 */
    xy_os_delay(1000);
    
    xy_log_i("OSAL demo completed\n");
}

#endif /* DEMO_OSAL */
