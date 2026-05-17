/**
 * @file bm_objects.h
 * @brief Baremetal OSAL internal object types — not part of the public API.
 */

#ifndef BM_OBJECTS_H
#define BM_OBJECTS_H

#include <stdint.h>
#include <string.h>

/* ==================== Pool size configuration ==================== */
/* All limits can be overridden via cmake target_compile_definitions   */

#ifndef XY_OS_BM_MAX_MUTEX
#define XY_OS_BM_MAX_MUTEX      8
#endif
#ifndef XY_OS_BM_MAX_SEM
#define XY_OS_BM_MAX_SEM        8
#endif
#ifndef XY_OS_BM_MAX_EVENT
#define XY_OS_BM_MAX_EVENT      8
#endif
#ifndef XY_OS_BM_MAX_QUEUE
#define XY_OS_BM_MAX_QUEUE      8
#endif
#ifndef XY_OS_BM_MAX_POOL
#define XY_OS_BM_MAX_POOL       4
#endif

/* Embedded data buffer sizes (bytes) per object instance */
#ifndef XY_OS_BM_MQ_BUF_SIZE
#define XY_OS_BM_MQ_BUF_SIZE    256
#endif
#ifndef XY_OS_BM_POOL_BUF_SIZE
#define XY_OS_BM_POOL_BUF_SIZE  128
#endif

#ifndef XY_OS_BM_NAME_LEN
#define XY_OS_BM_NAME_LEN       16
#endif

/* ==================== Magic numbers (validity guards) ==================== */

#define BM_MUTEX_MAGIC   0xBEEF1001u
#define BM_SEM_MAGIC     0xBEEF1002u
#define BM_EVENT_MAGIC   0xBEEF1003u
#define BM_QUEUE_MAGIC   0xBEEF1004u
#define BM_POOL_MAGIC    0xBEEF1005u

/* ==================== Internal object structures ==================== */

typedef struct {
    uint32_t         magic;
    char             name[XY_OS_BM_NAME_LEN];
    volatile uint32_t locked;      /* 0 = free, 1 = held */
    volatile uint32_t rec_count;   /* recursive lock depth */
    uint32_t         attr_bits;
} bm_mutex_t;

typedef struct {
    uint32_t         magic;
    char             name[XY_OS_BM_NAME_LEN];
    volatile uint32_t count;
    uint32_t         max_count;
} bm_sem_t;

typedef struct {
    uint32_t         magic;
    char             name[XY_OS_BM_NAME_LEN];
    volatile uint32_t flags;
} bm_event_t;

typedef struct {
    uint32_t         magic;
    char             name[XY_OS_BM_NAME_LEN];
    uint8_t         *data;                          /* points to embedded[] or external */
    uint8_t          embedded[XY_OS_BM_MQ_BUF_SIZE];
    uint32_t         msg_size;
    uint32_t         capacity;
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t count;
} bm_mq_t;

typedef struct {
    uint32_t         magic;
    char             name[XY_OS_BM_NAME_LEN];
    uint8_t         *mem;                           /* points to embedded[] or external */
    uint8_t          embedded[XY_OS_BM_POOL_BUF_SIZE];
    uint32_t         block_size;
    uint32_t         capacity;
    volatile uint32_t free_count;
    void            *free_list;                     /* singly-linked free block list */
} bm_pool_t;

#endif /* BM_OBJECTS_H */
