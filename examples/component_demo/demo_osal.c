/**
 * @file demo_osal.c
 * @brief OSAL Component Demo
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include "xy_os.h"

#ifdef DEMO_OSAL

int demo_osal_init(void)
{
    printf("[INFO] OSAL backend initialized\n");
    return 0;
}

void demo_osal_run(void)
{
    xy_os_thread_id_t thread1, thread2;
    xy_os_mutex_id_t mutex;
    xy_os_semaphore_id_t sem;
    
    printf("[INFO] Creating tasks...\n");
    xy_os_thread_create(&thread1, "worker_1", NULL, NULL, 3, 512);
    printf("[DEBUG] Task 'worker_1' created\n");
    
    xy_os_thread_create(&thread2, "worker_2", NULL, NULL, 3, 512);
    printf("[DEBUG] Task 'worker_2' created\n");
    
    printf("[INFO] Creating synchronization primitives...\n");
    xy_os_mutex_create(&mutex);
    printf("[DEBUG] Mutex created\n");
    
    xy_os_semaphore_create(&sem, 0, 10);
    printf("[DEBUG] Semaphore created\n");
    
    printf("[INFO] Testing semaphore...\n");
    xy_os_semaphore_put(sem);
    xy_os_semaphore_take(sem, 100);
    printf("[DEBUG] Semaphore put/take OK\n");
    
    printf("[INFO] Testing mutex...\n");
    xy_os_mutex_lock(mutex, 100);
    xy_os_mutex_unlock(mutex);
    printf("[DEBUG] Mutex lock/unlock OK\n");
    
    printf("[INFO] System tick: %lu\n", xy_os_tick_get());
    xy_os_delay(500);
    
    printf("[INFO] OSAL demo completed\n");
}

#endif /* DEMO_OSAL */
