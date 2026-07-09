/**
 * @file test_osal.c
 * @brief OSAL baremetal backend unit tests (mutex, semaphore, event_flags,
 *        msgqueue, mempool).  Runs on the PC-sim baremetal backend where all
 *        IRQ helpers are no-ops and xy_tick_get() increments per call.
 */

#include <string.h>

/* Pull in the full CMSIS-RTOS2-compatible API */
#include "unity.h"
#include "fff.h"
#include "xy_os.h"
/* Need tick for deadline tests */
#include "inc/xy_os_tick.h"
#include "xy_timer_sw.h"

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(test_timer_callback, void *)

#define TEST(name) static void name(void)
#define ASSERT(cond) TEST_ASSERT_TRUE(cond)

void setUp(void)
{
    RESET_FAKE(test_timer_callback);
    FFF_RESET_HISTORY();
}

void tearDown(void)
{
}

/* ========== Kernel / Thread / Timer ========== */

TEST(test_kernel_info_lock_and_ticks)
{
    xy_os_version_t version = {0};
    char id[48] = {0};
    ASSERT(xy_os_kernel_get_state() == XY_OS_KERNEL_READY);
    ASSERT(xy_os_kernel_get_info(&version, id, sizeof(id)) == XY_OS_OK);
    ASSERT(version.api != 0);
    ASSERT(strstr(id, "Baremetal") != NULL);
    ASSERT(xy_os_kernel_get_tick_freq() == 1000);
    ASSERT(xy_os_kernel_get_sys_timer_freq() == 1000);

    (void)xy_os_kernel_lock();
    ASSERT(xy_os_kernel_get_state() == XY_OS_KERNEL_LOCKED);
    ASSERT(xy_os_kernel_unlock() == 0);
    ASSERT(xy_os_kernel_get_state() == XY_OS_KERNEL_RUNNING);
    ASSERT(xy_os_kernel_restore_lock(1) == 0);
    ASSERT(xy_os_kernel_get_state() == XY_OS_KERNEL_LOCKED);
    ASSERT(xy_os_kernel_restore_lock(0) == 1);
    ASSERT(xy_os_kernel_get_state() == XY_OS_KERNEL_RUNNING);
}

TEST(test_thread_stub_contract)
{
    xy_os_thread_id_t current = xy_os_thread_get_id();
    xy_os_thread_id_t threads[2] = {0};
    ASSERT(xy_os_thread_new(NULL, NULL, NULL) == NULL);
    ASSERT(current != NULL);
    ASSERT(strcmp(xy_os_thread_get_name(current), "main") == 0);
    ASSERT(xy_os_thread_get_state(current) == XY_OS_THREAD_RUNNING);
    ASSERT(xy_os_thread_get_priority(current) == XY_OS_PRIORITY_NORMAL);
    ASSERT(xy_os_thread_get_count() == 1);
    ASSERT(xy_os_thread_enumerate(threads, 2) == 1);
    ASSERT(threads[0] == current);
    ASSERT(xy_os_thread_yield() == XY_OS_OK);
    ASSERT(xy_os_thread_suspend(current) == XY_OS_ERROR);
    ASSERT(xy_os_thread_flags_set(current, 0x1) == 0x80000000u);
    ASSERT(xy_os_thread_flags_get() == 0);
    ASSERT(xy_os_thread_flags_clear(0x1) == 0);
    ASSERT(xy_os_thread_flags_wait(0x1, XY_OS_FLAGS_WAIT_ANY, 0) == 0x80000000u);
}

TEST(test_timer_one_shot_and_reuse)
{
    int arg = 123;
    xy_os_timer_attr_t attr = { .name = "tmr" };

    ASSERT(xy_os_timer_new(NULL, XY_OS_TIMER_ONCE, &arg, &attr) == NULL);
    xy_os_timer_id_t timer = xy_os_timer_new(test_timer_callback, XY_OS_TIMER_ONCE, &arg, &attr);
    ASSERT(timer != NULL);
    ASSERT(strcmp(xy_os_timer_get_name(timer), "tmr") == 0);
    ASSERT(xy_os_timer_is_running(timer) == 0);
    ASSERT(xy_os_timer_start(timer, 1) == XY_OS_OK);
    ASSERT(xy_os_timer_is_running(timer) == 1);
    xy_timer_sw_poll();
    TEST_ASSERT_EQUAL_UINT(0U, test_timer_callback_fake.call_count);
    xy_timer_sw_poll();
    TEST_ASSERT_EQUAL_UINT(1U, test_timer_callback_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&arg, test_timer_callback_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(123, *(int *)test_timer_callback_fake.arg0_val);
    ASSERT(xy_os_timer_stop(timer) == XY_OS_OK);
    ASSERT(xy_os_timer_delete(timer) == XY_OS_OK);

    timer = xy_os_timer_new(test_timer_callback, XY_OS_TIMER_ONCE, &arg, NULL);
    ASSERT(timer != NULL);
    ASSERT(xy_os_timer_delete(timer) == XY_OS_OK);
}

/* ========== Mutex ========== */

TEST(test_mutex_basic)
{
    xy_os_mutex_id_t m = xy_os_mutex_new(NULL);
    ASSERT(m != NULL);
    ASSERT(xy_os_mutex_acquire(m, 0) == XY_OS_OK);
    ASSERT(xy_os_mutex_release(m) == XY_OS_OK);
    ASSERT(xy_os_mutex_delete(m) == XY_OS_OK);
}

TEST(test_mutex_recursive)
{
    xy_os_mutex_attr_t attr = { .name = "rec", .attr_bits = XY_OS_MUTEX_RECURSIVE };
    xy_os_mutex_id_t m = xy_os_mutex_new(&attr);
    ASSERT(m != NULL);
    ASSERT(xy_os_mutex_acquire(m, 0) == XY_OS_OK);
    ASSERT(xy_os_mutex_acquire(m, 0) == XY_OS_OK); /* recursive re-entry */
    ASSERT(xy_os_mutex_release(m) == XY_OS_OK);
    ASSERT(xy_os_mutex_release(m) == XY_OS_OK);    /* fully unlocked after 2 releases */
    ASSERT(xy_os_mutex_acquire(m, 0) == XY_OS_OK); /* can acquire again */
    ASSERT(xy_os_mutex_release(m) == XY_OS_OK);
    ASSERT(xy_os_mutex_delete(m) == XY_OS_OK);
}

TEST(test_mutex_timeout_immediate)
{
    xy_os_mutex_id_t m = xy_os_mutex_new(NULL);
    ASSERT(m != NULL);
    ASSERT(xy_os_mutex_acquire(m, 0) == XY_OS_OK);
    /* Non-recursive mutex held — timeout=0 must return immediately */
    xy_os_status_t s = xy_os_mutex_acquire(m, 0);
    ASSERT(s == XY_OS_ERROR_TIMEOUT);
    ASSERT(xy_os_mutex_release(m) == XY_OS_OK);
    ASSERT(xy_os_mutex_delete(m) == XY_OS_OK);
}

TEST(test_mutex_name)
{
    xy_os_mutex_attr_t attr = { .name = "myMtx" };
    xy_os_mutex_id_t m = xy_os_mutex_new(&attr);
    ASSERT(m != NULL);
    ASSERT(strcmp(xy_os_mutex_get_name(m), "myMtx") == 0);
    xy_os_mutex_delete(m);
}

TEST(test_mutex_external_cb)
{
    /* Provide external control-block storage */
    static uint8_t cb_mem[128];
    xy_os_mutex_attr_t attr = {
        .name    = "extMtx",
        .cb_mem  = cb_mem,
        .cb_size = sizeof(cb_mem),
    };
    xy_os_mutex_id_t m = xy_os_mutex_new(&attr);
    ASSERT(m == (xy_os_mutex_id_t)cb_mem);  /* must use our buffer */
    ASSERT(xy_os_mutex_acquire(m, 0) == XY_OS_OK);
    ASSERT(xy_os_mutex_release(m) == XY_OS_OK);
    /* Do NOT call delete on external-memory objects that pool_free would corrupt */
}

/* ========== Semaphore ========== */

TEST(test_semaphore_basic)
{
    xy_os_semaphore_id_t s = xy_os_semaphore_new(3, 3, NULL);
    ASSERT(s != NULL);
    ASSERT(xy_os_semaphore_get_count(s) == 3);
    ASSERT(xy_os_semaphore_acquire(s, 0) == XY_OS_OK);
    ASSERT(xy_os_semaphore_get_count(s) == 2);
    ASSERT(xy_os_semaphore_acquire(s, 0) == XY_OS_OK);
    ASSERT(xy_os_semaphore_acquire(s, 0) == XY_OS_OK);
    ASSERT(xy_os_semaphore_get_count(s) == 0);
    /* Release back up */
    ASSERT(xy_os_semaphore_release(s) == XY_OS_OK);
    ASSERT(xy_os_semaphore_get_count(s) == 1);
    ASSERT(xy_os_semaphore_delete(s) == XY_OS_OK);
}

TEST(test_semaphore_timeout)
{
    xy_os_semaphore_id_t s = xy_os_semaphore_new(1, 0, NULL);
    ASSERT(s != NULL);
    /* No tokens — timeout=0 must return immediately */
    ASSERT(xy_os_semaphore_acquire(s, 0) == XY_OS_ERROR_TIMEOUT);
    ASSERT(xy_os_semaphore_delete(s) == XY_OS_OK);
}

TEST(test_semaphore_max_count)
{
    xy_os_semaphore_id_t s = xy_os_semaphore_new(2, 2, NULL);
    ASSERT(s != NULL);
    /* Already at max — release should fail */
    ASSERT(xy_os_semaphore_release(s) == XY_OS_ERROR_RESOURCE);
    ASSERT(xy_os_semaphore_delete(s) == XY_OS_OK);
}

/* ========== Event Flags ========== */

TEST(test_event_flags_set_get)
{
    xy_os_event_flags_id_t ef = xy_os_event_flags_new(NULL);
    ASSERT(ef != NULL);
    ASSERT(xy_os_event_flags_get(ef) == 0);
    xy_os_event_flags_set(ef, 0x05);
    ASSERT(xy_os_event_flags_get(ef) == 0x05);
    xy_os_event_flags_set(ef, 0x02);
    ASSERT(xy_os_event_flags_get(ef) == 0x07);
    xy_os_event_flags_clear(ef, 0x03);
    ASSERT(xy_os_event_flags_get(ef) == 0x04);
    xy_os_event_flags_delete(ef);
}

TEST(test_event_flags_wait_any)
{
    xy_os_event_flags_id_t ef = xy_os_event_flags_new(NULL);
    ASSERT(ef != NULL);
    xy_os_event_flags_set(ef, 0x0F);
    /* WAIT_ANY — any bit in 0x03 set satisfies */
    uint32_t r = xy_os_event_flags_wait(ef, 0x03, XY_OS_FLAGS_WAIT_ANY, 0);
    ASSERT((r & 0x80000000u) == 0);  /* no error */
    /* Flags should be cleared */
    ASSERT((xy_os_event_flags_get(ef) & 0x03) == 0);
    xy_os_event_flags_delete(ef);
}

TEST(test_event_flags_wait_all)
{
    xy_os_event_flags_id_t ef = xy_os_event_flags_new(NULL);
    ASSERT(ef != NULL);
    xy_os_event_flags_set(ef, 0x01);  /* only bit 0 */
    /* WAIT_ALL for bits 0x03 — not met, timeout=0 */
    uint32_t r = xy_os_event_flags_wait(ef, 0x03, XY_OS_FLAGS_WAIT_ALL, 0);
    ASSERT(r == 0x80000000u);
    /* Set the remaining bit */
    xy_os_event_flags_set(ef, 0x02);
    r = xy_os_event_flags_wait(ef, 0x03, XY_OS_FLAGS_WAIT_ALL, 0);
    ASSERT((r & 0x80000000u) == 0);
    xy_os_event_flags_delete(ef);
}

TEST(test_event_flags_no_clear)
{
    xy_os_event_flags_id_t ef = xy_os_event_flags_new(NULL);
    ASSERT(ef != NULL);
    xy_os_event_flags_set(ef, 0xFF);
    uint32_t r = xy_os_event_flags_wait(ef, 0x01,
                     XY_OS_FLAGS_WAIT_ANY | XY_OS_FLAGS_NO_CLEAR, 0);
    ASSERT((r & 0x80000000u) == 0);
    /* Flags must NOT have been cleared */
    ASSERT(xy_os_event_flags_get(ef) == 0xFF);
    xy_os_event_flags_delete(ef);
}

/* ========== Message Queue ========== */

TEST(test_msgqueue_put_get)
{
    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(4, sizeof(uint32_t), NULL);
    ASSERT(q != NULL);
    ASSERT(xy_os_msgqueue_get_capacity(q) == 4);
    ASSERT(xy_os_msgqueue_get_msg_size(q) == sizeof(uint32_t));

    uint32_t val = 0xABCD;
    ASSERT(xy_os_msgqueue_put(q, &val, 0, 0) == XY_OS_OK);
    ASSERT(xy_os_msgqueue_get_count(q) == 1);

    uint32_t out = 0;
    ASSERT(xy_os_msgqueue_get(q, &out, NULL, 0) == XY_OS_OK);
    ASSERT(out == 0xABCD);
    ASSERT(xy_os_msgqueue_get_count(q) == 0);
    xy_os_msgqueue_delete(q);
}

TEST(test_msgqueue_fifo_order)
{
    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(3, sizeof(uint32_t), NULL);
    ASSERT(q != NULL);
    for (uint32_t i = 1; i <= 3; i++)
        ASSERT(xy_os_msgqueue_put(q, &i, 0, 0) == XY_OS_OK);

    for (uint32_t i = 1; i <= 3; i++) {
        uint32_t out = 0;
        ASSERT(xy_os_msgqueue_get(q, &out, NULL, 0) == XY_OS_OK);
        ASSERT(out == i);
    }
    xy_os_msgqueue_delete(q);
}

TEST(test_msgqueue_full)
{
    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(2, sizeof(uint32_t), NULL);
    ASSERT(q != NULL);
    uint32_t v = 1;
    ASSERT(xy_os_msgqueue_put(q, &v, 0, 0) == XY_OS_OK);
    ASSERT(xy_os_msgqueue_put(q, &v, 0, 0) == XY_OS_OK);
    /* Full — timeout=0 must not block */
    xy_os_status_t s = xy_os_msgqueue_put(q, &v, 0, 0);
    ASSERT(s == XY_OS_ERROR_RESOURCE || s == XY_OS_ERROR_TIMEOUT);
    xy_os_msgqueue_delete(q);
}

TEST(test_msgqueue_empty_get)
{
    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(2, sizeof(uint32_t), NULL);
    ASSERT(q != NULL);
    uint32_t out = 0;
    xy_os_status_t s = xy_os_msgqueue_get(q, &out, NULL, 0);
    ASSERT(s == XY_OS_ERROR_RESOURCE || s == XY_OS_ERROR_TIMEOUT);
    xy_os_msgqueue_delete(q);
}

TEST(test_msgqueue_reset)
{
    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(4, sizeof(uint32_t), NULL);
    ASSERT(q != NULL);
    uint32_t v = 99;
    xy_os_msgqueue_put(q, &v, 0, 0);
    xy_os_msgqueue_put(q, &v, 0, 0);
    ASSERT(xy_os_msgqueue_get_count(q) == 2);
    ASSERT(xy_os_msgqueue_reset(q) == XY_OS_OK);
    ASSERT(xy_os_msgqueue_get_count(q) == 0);
    xy_os_msgqueue_delete(q);
}

/* ========== Memory Pool ========== */

TEST(test_mempool_alloc_free)
{
    xy_os_mempool_id_t mp = xy_os_mempool_new(4, 16, NULL);
    ASSERT(mp != NULL);
    ASSERT(xy_os_mempool_get_capacity(mp) == 4);
    ASSERT(xy_os_mempool_get_space(mp) == 4);
    ASSERT(xy_os_mempool_get_count(mp) == 0);

    void *blk = xy_os_mempool_alloc(mp, 0);
    ASSERT(blk != NULL);
    ASSERT(xy_os_mempool_get_count(mp) == 1);
    ASSERT(xy_os_mempool_get_space(mp) == 3);

    ASSERT(xy_os_mempool_free(mp, blk) == XY_OS_OK);
    ASSERT(xy_os_mempool_get_count(mp) == 0);
    ASSERT(xy_os_mempool_get_space(mp) == 4);
    xy_os_mempool_delete(mp);
}

TEST(test_mempool_exhaust)
{
    xy_os_mempool_id_t mp = xy_os_mempool_new(2, 8, NULL);
    ASSERT(mp != NULL);
    void *a = xy_os_mempool_alloc(mp, 0);
    void *b = xy_os_mempool_alloc(mp, 0);
    ASSERT(a != NULL && b != NULL);
    /* Pool exhausted — timeout=0 must return NULL */
    void *c = xy_os_mempool_alloc(mp, 0);
    ASSERT(c == NULL);
    ASSERT(xy_os_mempool_free(mp, a) == XY_OS_OK);
    ASSERT(xy_os_mempool_free(mp, b) == XY_OS_OK);
    xy_os_mempool_delete(mp);
}

TEST(test_mempool_cycle)
{
    /* Repeatedly alloc/free to verify free-list consistency */
    xy_os_mempool_id_t mp = xy_os_mempool_new(3, 32, NULL);
    ASSERT(mp != NULL);
    for (int round = 0; round < 5; round++) {
        void *ptrs[3];
        for (int i = 0; i < 3; i++) {
            ptrs[i] = xy_os_mempool_alloc(mp, 0);
            ASSERT(ptrs[i] != NULL);
        }
        for (int i = 0; i < 3; i++)
            ASSERT(xy_os_mempool_free(mp, ptrs[i]) == XY_OS_OK);
    }
    ASSERT(xy_os_mempool_get_space(mp) == 3);
    xy_os_mempool_delete(mp);
}

TEST(test_mempool_write_to_block)
{
    xy_os_mempool_id_t mp = xy_os_mempool_new(2, 64, NULL);
    ASSERT(mp != NULL);
    uint8_t *blk = (uint8_t *)xy_os_mempool_alloc(mp, 0);
    ASSERT(blk != NULL);
    memset(blk, 0xAB, 64);
    ASSERT(blk[0] == 0xAB && blk[63] == 0xAB);
    ASSERT(xy_os_mempool_free(mp, blk) == XY_OS_OK);
    xy_os_mempool_delete(mp);
}

/* ========== main ========== */

int main(void)
{
    UNITY_BEGIN();
    xy_os_kernel_init();

    /* Kernel / Thread / Timer */
    RUN_TEST(test_kernel_info_lock_and_ticks);
    RUN_TEST(test_thread_stub_contract);
    RUN_TEST(test_timer_one_shot_and_reuse);

    /* Mutex */
    RUN_TEST(test_mutex_basic);
    RUN_TEST(test_mutex_recursive);
    RUN_TEST(test_mutex_timeout_immediate);
    RUN_TEST(test_mutex_name);
    RUN_TEST(test_mutex_external_cb);

    /* Semaphore */
    RUN_TEST(test_semaphore_basic);
    RUN_TEST(test_semaphore_timeout);
    RUN_TEST(test_semaphore_max_count);

    /* Event Flags */
    RUN_TEST(test_event_flags_set_get);
    RUN_TEST(test_event_flags_wait_any);
    RUN_TEST(test_event_flags_wait_all);
    RUN_TEST(test_event_flags_no_clear);

    /* Message Queue */
    RUN_TEST(test_msgqueue_put_get);
    RUN_TEST(test_msgqueue_fifo_order);
    RUN_TEST(test_msgqueue_full);
    RUN_TEST(test_msgqueue_empty_get);
    RUN_TEST(test_msgqueue_reset);

    /* Memory Pool */
    RUN_TEST(test_mempool_alloc_free);
    RUN_TEST(test_mempool_exhaust);
    RUN_TEST(test_mempool_cycle);
    RUN_TEST(test_mempool_write_to_block);

    return UNITY_END();
}
