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

void setUp(void)
{
    RESET_FAKE(test_timer_callback);
    FFF_RESET_HISTORY();
}

void tearDown(void)
{
}

/* ========== Kernel / Thread / Timer ========== */

static void test_kernel_info_lock_and_ticks(void)
{
    xy_os_version_t version = {0};
    char id[48] = {0};
    TEST_ASSERT_EQUAL(XY_OS_KERNEL_READY, xy_os_kernel_get_state());
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_kernel_get_info(&version, id, sizeof(id)));
    TEST_ASSERT_NOT_EQUAL(0, version.api);
    TEST_ASSERT_NOT_NULL(strstr(id, "Baremetal"));
    TEST_ASSERT_EQUAL_UINT32(1000U, xy_os_kernel_get_tick_freq());
    TEST_ASSERT_EQUAL_UINT32(1000U, xy_os_kernel_get_sys_timer_freq());

    (void)xy_os_kernel_lock();
    TEST_ASSERT_EQUAL(XY_OS_KERNEL_LOCKED, xy_os_kernel_get_state());
    TEST_ASSERT_EQUAL_INT(0, xy_os_kernel_unlock());
    TEST_ASSERT_EQUAL(XY_OS_KERNEL_RUNNING, xy_os_kernel_get_state());
    TEST_ASSERT_EQUAL_INT(0, xy_os_kernel_restore_lock(1));
    TEST_ASSERT_EQUAL(XY_OS_KERNEL_LOCKED, xy_os_kernel_get_state());
    TEST_ASSERT_EQUAL_INT(1, xy_os_kernel_restore_lock(0));
    TEST_ASSERT_EQUAL(XY_OS_KERNEL_RUNNING, xy_os_kernel_get_state());
}

static void test_thread_stub_contract(void)
{
    xy_os_thread_id_t current = xy_os_thread_get_id();
    xy_os_thread_id_t threads[2] = {0};
    TEST_ASSERT_NULL(xy_os_thread_new(NULL, NULL, NULL));
    TEST_ASSERT_NOT_NULL(current);
    TEST_ASSERT_EQUAL_STRING("main", xy_os_thread_get_name(current));
    TEST_ASSERT_EQUAL(XY_OS_THREAD_RUNNING, xy_os_thread_get_state(current));
    TEST_ASSERT_EQUAL(XY_OS_PRIORITY_NORMAL, xy_os_thread_get_priority(current));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_os_thread_get_count());
    TEST_ASSERT_EQUAL_UINT32(1U, xy_os_thread_enumerate(threads, 2));
    TEST_ASSERT_EQUAL_PTR(current, threads[0]);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_thread_yield());
    TEST_ASSERT_EQUAL(XY_OS_ERROR, xy_os_thread_suspend(current));
    TEST_ASSERT_EQUAL_UINT32(0x80000000u, xy_os_thread_flags_set(current, 0x1));
    TEST_ASSERT_EQUAL_INT(0, xy_os_thread_flags_get());
    TEST_ASSERT_EQUAL_INT(0, xy_os_thread_flags_clear(0x1));
    TEST_ASSERT_EQUAL_UINT32(0x80000000u, xy_os_thread_flags_wait(0x1, XY_OS_FLAGS_WAIT_ANY, 0));
}

static void test_timer_one_shot_and_reuse(void)
{
    int arg = 123;
    xy_os_timer_attr_t attr = { .name = "tmr" };

    TEST_ASSERT_NULL(xy_os_timer_new(NULL, XY_OS_TIMER_ONCE, &arg, &attr));
    xy_os_timer_id_t timer = xy_os_timer_new(test_timer_callback, XY_OS_TIMER_ONCE, &arg, &attr);
    TEST_ASSERT_NOT_NULL(timer);
    TEST_ASSERT_EQUAL_STRING("tmr", xy_os_timer_get_name(timer));
    TEST_ASSERT_EQUAL_INT(0, xy_os_timer_is_running(timer));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_timer_start(timer, 1));
    TEST_ASSERT_EQUAL_INT(1, xy_os_timer_is_running(timer));
    xy_timer_sw_poll();
    TEST_ASSERT_EQUAL_UINT(0U, test_timer_callback_fake.call_count);
    xy_timer_sw_poll();
    TEST_ASSERT_EQUAL_UINT(1U, test_timer_callback_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&arg, test_timer_callback_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(123, *(int *)test_timer_callback_fake.arg0_val);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_timer_stop(timer));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_timer_delete(timer));

    timer = xy_os_timer_new(test_timer_callback, XY_OS_TIMER_ONCE, &arg, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_timer_delete(timer));
}

/* ========== Mutex ========== */

static void test_mutex_basic(void)
{
    xy_os_mutex_id_t m = xy_os_mutex_new(NULL);
    TEST_ASSERT_NOT_NULL(m);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_acquire(m, 0));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_release(m));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_delete(m));
}

static void test_mutex_recursive(void)
{
    xy_os_mutex_attr_t attr = { .name = "rec", .attr_bits = XY_OS_MUTEX_RECURSIVE };
    xy_os_mutex_id_t m = xy_os_mutex_new(&attr);
    TEST_ASSERT_NOT_NULL(m);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_acquire(m, 0));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_acquire(m, 0)); /* recursive re-entry */
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_release(m));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_release(m));    /* fully unlocked after 2 releases */
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_acquire(m, 0)); /* can acquire again */
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_release(m));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_delete(m));
}

static void test_mutex_timeout_immediate(void)
{
    xy_os_mutex_id_t m = xy_os_mutex_new(NULL);
    TEST_ASSERT_NOT_NULL(m);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_acquire(m, 0));
    /* Non-recursive mutex held — timeout=0 must return immediately */
    xy_os_status_t s = xy_os_mutex_acquire(m, 0);
    TEST_ASSERT_EQUAL(XY_OS_ERROR_TIMEOUT, s);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_release(m));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_delete(m));
}

static void test_mutex_name(void)
{
    xy_os_mutex_attr_t attr = { .name = "myMtx" };
    xy_os_mutex_id_t m = xy_os_mutex_new(&attr);
    TEST_ASSERT_NOT_NULL(m);
    TEST_ASSERT_EQUAL_STRING("myMtx", xy_os_mutex_get_name(m));
    xy_os_mutex_delete(m);
}

static void test_mutex_external_cb(void)
{
    /* Provide external control-block storage */
    static uint8_t cb_mem[128];
    xy_os_mutex_attr_t attr = {
        .name    = "extMtx",
        .cb_mem  = cb_mem,
        .cb_size = sizeof(cb_mem),
    };
    xy_os_mutex_id_t m = xy_os_mutex_new(&attr);
    TEST_ASSERT_EQUAL_PTR((xy_os_mutex_id_t)cb_mem, m);  /* must use our buffer */
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_acquire(m, 0));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_release(m));
    /* Do NOT call delete on external-memory objects that pool_free would corrupt */
}

/* ========== Semaphore ========== */

static void test_semaphore_basic(void)
{
    xy_os_semaphore_id_t s = xy_os_semaphore_new(3, 3, NULL);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQUAL_UINT32(3U, xy_os_semaphore_get_count(s));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_semaphore_acquire(s, 0));
    TEST_ASSERT_EQUAL_UINT32(2U, xy_os_semaphore_get_count(s));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_semaphore_acquire(s, 0));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_semaphore_acquire(s, 0));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_os_semaphore_get_count(s));
    /* Release back up */
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_semaphore_release(s));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_os_semaphore_get_count(s));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_semaphore_delete(s));
}

static void test_semaphore_timeout(void)
{
    xy_os_semaphore_id_t s = xy_os_semaphore_new(1, 0, NULL);
    TEST_ASSERT_NOT_NULL(s);
    /* No tokens — timeout=0 must return immediately */
    TEST_ASSERT_EQUAL(XY_OS_ERROR_TIMEOUT, xy_os_semaphore_acquire(s, 0));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_semaphore_delete(s));
}

static void test_semaphore_max_count(void)
{
    xy_os_semaphore_id_t s = xy_os_semaphore_new(2, 2, NULL);
    TEST_ASSERT_NOT_NULL(s);
    /* Already at max — release should fail */
    TEST_ASSERT_EQUAL(XY_OS_ERROR_RESOURCE, xy_os_semaphore_release(s));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_semaphore_delete(s));
}

/* ========== Event Flags ========== */

static void test_event_flags_set_get(void)
{
    xy_os_event_flags_id_t ef = xy_os_event_flags_new(NULL);
    TEST_ASSERT_NOT_NULL(ef);
    TEST_ASSERT_EQUAL_INT(0, xy_os_event_flags_get(ef));
    xy_os_event_flags_set(ef, 0x05);
    TEST_ASSERT_EQUAL_UINT32(0x05, xy_os_event_flags_get(ef));
    xy_os_event_flags_set(ef, 0x02);
    TEST_ASSERT_EQUAL_UINT32(0x07, xy_os_event_flags_get(ef));
    xy_os_event_flags_clear(ef, 0x03);
    TEST_ASSERT_EQUAL_UINT32(0x04, xy_os_event_flags_get(ef));
    xy_os_event_flags_delete(ef);
}

static void test_event_flags_wait_any(void)
{
    xy_os_event_flags_id_t ef = xy_os_event_flags_new(NULL);
    TEST_ASSERT_NOT_NULL(ef);
    xy_os_event_flags_set(ef, 0x0F);
    /* WAIT_ANY — any bit in 0x03 set satisfies */
    uint32_t r = xy_os_event_flags_wait(ef, 0x03, XY_OS_FLAGS_WAIT_ANY, 0);
    TEST_ASSERT_EQUAL_INT(0, (r & 0x80000000u));  /* no error */
    /* Flags should be cleared */
    TEST_ASSERT_EQUAL_INT(0, (xy_os_event_flags_get(ef) & 0x03));
    xy_os_event_flags_delete(ef);
}

static void test_event_flags_wait_all(void)
{
    xy_os_event_flags_id_t ef = xy_os_event_flags_new(NULL);
    TEST_ASSERT_NOT_NULL(ef);
    xy_os_event_flags_set(ef, 0x01);  /* only bit 0 */
    /* WAIT_ALL for bits 0x03 — not met, timeout=0 */
    uint32_t r = xy_os_event_flags_wait(ef, 0x03, XY_OS_FLAGS_WAIT_ALL, 0);
    TEST_ASSERT_EQUAL_UINT32(0x80000000u, r);
    /* Set the remaining bit */
    xy_os_event_flags_set(ef, 0x02);
    r = xy_os_event_flags_wait(ef, 0x03, XY_OS_FLAGS_WAIT_ALL, 0);
    TEST_ASSERT_EQUAL_INT(0, (r & 0x80000000u));
    xy_os_event_flags_delete(ef);
}

static void test_event_flags_no_clear(void)
{
    xy_os_event_flags_id_t ef = xy_os_event_flags_new(NULL);
    TEST_ASSERT_NOT_NULL(ef);
    xy_os_event_flags_set(ef, 0xFF);
    uint32_t r = xy_os_event_flags_wait(ef, 0x01,
                     XY_OS_FLAGS_WAIT_ANY | XY_OS_FLAGS_NO_CLEAR, 0);
    TEST_ASSERT_EQUAL_INT(0, (r & 0x80000000u));
    /* Flags must NOT have been cleared */
    TEST_ASSERT_EQUAL_UINT32(0xFF, xy_os_event_flags_get(ef));
    xy_os_event_flags_delete(ef);
}

/* ========== Message Queue ========== */

static void test_msgqueue_put_get(void)
{
    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(4, sizeof(uint32_t), NULL);
    TEST_ASSERT_NOT_NULL(q);
    TEST_ASSERT_EQUAL_UINT32(4U, xy_os_msgqueue_get_capacity(q));
    TEST_ASSERT_EQUAL(sizeof(uint32_t), xy_os_msgqueue_get_msg_size(q));

    uint32_t val = 0xABCD;
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_put(q, &val, 0, 0));
    TEST_ASSERT_EQUAL_UINT32(1U, xy_os_msgqueue_get_count(q));

    uint32_t out = 0;
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_get(q, &out, NULL, 0));
    TEST_ASSERT_EQUAL_UINT32(0xABCD, out);
    TEST_ASSERT_EQUAL_UINT32(0U, xy_os_msgqueue_get_count(q));
    xy_os_msgqueue_delete(q);
}

static void test_msgqueue_fifo_order(void)
{
    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(3, sizeof(uint32_t), NULL);
    TEST_ASSERT_NOT_NULL(q);
    for (uint32_t i = 1; i <= 3; i++)
        TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_put(q, &i, 0, 0));

    for (uint32_t i = 1; i <= 3; i++) {
        uint32_t out = 0;
        TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_get(q, &out, NULL, 0));
        TEST_ASSERT_EQUAL(i, out);
    }
    xy_os_msgqueue_delete(q);
}

static void test_msgqueue_full(void)
{
    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(2, sizeof(uint32_t), NULL);
    TEST_ASSERT_NOT_NULL(q);
    uint32_t v = 1;
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_put(q, &v, 0, 0));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_put(q, &v, 0, 0));
    /* Full — timeout=0 must not block */
    xy_os_status_t s = xy_os_msgqueue_put(q, &v, 0, 0);
    if (s == XY_OS_ERROR_RESOURCE) {
        TEST_ASSERT_EQUAL(XY_OS_ERROR_RESOURCE, s);
    } else {
        TEST_ASSERT_EQUAL(XY_OS_ERROR_TIMEOUT, s);
    }
    xy_os_msgqueue_delete(q);
}

static void test_msgqueue_empty_get(void)
{
    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(2, sizeof(uint32_t), NULL);
    TEST_ASSERT_NOT_NULL(q);
    uint32_t out = 0;
    xy_os_status_t s = xy_os_msgqueue_get(q, &out, NULL, 0);
    if (s == XY_OS_ERROR_RESOURCE) {
        TEST_ASSERT_EQUAL(XY_OS_ERROR_RESOURCE, s);
    } else {
        TEST_ASSERT_EQUAL(XY_OS_ERROR_TIMEOUT, s);
    }
    xy_os_msgqueue_delete(q);
}

static void test_msgqueue_reset(void)
{
    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(4, sizeof(uint32_t), NULL);
    TEST_ASSERT_NOT_NULL(q);
    uint32_t v = 99;
    xy_os_msgqueue_put(q, &v, 0, 0);
    xy_os_msgqueue_put(q, &v, 0, 0);
    TEST_ASSERT_EQUAL_UINT32(2U, xy_os_msgqueue_get_count(q));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_reset(q));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_os_msgqueue_get_count(q));
    xy_os_msgqueue_delete(q);
}

/* ========== Memory Pool ========== */

static void test_mempool_alloc_free(void)
{
    xy_os_mempool_id_t mp = xy_os_mempool_new(4, 16, NULL);
    TEST_ASSERT_NOT_NULL(mp);
    TEST_ASSERT_EQUAL_UINT32(4U, xy_os_mempool_get_capacity(mp));
    TEST_ASSERT_EQUAL_UINT32(4U, xy_os_mempool_get_space(mp));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_os_mempool_get_count(mp));

    void *blk = xy_os_mempool_alloc(mp, 0);
    TEST_ASSERT_NOT_NULL(blk);
    TEST_ASSERT_EQUAL_UINT32(1U, xy_os_mempool_get_count(mp));
    TEST_ASSERT_EQUAL_UINT32(3U, xy_os_mempool_get_space(mp));

    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mempool_free(mp, blk));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_os_mempool_get_count(mp));
    TEST_ASSERT_EQUAL_UINT32(4U, xy_os_mempool_get_space(mp));
    xy_os_mempool_delete(mp);
}

static void test_mempool_exhaust(void)
{
    xy_os_mempool_id_t mp = xy_os_mempool_new(2, 8, NULL);
    TEST_ASSERT_NOT_NULL(mp);
    void *a = xy_os_mempool_alloc(mp, 0);
    void *b = xy_os_mempool_alloc(mp, 0);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    /* Pool exhausted — timeout=0 must return NULL */
    void *c = xy_os_mempool_alloc(mp, 0);
    TEST_ASSERT_NULL(c);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mempool_free(mp, a));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mempool_free(mp, b));
    xy_os_mempool_delete(mp);
}

static void test_mempool_cycle(void)
{
    /* Repeatedly alloc/free to verify free-list consistency */
    xy_os_mempool_id_t mp = xy_os_mempool_new(3, 32, NULL);
    TEST_ASSERT_NOT_NULL(mp);
    for (int round = 0; round < 5; round++) {
        void *ptrs[3];
        for (int i = 0; i < 3; i++) {
            ptrs[i] = xy_os_mempool_alloc(mp, 0);
            TEST_ASSERT_NOT_NULL(ptrs[i]);
        }
        for (int i = 0; i < 3; i++)
            TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mempool_free(mp, ptrs[i]));
    }
    TEST_ASSERT_EQUAL_UINT32(3U, xy_os_mempool_get_space(mp));
    xy_os_mempool_delete(mp);
}

static void test_mempool_write_to_block(void)
{
    xy_os_mempool_id_t mp = xy_os_mempool_new(2, 64, NULL);
    TEST_ASSERT_NOT_NULL(mp);
    uint8_t *blk = (uint8_t *)xy_os_mempool_alloc(mp, 0);
    TEST_ASSERT_NOT_NULL(blk);
    memset(blk, 0xAB, 64);
    TEST_ASSERT_EQUAL_HEX8(0xAB, blk[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAB, blk[63]);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mempool_free(mp, blk));
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
