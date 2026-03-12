/**
 * @file osal_pc.h
 * @brief Minimal OSAL Header for PC Demo
 * @version 1.0.0
 * @date 2026-03-13
 */

#ifndef OSAL_PC_H
#define OSAL_PC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* OS 对象类型 */
typedef struct {
    void *handle;
} xy_os_thread_t;

typedef struct {
    void *handle;
} xy_os_mutex_t;

typedef struct {
    void *handle;
} xy_os_sem_t;

typedef struct {
    void *handle;
} xy_os_msgqueue_t;

/* 任务函数类型 */
typedef void (*xy_os_thread_func_t)(void *arg);

/* 核心 API */
uint32_t xy_os_tick_get(void);
void xy_os_delay(uint32_t ms);
int xy_os_kernel_init(void);
int xy_os_kernel_start(void);

/* 任务 API */
int xy_os_thread_create(xy_os_thread_t *thread, const char *name,
                        xy_os_thread_func_t func, void *arg,
                        uint8_t priority, uint16_t stack_size);
int xy_os_thread_delete(xy_os_thread_t *thread);

/* 同步 API */
int xy_os_mutex_create(xy_os_mutex_t *mutex);
int xy_os_mutex_lock(xy_os_mutex_t *mutex, uint32_t timeout);
int xy_os_mutex_unlock(xy_os_mutex_t *mutex);

int xy_os_sem_create(xy_os_sem_t *sem, uint16_t initial, uint16_t max);
int xy_os_sem_take(xy_os_sem_t *sem, uint32_t timeout);
int xy_os_sem_put(xy_os_sem_t *sem);

/* 消息队列 API */
int xy_os_msgqueue_create(xy_os_msgqueue_t *mq, uint16_t count, uint16_t size);
int xy_os_msgqueue_put(xy_os_msgqueue_t *mq, const void *msg, uint32_t timeout);
int xy_os_msgqueue_get(xy_os_msgqueue_t *mq, void *msg, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_PC_H */
