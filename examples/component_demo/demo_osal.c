/**
 * @file demo_osal.c
 * @brief OSAL Component Demo (Standalone)
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "osal_pc.h"

#ifdef DEMO_OSAL

static int g_worker1_count = 0;
static int g_worker2_count = 0;

static void worker_task_1(void *arg)
{
    (void)arg;
    while (g_worker1_count < 3) {
        printf("  [Worker 1] iteration %d\n", g_worker1_count++);
        xy_os_delay(50);
    }
}

static void worker_task_2(void *arg)
{
    (void)arg;
    while (g_worker2_count < 3) {
        printf("  [Worker 2] iteration %d\n", g_worker2_count++);
        xy_os_delay(75);
    }
}

int demo_osal_init(void)
{
    printf("  OSAL backend: PC Simulator\n");
    return 0;
}

void demo_osal_run(void)
{
    xy_os_thread_t thread1, thread2;
    xy_os_mutex_t mutex;
    xy_os_sem_t sem;
    
    /* 演示任务创建 */
    printf("  Creating tasks...\n");
    
    xy_os_thread_create(&thread1, "worker_1", worker_task_1, NULL, 3, 512);
    printf("  Task 'worker_1' created\n");
    
    xy_os_thread_create(&thread2, "worker_2", worker_task_2, NULL, 3, 512);
    printf("  Task 'worker_2' created\n");
    
    /* 演示同步原语 */
    printf("  Creating sync primitives...\n");
    
    xy_os_mutex_create(&mutex);
    printf("  Mutex created\n");
    
    xy_os_sem_create(&sem, 0, 10);
    printf("  Semaphore created\n");
    
    /* 演示信号量操作 */
    printf("  Testing semaphore...\n");
    xy_os_sem_put(&sem);
    printf("  Semaphore put\n");
    
    int ret = xy_os_sem_take(&sem, 100);
    if (ret == 0) {
        printf("  Semaphore taken successfully\n");
    }
    
    /* 演示互斥量操作 */
    printf("  Testing mutex...\n");
    xy_os_mutex_lock(&mutex, 100);
    printf("  Mutex locked\n");
    
    xy_os_mutex_unlock(&mutex);
    printf("  Mutex unlocked\n");
    
    /* 系统信息 */
    printf("  System tick: %lu\n", xy_os_tick_get());
    
    printf("  OSAL demo completed\n");
}

#endif /* DEMO_OSAL */
