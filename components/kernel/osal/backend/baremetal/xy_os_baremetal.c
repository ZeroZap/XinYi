/**
 * @file xy_os_baremetal.c
 * @brief XY OSAL Bare-metal Implementation - Minimal RTOS stub
 * @version 1.0.0
 */

#include "../xy_os.h"
#include "../../misc/xy_tick.h"
#include "../../misc/xy_timer_sw.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief Platform-specific interrupt control
 * 
 * Supports:
 * - ARM Cortex-M (M0/M0+/M3/M4/M7/M33)
 * - RISC-V (with CSR access)
 * - x86/x64 (PC/Testing - no interrupt control)
 * - Generic fallback
 */

#if defined(XY_OS_DISABLE_ASM) || defined(__x86_64__) || defined(_M_X64) || \
    defined(__i386__) || defined(_M_IX86)
    /* 
     * PC/x86 - No interrupt control needed 
     * Used for testing and simulation
     */
    static __inline void __disable_irq_global(void) {}
    static __inline void __enable_irq_global(void) {}
    static __inline uint32_t __get_PRIMASK_global(void) { return 0; }
    
    #define XY_OS_BAREMETAL_PLATFORM "x86/x64 (No IRQ Control)"

#elif defined(__ARMCC_VERSION) || (defined(__GNUC__) && defined(__ARM_ARCH))
    /* 
     * ARM Cortex-M interrupt disable/enable
     * Uses PRIMASK register to disable all IRQ exceptions
     */
    #include <stdint.h>
    
    static __inline void __disable_irq_global(void) {
        __asm volatile ("cpsid i" : : : "memory");
    }
    
    static __inline void __enable_irq_global(void) {
        __asm volatile ("cpsie i" : : : "memory");
    }
    
    static __inline uint32_t __get_PRIMASK_global(void) {
        uint32_t result;
        __asm volatile ("MRS %0, primask" : "=r" (result) );
        return result;
    }
    
    #define XY_OS_BAREMETAL_PLATFORM "ARM Cortex-M (PRIMASK)"

#elif defined(__ICCARM__)
    /* 
     * IAR ARM interrupt control 
     * Uses intrinsic functions
     */
    #include <intrinsics.h>
    #define __disable_irq_global __disable_interrupt
    #define __enable_irq_global __enable_interrupt
    #define __get_PRIMASK_global __get_interrupt_state
    
    #define XY_OS_BAREMETAL_PLATFORM "ARM Cortex-M (IAR)"

#elif defined(__riscv) || defined(__riscv__)
    /* 
     * RISC-V interrupt control
     * Uses mstatus CSR to control MIE (Machine Interrupt Enable) bit
     * 
     * Note: Requires machine mode (M-mode) access
     * For user mode (U-mode), use supervisor calls or platform-specific API
     */
    #include <stdint.h>
    
    /* RISC-V mstatus register MIE bit position */
    #define XY_OS_RISCV_MSTATUS_MIE_BIT  (1 << 3)
    
    static __inline void __disable_irq_global(void) {
        #if defined(__GNUC__) || defined(__clang__)
            __asm volatile ("csrc mstatus, %0" :: "r"(XY_OS_RISCV_MSTATUS_MIE_BIT));
        #else
            /* Generic CSR write */
            uint32_t mstatus;
            __asm volatile ("csrr %0, mstatus" : "=r"(mstatus));
            mstatus &= ~XY_OS_RISCV_MSTATUS_MIE_BIT;
            __asm volatile ("csrw mstatus, %0" :: "r"(mstatus));
        #endif
    }
    
    static __inline void __enable_irq_global(void) {
        #if defined(__GNUC__) || defined(__clang__)
            __asm volatile ("csrs mstatus, %0" :: "r"(XY_OS_RISCV_MSTATUS_MIE_BIT));
        #else
            /* Generic CSR write */
            uint32_t mstatus;
            __asm volatile ("csrr %0, mstatus" : "=r"(mstatus));
            mstatus |= XY_OS_RISCV_MSTATUS_MIE_BIT;
            __asm volatile ("csrw mstatus, %0" :: "r"(mstatus));
        #endif
    }
    
    static __inline uint32_t __get_PRIMASK_global(void) {
        uint32_t mstatus;
        __asm volatile ("csrr %0, mstatus" : "=r"(mstatus));
        return (mstatus & XY_OS_RISCV_MSTATUS_MIE_BIT) ? 0 : 1;
    }
    
    #define XY_OS_BAREMETAL_PLATFORM "RISC-V (mstatus.MIE)"

#elif defined(__ARC__)
    /* 
     * Synopsys DesignWare ARC interrupt control
     */
    #include <arc/arc_exception.h>
    #define __disable_irq_global _interrupt_disable
    #define __enable_irq_global _interrupt_enable
    #define __get_PRIMASK_global _interrupt_status
    
    #define XY_OS_BAREMETAL_PLATFORM "ARC (DesignWare)"

#else
    /* 
     * Generic fallback - no interrupt control
     * Use with caution - not safe for ISR contexts
     */
    static __inline void __disable_irq_global(void) {}
    static __inline void __enable_irq_global(void) {}
    static __inline uint32_t __get_PRIMASK_global(void) { return 0; }
    
    #define XY_OS_BAREMETAL_PLATFORM "Generic (No IRQ Control)"
#endif

/* ==================== Kernel State ==================== */

static volatile uint32_t s_lock_count = 0;
static xy_os_kernel_state_t s_state   = XY_OS_KERNEL_INACTIVE;

/**
 * @brief Get bare-metal platform information
 * @return Platform description string
 */
const char *xy_os_baremetal_get_platform(void)
{
    #if defined(XY_OS_BAREMETAL_PLATFORM)
        return XY_OS_BAREMETAL_PLATFORM;
    #else
        return "Unknown";
    #endif
}

// Kernel functions
xy_os_status_t xy_os_kernel_init(void)
{
    s_state = XY_OS_KERNEL_READY;
    xy_timer_sw_init();
    return XY_OS_OK;
}

xy_os_status_t xy_os_kernel_get_info(xy_os_version_t *version, char *id_buf,
                                     uint32_t id_size)
{
    if (version) {
        version->api    = (XY_OSAL_VERSION_MAJOR << 16) | XY_OSAL_VERSION_MINOR;
        version->kernel = 0x00010000;
    }
    if (id_buf && id_size > 0) {
        /* Include platform information in kernel ID */
        int written = snprintf(id_buf, id_size, "Baremetal (%s)", 
                               xy_os_baremetal_get_platform());
        if (written < 0 || (uint32_t)written >= id_size) {
            /* Fallback if buffer too small */
            strncpy(id_buf, "Baremetal", id_size - 1);
            id_buf[id_size - 1] = '\0';
        }
    }
    return XY_OS_OK;
}

xy_os_kernel_state_t xy_os_kernel_get_state(void)
{
    return s_state;
}
xy_os_status_t xy_os_kernel_start(void)
{
    s_state = XY_OS_KERNEL_RUNNING;
    return XY_OS_OK;
}

/**
 * @brief Lock kernel (disable interrupts)
 * @return Previous lock count
 * 
 * Implementation:
 * - ARM Cortex-M: Uses PRIMASK to disable IRQ
 * - Other platforms: Counter-based (no hardware interrupt control)
 */
int32_t xy_os_kernel_lock(void)
{
    uint32_t prev_primask = __get_PRIMASK_global();
    
    /* Disable global interrupts on first lock */
    if (s_lock_count == 0) {
        __disable_irq_global();
        s_state = XY_OS_KERNEL_LOCKED;
    }
    
    s_lock_count++;
    
    /* Return previous interrupt state for restore */
    return (int32_t)prev_primask;
}

/**
 * @brief Unlock kernel (enable interrupts)
 * @return Current lock count
 */
int32_t xy_os_kernel_unlock(void)
{
    if (s_lock_count > 0) {
        s_lock_count--;
    }
    
    /* Enable interrupts when lock count reaches 0 */
    if (s_lock_count == 0) {
        s_state = XY_OS_KERNEL_RUNNING;
        __enable_irq_global();
    }
    
    return (int32_t)s_lock_count;
}

/**
 * @brief Restore previous lock state
 * @param lock Previous lock state (from xy_os_kernel_lock)
 * @return Previous lock count
 */
int32_t xy_os_kernel_restore_lock(int32_t lock)
{
    uint32_t prev = s_lock_count;
    
    /* Restore interrupt state based on lock parameter */
    if (lock == 0) {
        /* Previous state was unlocked */
        s_lock_count = 0;
        s_state = XY_OS_KERNEL_RUNNING;
        __enable_irq_global();
    } else {
        /* Previous state was locked */
        s_lock_count = 1;
        s_state = XY_OS_KERNEL_LOCKED;
        /* Interrupts already disabled */
    }
    
    return (int32_t)prev;
}

uint32_t xy_os_kernel_get_tick_count(void)
{
    return xy_tick_get();
}
uint32_t xy_os_kernel_get_tick_freq(void)
{
    return 1000;
}
uint32_t xy_os_kernel_get_sys_timer_count(void)
{
    return xy_tick_get();
}
uint32_t xy_os_kernel_get_sys_timer_freq(void)
{
    return 1000;
}

// Delay functions
xy_os_status_t xy_os_delay(uint32_t ticks)
{
    uint32_t start = xy_tick_get();
    while ((xy_tick_get() - start) < ticks)
        ;
    return XY_OS_OK;
}

xy_os_status_t xy_os_delay_until(uint32_t ticks)
{
    uint32_t now = xy_tick_get();
    return (ticks > now) ? xy_os_delay(ticks - now) : XY_OS_OK;
}

// Thread stubs
xy_os_thread_id_t xy_os_thread_new(xy_os_thread_func_t f, void *arg,
                                   const xy_os_thread_attr_t *attr)
{
    return NULL;
}
const char *xy_os_thread_get_name(xy_os_thread_id_t id)
{
    return "main";
}
xy_os_thread_id_t xy_os_thread_get_id(void)
{
    return (void *)0x1;
}
xy_os_thread_state_t xy_os_thread_get_state(xy_os_thread_id_t id)
{
    return XY_OS_THREAD_RUNNING;
}
uint32_t xy_os_thread_get_stack_size(xy_os_thread_id_t id)
{
    return 0;
}
uint32_t xy_os_thread_get_stack_space(xy_os_thread_id_t id)
{
    return 0;
}
xy_os_status_t xy_os_thread_set_priority(xy_os_thread_id_t id,
                                         xy_os_priority_t p)
{
    return XY_OS_ERROR;
}
xy_os_priority_t xy_os_thread_get_priority(xy_os_thread_id_t id)
{
    return XY_OS_PRIORITY_NORMAL;
}
xy_os_status_t xy_os_thread_yield(void)
{
    return XY_OS_OK;
}
xy_os_status_t xy_os_thread_suspend(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_thread_resume(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_thread_detach(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
xy_os_status_t xy_os_thread_join(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
void xy_os_thread_exit(void)
{
    while (1)
        ;
}
xy_os_status_t xy_os_thread_terminate(xy_os_thread_id_t id)
{
    return XY_OS_ERROR;
}
uint32_t xy_os_thread_get_count(void)
{
    return 1;
}
uint32_t xy_os_thread_enumerate(xy_os_thread_id_t *arr, uint32_t n)
{
    if (arr && n > 0) {
        arr[0] = xy_os_thread_get_id();
        return 1;
    }
    return 0;
}

// Thread flags stubs
uint32_t xy_os_thread_flags_set(xy_os_thread_id_t id, uint32_t f)
{
    return 0x80000000;
}
uint32_t xy_os_thread_flags_clear(uint32_t f)
{
    return 0;
}
uint32_t xy_os_thread_flags_get(void)
{
    return 0;
}
uint32_t xy_os_thread_flags_wait(uint32_t f, uint32_t opt, uint32_t to)
{
    return 0x80000000;
}

/* ==================== Baremetal internal helpers ==================== */

#include "bm_objects.h"

static inline uint32_t bm_enter_critical(void)
{
    uint32_t s = __get_PRIMASK_global();
    __disable_irq_global();
    return s;
}

static inline void bm_exit_critical(uint32_t saved)
{
    if (!(saved & 1u))
        __enable_irq_global();
}

/* ==================== Object pool helpers ==================== */

/* --- Mutex pool --- */
static bm_mutex_t s_mutex_pool[XY_OS_BM_MAX_MUTEX];
static uint8_t    s_mutex_used[XY_OS_BM_MAX_MUTEX];

static bm_mutex_t *bm_mutex_alloc(void)
{
    for (int i = 0; i < XY_OS_BM_MAX_MUTEX; i++) {
        if (!s_mutex_used[i]) { s_mutex_used[i] = 1; return &s_mutex_pool[i]; }
    }
    return NULL;
}
static void bm_mutex_pool_free(bm_mutex_t *m)
{
    for (int i = 0; i < XY_OS_BM_MAX_MUTEX; i++) {
        if (&s_mutex_pool[i] == m) { s_mutex_used[i] = 0; break; }
    }
}

/* --- Semaphore pool --- */
static bm_sem_t s_sem_pool[XY_OS_BM_MAX_SEM];
static uint8_t  s_sem_used[XY_OS_BM_MAX_SEM];

static bm_sem_t *bm_sem_alloc(void)
{
    for (int i = 0; i < XY_OS_BM_MAX_SEM; i++) {
        if (!s_sem_used[i]) { s_sem_used[i] = 1; return &s_sem_pool[i]; }
    }
    return NULL;
}
static void bm_sem_pool_free(bm_sem_t *s)
{
    for (int i = 0; i < XY_OS_BM_MAX_SEM; i++) {
        if (&s_sem_pool[i] == s) { s_sem_used[i] = 0; break; }
    }
}

/* --- Event pool --- */
static bm_event_t s_event_pool[XY_OS_BM_MAX_EVENT];
static uint8_t    s_event_used[XY_OS_BM_MAX_EVENT];

static bm_event_t *bm_event_alloc(void)
{
    for (int i = 0; i < XY_OS_BM_MAX_EVENT; i++) {
        if (!s_event_used[i]) { s_event_used[i] = 1; return &s_event_pool[i]; }
    }
    return NULL;
}
static void bm_event_pool_free(bm_event_t *e)
{
    for (int i = 0; i < XY_OS_BM_MAX_EVENT; i++) {
        if (&s_event_pool[i] == e) { s_event_used[i] = 0; break; }
    }
}

/* --- Message queue pool --- */
static bm_mq_t s_mq_pool[XY_OS_BM_MAX_QUEUE];
static uint8_t  s_mq_used[XY_OS_BM_MAX_QUEUE];

static bm_mq_t *bm_mq_alloc(void)
{
    for (int i = 0; i < XY_OS_BM_MAX_QUEUE; i++) {
        if (!s_mq_used[i]) { s_mq_used[i] = 1; return &s_mq_pool[i]; }
    }
    return NULL;
}
static void bm_mq_pool_free(bm_mq_t *q)
{
    for (int i = 0; i < XY_OS_BM_MAX_QUEUE; i++) {
        if (&s_mq_pool[i] == q) { s_mq_used[i] = 0; break; }
    }
}

/* --- Memory pool pool --- */
static bm_pool_t s_pool_pool[XY_OS_BM_MAX_POOL];
static uint8_t   s_pool_used[XY_OS_BM_MAX_POOL];

static bm_pool_t *bm_pool_alloc(void)
{
    for (int i = 0; i < XY_OS_BM_MAX_POOL; i++) {
        if (!s_pool_used[i]) { s_pool_used[i] = 1; return &s_pool_pool[i]; }
    }
    return NULL;
}
static void bm_pool_pool_free(bm_pool_t *p)
{
    for (int i = 0; i < XY_OS_BM_MAX_POOL; i++) {
        if (&s_pool_pool[i] == p) { s_pool_used[i] = 0; break; }
    }
}

/* ==================== Software Timer Implementation ==================== */
/* Uses xy_timer_sw module from components/kernel/misc                     */

/* Software Timer Implementation using xy_timer_sw module */
typedef struct {
    xy_timer_sw_id_t sw_timer_id;
    xy_os_timer_func_t callback;
    void *arg;
    xy_os_timer_type_t type;
    char name[16];
} baremetal_timer_ctx_t;

static baremetal_timer_ctx_t s_timers[8] = { 0 };

static void timer_sw_callback(void *arg)
{
    baremetal_timer_ctx_t *ctx = (baremetal_timer_ctx_t *)arg;
    if (ctx && ctx->callback) {
        ctx->callback(ctx->arg);
    }
}

xy_os_timer_id_t xy_os_timer_new(xy_os_timer_func_t f, xy_os_timer_type_t t,
                                 void *arg, const xy_os_timer_attr_t *attr)
{
    if (f == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < 8; i++) {
        if (s_timers[i].callback == NULL) {
            memset(&s_timers[i], 0, sizeof(s_timers[i]));
            s_timers[i].callback = f;
            s_timers[i].type = t;
            s_timers[i].arg = arg;
            if (attr && attr->name) {
                strncpy(s_timers[i].name, attr->name, sizeof(s_timers[i].name) - 1);
            }
            return (xy_os_timer_id_t)(uintptr_t)(i + 1);
        }
    }
    return NULL;
}

const char *xy_os_timer_get_name(xy_os_timer_id_t id)
{
    if (!id || (uintptr_t)id > 8) {
        return NULL;
    }
    return s_timers[(uintptr_t)id - 1].name[0] ? s_timers[(uintptr_t)id - 1].name : "timer";
}

xy_os_status_t xy_os_timer_start(xy_os_timer_id_t id, uint32_t ticks)
{
    if (!id || (uintptr_t)id > 8) {
        return XY_OS_ERROR;
    }
    
    baremetal_timer_ctx_t *ctx = &s_timers[(uintptr_t)id - 1];
    uint8_t periodic = (ctx->type == XY_OS_TIMER_PERIODIC) ? 1 : 0;
    
    ctx->sw_timer_id = xy_timer_sw_create(ticks, timer_sw_callback, ctx, periodic);
    if (ctx->sw_timer_id == XY_TIMER_SW_INVALID_ID) {
        return XY_OS_ERROR;
    }
    
    return XY_OS_OK;
}

xy_os_status_t xy_os_timer_stop(xy_os_timer_id_t id)
{
    if (!id || (uintptr_t)id > 8) {
        return XY_OS_ERROR;
    }
    
    baremetal_timer_ctx_t *ctx = &s_timers[(uintptr_t)id - 1];
    if (ctx->sw_timer_id) {
        xy_timer_sw_stop(ctx->sw_timer_id);
        ctx->sw_timer_id = 0;
    }
    return XY_OS_OK;
}

uint32_t xy_os_timer_is_running(xy_os_timer_id_t id)
{
    if (!id || (uintptr_t)id > 8) {
        return 0;
    }
    return s_timers[(uintptr_t)id - 1].sw_timer_id ? 1 : 0;
}

xy_os_status_t xy_os_timer_delete(xy_os_timer_id_t id)
{
    if (!id || (uintptr_t)id > 8) {
        return XY_OS_ERROR;
    }
    
    baremetal_timer_ctx_t *ctx = &s_timers[(uintptr_t)id - 1];
    if (ctx->sw_timer_id) {
        xy_timer_sw_delete(ctx->sw_timer_id);
        ctx->sw_timer_id = 0;
    }
    ctx->callback = NULL;
    ctx->arg = NULL;
    ctx->type = 0;
    memset(ctx->name, 0, sizeof(ctx->name));
    return XY_OS_OK;
}

/* ==================== Event Flags ==================== */

xy_os_event_flags_id_t
xy_os_event_flags_new(const xy_os_event_flags_attr_t *attr)
{
    bm_event_t *e = (attr && attr->cb_mem &&
                     attr->cb_size >= sizeof(bm_event_t))
                    ? (bm_event_t *)attr->cb_mem : bm_event_alloc();
    if (!e) return NULL;
    memset(e, 0, sizeof(*e));
    e->magic = BM_EVENT_MAGIC;
    if (attr && attr->name)
        strncpy(e->name, attr->name, XY_OS_BM_NAME_LEN - 1);
    return (xy_os_event_flags_id_t)e;
}

const char *xy_os_event_flags_get_name(xy_os_event_flags_id_t ef_id)
{
    bm_event_t *e = (bm_event_t *)ef_id;
    return (e && e->magic == BM_EVENT_MAGIC) ? e->name : NULL;
}

uint32_t xy_os_event_flags_set(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    bm_event_t *e = (bm_event_t *)ef_id;
    if (!e || e->magic != BM_EVENT_MAGIC) return 0x80000000u;
    uint32_t irq = bm_enter_critical();
    e->flags |= flags;
    uint32_t result = e->flags;
    bm_exit_critical(irq);
    return result;
}

uint32_t xy_os_event_flags_clear(xy_os_event_flags_id_t ef_id, uint32_t flags)
{
    bm_event_t *e = (bm_event_t *)ef_id;
    if (!e || e->magic != BM_EVENT_MAGIC) return 0x80000000u;
    uint32_t irq = bm_enter_critical();
    uint32_t prev = e->flags;
    e->flags &= ~flags;
    bm_exit_critical(irq);
    return prev;
}

uint32_t xy_os_event_flags_get(xy_os_event_flags_id_t ef_id)
{
    bm_event_t *e = (bm_event_t *)ef_id;
    if (!e || e->magic != BM_EVENT_MAGIC) return 0;
    uint32_t irq = bm_enter_critical();
    uint32_t f = e->flags;
    bm_exit_critical(irq);
    return f;
}

uint32_t xy_os_event_flags_wait(xy_os_event_flags_id_t ef_id, uint32_t flags,
                                uint32_t options, uint32_t timeout)
{
    bm_event_t *e = (bm_event_t *)ef_id;
    if (!e || e->magic != BM_EVENT_MAGIC) return 0x80000000u;

    uint32_t deadline = (timeout == XY_OS_WAIT_FOREVER)
                        ? 0xFFFFFFFFu
                        : xy_tick_get() + timeout;

    while (1) {
        uint32_t irq = bm_enter_critical();
        uint32_t cur = e->flags;
        int met = (options & XY_OS_FLAGS_WAIT_ALL)
                  ? ((cur & flags) == flags)
                  : ((cur & flags) != 0);
        if (met) {
            if (!(options & XY_OS_FLAGS_NO_CLEAR))
                e->flags &= ~flags;
            bm_exit_critical(irq);
            return cur;
        }
        bm_exit_critical(irq);
        if (timeout == 0) return 0x80000000u;
        if (timeout != XY_OS_WAIT_FOREVER && xy_tick_get() >= deadline)
            return 0x80000000u;
    }
}

xy_os_status_t xy_os_event_flags_delete(xy_os_event_flags_id_t ef_id)
{
    bm_event_t *e = (bm_event_t *)ef_id;
    if (!e || e->magic != BM_EVENT_MAGIC) return XY_OS_ERROR_PARAMETER;
    e->magic = 0;
    bm_event_pool_free(e);
    return XY_OS_OK;
}

/* ==================== Mutex ==================== */

xy_os_mutex_id_t xy_os_mutex_new(const xy_os_mutex_attr_t *attr)
{
    bm_mutex_t *m = (attr && attr->cb_mem &&
                     attr->cb_size >= sizeof(bm_mutex_t))
                    ? (bm_mutex_t *)attr->cb_mem : bm_mutex_alloc();
    if (!m) return NULL;
    memset(m, 0, sizeof(*m));
    m->magic = BM_MUTEX_MAGIC;
    if (attr) {
        if (attr->name)
            strncpy(m->name, attr->name, XY_OS_BM_NAME_LEN - 1);
        m->attr_bits = attr->attr_bits;
    }
    return (xy_os_mutex_id_t)m;
}

const char *xy_os_mutex_get_name(xy_os_mutex_id_t mutex_id)
{
    bm_mutex_t *m = (bm_mutex_t *)mutex_id;
    return (m && m->magic == BM_MUTEX_MAGIC) ? m->name : NULL;
}

xy_os_status_t xy_os_mutex_acquire(xy_os_mutex_id_t mutex_id, uint32_t timeout)
{
    bm_mutex_t *m = (bm_mutex_t *)mutex_id;
    if (!m || m->magic != BM_MUTEX_MAGIC) return XY_OS_ERROR_PARAMETER;

    uint32_t deadline = (timeout == XY_OS_WAIT_FOREVER)
                        ? 0xFFFFFFFFu
                        : xy_tick_get() + timeout;

    while (1) {
        uint32_t irq = bm_enter_critical();
        if (!m->locked) {
            m->locked    = 1;
            m->rec_count = 1;
            bm_exit_critical(irq);
            return XY_OS_OK;
        }
        /* Recursive re-entry */
        if (m->attr_bits & XY_OS_MUTEX_RECURSIVE) {
            m->rec_count++;
            bm_exit_critical(irq);
            return XY_OS_OK;
        }
        bm_exit_critical(irq);
        if (timeout == 0) return XY_OS_ERROR_TIMEOUT;
        if (timeout != XY_OS_WAIT_FOREVER && xy_tick_get() >= deadline)
            return XY_OS_ERROR_TIMEOUT;
    }
}

xy_os_status_t xy_os_mutex_release(xy_os_mutex_id_t mutex_id)
{
    bm_mutex_t *m = (bm_mutex_t *)mutex_id;
    if (!m || m->magic != BM_MUTEX_MAGIC) return XY_OS_ERROR_PARAMETER;
    uint32_t irq = bm_enter_critical();
    if (!m->locked) { bm_exit_critical(irq); return XY_OS_ERROR; }
    if (m->rec_count > 1) {
        m->rec_count--;
    } else {
        m->rec_count = 0;
        m->locked    = 0;
    }
    bm_exit_critical(irq);
    return XY_OS_OK;
}

xy_os_thread_id_t xy_os_mutex_get_owner(xy_os_mutex_id_t mutex_id)
{
    (void)mutex_id;
    return NULL;
}

xy_os_status_t xy_os_mutex_delete(xy_os_mutex_id_t mutex_id)
{
    bm_mutex_t *m = (bm_mutex_t *)mutex_id;
    if (!m || m->magic != BM_MUTEX_MAGIC) return XY_OS_ERROR_PARAMETER;
    m->magic = 0;
    bm_mutex_pool_free(m);
    return XY_OS_OK;
}

/* ==================== Semaphore ==================== */

xy_os_semaphore_id_t xy_os_semaphore_new(uint32_t max_count,
                                         uint32_t initial_count,
                                         const xy_os_semaphore_attr_t *attr)
{
    bm_sem_t *s = (attr && attr->cb_mem &&
                   attr->cb_size >= sizeof(bm_sem_t))
                  ? (bm_sem_t *)attr->cb_mem : bm_sem_alloc();
    if (!s) return NULL;
    memset(s, 0, sizeof(*s));
    s->magic     = BM_SEM_MAGIC;
    s->max_count = max_count ? max_count : 0xFFFFFFFFu;
    s->count     = (initial_count <= s->max_count) ? initial_count : s->max_count;
    if (attr && attr->name)
        strncpy(s->name, attr->name, XY_OS_BM_NAME_LEN - 1);
    return (xy_os_semaphore_id_t)s;
}

const char *xy_os_semaphore_get_name(xy_os_semaphore_id_t semaphore_id)
{
    bm_sem_t *s = (bm_sem_t *)semaphore_id;
    return (s && s->magic == BM_SEM_MAGIC) ? s->name : NULL;
}

xy_os_status_t xy_os_semaphore_acquire(xy_os_semaphore_id_t semaphore_id,
                                       uint32_t timeout)
{
    bm_sem_t *s = (bm_sem_t *)semaphore_id;
    if (!s || s->magic != BM_SEM_MAGIC) return XY_OS_ERROR_PARAMETER;

    uint32_t deadline = (timeout == XY_OS_WAIT_FOREVER)
                        ? 0xFFFFFFFFu
                        : xy_tick_get() + timeout;

    while (1) {
        uint32_t irq = bm_enter_critical();
        if (s->count > 0) {
            s->count--;
            bm_exit_critical(irq);
            return XY_OS_OK;
        }
        bm_exit_critical(irq);
        if (timeout == 0) return XY_OS_ERROR_TIMEOUT;
        if (timeout != XY_OS_WAIT_FOREVER && xy_tick_get() >= deadline)
            return XY_OS_ERROR_TIMEOUT;
    }
}

xy_os_status_t xy_os_semaphore_release(xy_os_semaphore_id_t semaphore_id)
{
    bm_sem_t *s = (bm_sem_t *)semaphore_id;
    if (!s || s->magic != BM_SEM_MAGIC) return XY_OS_ERROR_PARAMETER;
    uint32_t irq = bm_enter_critical();
    if (s->count >= s->max_count) {
        bm_exit_critical(irq);
        return XY_OS_ERROR_RESOURCE;
    }
    s->count++;
    bm_exit_critical(irq);
    return XY_OS_OK;
}

xy_os_status_t xy_os_semaphore_release_from_isr(xy_os_semaphore_id_t semaphore_id)
{
    return xy_os_semaphore_release(semaphore_id);
}

uint32_t xy_os_semaphore_get_count(xy_os_semaphore_id_t semaphore_id)
{
    bm_sem_t *s = (bm_sem_t *)semaphore_id;
    if (!s || s->magic != BM_SEM_MAGIC) return 0;
    uint32_t irq = bm_enter_critical();
    uint32_t c = s->count;
    bm_exit_critical(irq);
    return c;
}

xy_os_status_t xy_os_semaphore_delete(xy_os_semaphore_id_t semaphore_id)
{
    bm_sem_t *s = (bm_sem_t *)semaphore_id;
    if (!s || s->magic != BM_SEM_MAGIC) return XY_OS_ERROR_PARAMETER;
    s->magic = 0;
    bm_sem_pool_free(s);
    return XY_OS_OK;
}

/* ==================== Memory Pool ==================== */

xy_os_mempool_id_t xy_os_mempool_new(uint32_t block_count, uint32_t block_size,
                                     const xy_os_mempool_attr_t *attr)
{
    if (block_count == 0 || block_size == 0) return NULL;

    /* block must be large enough to hold a pointer for the free-list */
    uint32_t bsz = (block_size < sizeof(void *)) ? (uint32_t)sizeof(void *) : block_size;

    bm_pool_t *p = (attr && attr->cb_mem &&
                    attr->cb_size >= sizeof(bm_pool_t))
                   ? (bm_pool_t *)attr->cb_mem : bm_pool_alloc();
    if (!p) return NULL;
    memset(p, 0, sizeof(*p));
    p->magic      = BM_POOL_MAGIC;
    p->block_size = bsz;
    p->capacity   = block_count;

    /* Choose memory region */
    uint32_t needed = block_count * bsz;
    if (attr && attr->mp_mem && attr->mp_size >= needed) {
        p->mem = (uint8_t *)attr->mp_mem;
    } else if (needed <= XY_OS_BM_POOL_BUF_SIZE) {
        p->mem = p->embedded;
    } else {
        /* Not enough embedded space and no external memory provided */
        p->magic = 0;
        bm_pool_pool_free(p);
        return NULL;
    }

    if (attr && attr->name)
        strncpy(p->name, attr->name, XY_OS_BM_NAME_LEN - 1);

    /* Build free-list: each block's first word points to next free block */
    p->free_list  = NULL;
    p->free_count = 0;
    for (uint32_t i = block_count; i > 0; i--) {
        void *blk = p->mem + (i - 1) * bsz;
        *(void **)blk = p->free_list;
        p->free_list  = blk;
        p->free_count++;
    }

    return (xy_os_mempool_id_t)p;
}

const char *xy_os_mempool_get_name(xy_os_mempool_id_t mp_id)
{
    bm_pool_t *p = (bm_pool_t *)mp_id;
    return (p && p->magic == BM_POOL_MAGIC) ? p->name : NULL;
}

void *xy_os_mempool_alloc(xy_os_mempool_id_t mp_id, uint32_t timeout)
{
    bm_pool_t *p = (bm_pool_t *)mp_id;
    if (!p || p->magic != BM_POOL_MAGIC) return NULL;

    uint32_t deadline = (timeout == XY_OS_WAIT_FOREVER)
                        ? 0xFFFFFFFFu
                        : xy_tick_get() + timeout;

    while (1) {
        uint32_t irq = bm_enter_critical();
        if (p->free_list) {
            void *blk    = p->free_list;
            p->free_list = *(void **)blk;
            p->free_count--;
            bm_exit_critical(irq);
            return blk;
        }
        bm_exit_critical(irq);
        if (timeout == 0) return NULL;
        if (timeout != XY_OS_WAIT_FOREVER && xy_tick_get() >= deadline)
            return NULL;
    }
}

xy_os_status_t xy_os_mempool_free(xy_os_mempool_id_t mp_id, void *block)
{
    bm_pool_t *p = (bm_pool_t *)mp_id;
    if (!p || p->magic != BM_POOL_MAGIC || !block) return XY_OS_ERROR_PARAMETER;
    uint32_t irq  = bm_enter_critical();
    *(void **)block = p->free_list;
    p->free_list    = block;
    p->free_count++;
    bm_exit_critical(irq);
    return XY_OS_OK;
}

uint32_t xy_os_mempool_get_capacity(xy_os_mempool_id_t mp_id)
{
    bm_pool_t *p = (bm_pool_t *)mp_id;
    return (p && p->magic == BM_POOL_MAGIC) ? p->capacity : 0;
}

uint32_t xy_os_mempool_get_block_size(xy_os_mempool_id_t mp_id)
{
    bm_pool_t *p = (bm_pool_t *)mp_id;
    return (p && p->magic == BM_POOL_MAGIC) ? p->block_size : 0;
}

uint32_t xy_os_mempool_get_count(xy_os_mempool_id_t mp_id)
{
    bm_pool_t *p = (bm_pool_t *)mp_id;
    if (!p || p->magic != BM_POOL_MAGIC) return 0;
    uint32_t irq = bm_enter_critical();
    uint32_t used = p->capacity - p->free_count;
    bm_exit_critical(irq);
    return used;
}

uint32_t xy_os_mempool_get_space(xy_os_mempool_id_t mp_id)
{
    bm_pool_t *p = (bm_pool_t *)mp_id;
    if (!p || p->magic != BM_POOL_MAGIC) return 0;
    uint32_t irq = bm_enter_critical();
    uint32_t free = p->free_count;
    bm_exit_critical(irq);
    return free;
}

xy_os_status_t xy_os_mempool_delete(xy_os_mempool_id_t mp_id)
{
    bm_pool_t *p = (bm_pool_t *)mp_id;
    if (!p || p->magic != BM_POOL_MAGIC) return XY_OS_ERROR_PARAMETER;
    p->magic = 0;
    bm_pool_pool_free(p);
    return XY_OS_OK;
}

/* ==================== Message Queue ==================== */

xy_os_msgqueue_id_t xy_os_msgqueue_new(uint32_t msg_count, uint32_t msg_size,
                                       const xy_os_msgqueue_attr_t *attr)
{
    if (msg_count == 0 || msg_size == 0) return NULL;

    bm_mq_t *q = (attr && attr->cb_mem &&
                  attr->cb_size >= sizeof(bm_mq_t))
                 ? (bm_mq_t *)attr->cb_mem : bm_mq_alloc();
    if (!q) return NULL;
    memset(q, 0, sizeof(*q));
    q->magic    = BM_QUEUE_MAGIC;
    q->msg_size = msg_size;
    q->capacity = msg_count;

    uint32_t needed = msg_count * msg_size;
    if (attr && attr->mq_mem && attr->mq_size >= needed) {
        q->data = (uint8_t *)attr->mq_mem;
    } else if (needed <= XY_OS_BM_MQ_BUF_SIZE) {
        q->data = q->embedded;
    } else {
        q->magic = 0;
        bm_mq_pool_free(q);
        return NULL;
    }

    if (attr && attr->name)
        strncpy(q->name, attr->name, XY_OS_BM_NAME_LEN - 1);

    return (xy_os_msgqueue_id_t)q;
}

const char *xy_os_msgqueue_get_name(xy_os_msgqueue_id_t mq_id)
{
    bm_mq_t *q = (bm_mq_t *)mq_id;
    return (q && q->magic == BM_QUEUE_MAGIC) ? q->name : NULL;
}

xy_os_status_t xy_os_msgqueue_put(xy_os_msgqueue_id_t mq_id,
                                  const void *msg_ptr, uint8_t msg_prio,
                                  uint32_t timeout)
{
    (void)msg_prio;
    bm_mq_t *q = (bm_mq_t *)mq_id;
    if (!q || q->magic != BM_QUEUE_MAGIC || !msg_ptr) return XY_OS_ERROR_PARAMETER;

    uint32_t deadline = (timeout == XY_OS_WAIT_FOREVER)
                        ? 0xFFFFFFFFu
                        : xy_tick_get() + timeout;

    while (1) {
        uint32_t irq = bm_enter_critical();
        if (q->count < q->capacity) {
            memcpy(q->data + q->tail * q->msg_size, msg_ptr, q->msg_size);
            q->tail = (q->tail + 1) % q->capacity;
            q->count++;
            bm_exit_critical(irq);
            return XY_OS_OK;
        }
        bm_exit_critical(irq);
        if (timeout == 0) return XY_OS_ERROR_RESOURCE;
        if (timeout != XY_OS_WAIT_FOREVER && xy_tick_get() >= deadline)
            return XY_OS_ERROR_TIMEOUT;
    }
}

xy_os_status_t xy_os_msgqueue_get(xy_os_msgqueue_id_t mq_id, void *msg_ptr,
                                  uint8_t *msg_prio, uint32_t timeout)
{
    bm_mq_t *q = (bm_mq_t *)mq_id;
    if (!q || q->magic != BM_QUEUE_MAGIC || !msg_ptr) return XY_OS_ERROR_PARAMETER;
    if (msg_prio) *msg_prio = 0;

    uint32_t deadline = (timeout == XY_OS_WAIT_FOREVER)
                        ? 0xFFFFFFFFu
                        : xy_tick_get() + timeout;

    while (1) {
        uint32_t irq = bm_enter_critical();
        if (q->count > 0) {
            memcpy(msg_ptr, q->data + q->head * q->msg_size, q->msg_size);
            q->head = (q->head + 1) % q->capacity;
            q->count--;
            bm_exit_critical(irq);
            return XY_OS_OK;
        }
        bm_exit_critical(irq);
        if (timeout == 0) return XY_OS_ERROR_RESOURCE;
        if (timeout != XY_OS_WAIT_FOREVER && xy_tick_get() >= deadline)
            return XY_OS_ERROR_TIMEOUT;
    }
}

uint32_t xy_os_msgqueue_get_capacity(xy_os_msgqueue_id_t mq_id)
{
    bm_mq_t *q = (bm_mq_t *)mq_id;
    return (q && q->magic == BM_QUEUE_MAGIC) ? q->capacity : 0;
}

uint32_t xy_os_msgqueue_get_msg_size(xy_os_msgqueue_id_t mq_id)
{
    bm_mq_t *q = (bm_mq_t *)mq_id;
    return (q && q->magic == BM_QUEUE_MAGIC) ? q->msg_size : 0;
}

uint32_t xy_os_msgqueue_get_count(xy_os_msgqueue_id_t mq_id)
{
    bm_mq_t *q = (bm_mq_t *)mq_id;
    if (!q || q->magic != BM_QUEUE_MAGIC) return 0;
    uint32_t irq = bm_enter_critical();
    uint32_t c = q->count;
    bm_exit_critical(irq);
    return c;
}

uint32_t xy_os_msgqueue_get_space(xy_os_msgqueue_id_t mq_id)
{
    bm_mq_t *q = (bm_mq_t *)mq_id;
    if (!q || q->magic != BM_QUEUE_MAGIC) return 0;
    uint32_t irq = bm_enter_critical();
    uint32_t space = q->capacity - q->count;
    bm_exit_critical(irq);
    return space;
}

xy_os_status_t xy_os_msgqueue_reset(xy_os_msgqueue_id_t mq_id)
{
    bm_mq_t *q = (bm_mq_t *)mq_id;
    if (!q || q->magic != BM_QUEUE_MAGIC) return XY_OS_ERROR_PARAMETER;
    uint32_t irq = bm_enter_critical();
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
    bm_exit_critical(irq);
    return XY_OS_OK;
}

xy_os_status_t xy_os_msgqueue_delete(xy_os_msgqueue_id_t mq_id)
{
    bm_mq_t *q = (bm_mq_t *)mq_id;
    if (!q || q->magic != BM_QUEUE_MAGIC) return XY_OS_ERROR_PARAMETER;
    q->magic = 0;
    bm_mq_pool_free(q);
    return XY_OS_OK;
}
