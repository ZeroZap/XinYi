/**
 * @file test_osal.c
 * @brief OSAL baremetal backend unit tests (mutex, semaphore, event_flags,
 *        msgqueue, mempool).  Runs on the PC-sim baremetal backend where all
 *        IRQ helpers are no-ops and xy_tick_get() increments per call.
 */

#include <stdint.h>
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
    TEST_ASSERT_EQUAL(XY_OS_ERROR_NOT_SUPPORTED, xy_os_thread_suspend(current));
    TEST_ASSERT_EQUAL_UINT32(0x80000000u, xy_os_thread_flags_set(current, 0x1));
    /* get() used to answer 0 and clear() reported success. Zero is what "no flags are set" looks like,
     * so on a backend that has no thread flags at all a caller polling for work was told, forever,
     * that there was none - and clear() reported clearing something that never existed. The other
     * backends answer 0x80000000 here: this one now does too. */
    TEST_ASSERT_EQUAL_UINT32(XY_OS_THREAD_FLAGS_ERROR, xy_os_thread_flags_get());
    TEST_ASSERT_EQUAL_UINT32(XY_OS_THREAD_FLAGS_ERROR, xy_os_thread_flags_clear(0x1));
    /* And the error answer must not be mistakable for a flag word. */
    TEST_ASSERT_NOT_EQUAL(0U, xy_os_thread_flags_get());
    TEST_ASSERT_EQUAL_UINT32(0x80000000u, xy_os_thread_flags_wait(0x1, XY_OS_FLAGS_WAIT_ANY, 0));
}

/* Suspending, resuming, re-prioritising, detaching, joining and terminating a thread all need a
 * thread table, and this backend has one context and no scheduler. It used to answer XY_OS_ERROR -
 * "unspecified RTOS error" - for every one of them, which reads the same as a driver fault and tells a
 * caller nothing it can act on. The answer is that the operation does not exist here. */
static void test_thread_operations_this_backend_cannot_do(void)
{
    xy_os_thread_id_t current = xy_os_thread_get_id();

    TEST_ASSERT_EQUAL(XY_OS_ERROR_NOT_SUPPORTED, xy_os_thread_suspend(current));
    TEST_ASSERT_EQUAL(XY_OS_ERROR_NOT_SUPPORTED, xy_os_thread_resume(current));
    TEST_ASSERT_EQUAL(XY_OS_ERROR_NOT_SUPPORTED, xy_os_thread_set_priority(current, XY_OS_PRIORITY_HIGH));
    TEST_ASSERT_EQUAL(XY_OS_ERROR_NOT_SUPPORTED, xy_os_thread_detach(current));
    TEST_ASSERT_EQUAL(XY_OS_ERROR_NOT_SUPPORTED, xy_os_thread_join(current));
    TEST_ASSERT_EQUAL(XY_OS_ERROR_NOT_SUPPORTED, xy_os_thread_terminate(current));

    /* The distinction is the whole point: "cannot be done here" is not "something went wrong". */
    TEST_ASSERT_NOT_EQUAL(XY_OS_ERROR, xy_os_thread_suspend(current));
    /* And it is not the parameter error either - the handle was fine. */
    TEST_ASSERT_NOT_EQUAL(XY_OS_ERROR_PARAMETER, xy_os_thread_suspend(current));
}

/* Every other handle check in this backend answers XY_OS_ERROR_PARAMETER; the three timer entries
 * were the exception, answering the unspecified error instead, so the same misuse had two answers
 * inside one file. */
static void test_timer_handle_misuse_is_a_parameter_error(void)
{
    TEST_ASSERT_EQUAL(XY_OS_ERROR_PARAMETER, xy_os_timer_start(NULL, 10U));
    TEST_ASSERT_EQUAL(XY_OS_ERROR_PARAMETER, xy_os_timer_stop(NULL));
    TEST_ASSERT_EQUAL(XY_OS_ERROR_PARAMETER, xy_os_timer_delete(NULL));
    TEST_ASSERT_EQUAL(XY_OS_ERROR_PARAMETER, xy_os_timer_start((xy_os_timer_id_t)99, 10U));
    TEST_ASSERT_EQUAL(XY_OS_ERROR_PARAMETER, xy_os_timer_stop((xy_os_timer_id_t)99));
    TEST_ASSERT_EQUAL(XY_OS_ERROR_PARAMETER, xy_os_timer_delete((xy_os_timer_id_t)99));
}

/* The software-timer pool holds sixteen entries and xy_os_timer_start creates one per call. A stop
 * that drops the id without releasing the entry leaks it, so an application that starts and stops a
 * timer in a loop - the ordinary way to use one - dies after sixteen cycles, with the unspecified
 * error as the only clue. Twenty cycles is more than the pool has, on purpose. */
static void test_start_and_stop_does_not_exhaust_the_timer_pool(void)
{
    int arg = 0;
    xy_os_timer_attr_t attr = { .name = "pool" };
    xy_os_timer_id_t timer = xy_os_timer_new(test_timer_callback, XY_OS_TIMER_ONCE, &arg, &attr);

    TEST_ASSERT_NOT_NULL(timer);
    for (int cycle = 0; cycle < 20; ++cycle) {
        TEST_ASSERT_EQUAL_MESSAGE(XY_OS_OK, xy_os_timer_start(timer, 100U), "a start after a stop");
        TEST_ASSERT_EQUAL_MESSAGE(XY_OS_OK, xy_os_timer_stop(timer), "a stop");
    }
    /* Starting twice without stopping leaks the first entry in the same way. */
    for (int cycle = 0; cycle < 20; ++cycle) {
        TEST_ASSERT_EQUAL_MESSAGE(XY_OS_OK, xy_os_timer_start(timer, 100U), "a repeated start");
    }
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_timer_delete(timer));
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

/* Bit 31 is the error answer for this whole family of calls, and this backend did not reserve it: a
 * caller could set 0x80000000 as an ordinary flag, after which xy_os_event_flags_get returned exactly
 * the value that means "the call failed". The error could be forged, and a real error could be read as
 * a flag. FreeRTOS rejects masks containing that bit and rejects an empty mask; this backend accepted
 * both, so the same call was refused on one backend and silently stored on the other. */
static void test_event_flags_reserve_the_error_bit_and_refuse_empty_masks(void)
{
    xy_os_event_flags_id_t ef = xy_os_event_flags_new(NULL);
    TEST_ASSERT_NOT_NULL(ef);

    /* The reserved bit cannot be stored, so get() can never return the error answer by accident. */
    TEST_ASSERT_EQUAL_UINT32(XY_OS_EVENT_FLAGS_ERROR,
                             xy_os_event_flags_set(ef, XY_OS_EVENT_FLAGS_ERROR));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_os_event_flags_get(ef));
    TEST_ASSERT_NOT_EQUAL(XY_OS_EVENT_FLAGS_ERROR, xy_os_event_flags_get(ef));

    /* A mask of zero asks to set/clear nothing, which is not a request. */
    TEST_ASSERT_EQUAL_UINT32(XY_OS_EVENT_FLAGS_ERROR, xy_os_event_flags_set(ef, 0U));
    TEST_ASSERT_EQUAL_UINT32(XY_OS_EVENT_FLAGS_ERROR, xy_os_event_flags_clear(ef, 0U));

    /* The reserved range is the whole top byte, as on FreeRTOS, not just the error bit: those bits are
     * FreeRTOS control bits, and a mask of 0x01000000 was accepted here and refused there. */
    TEST_ASSERT_EQUAL_UINT32(XY_OS_EVENT_FLAGS_ERROR, xy_os_event_flags_set(ef, 0x01000000U));
    TEST_ASSERT_EQUAL_UINT32(XY_OS_EVENT_FLAGS_ERROR, xy_os_event_flags_set(ef, 0xFF000000U));

    /* A legal mask still works, and mixing in the reserved range does not sneak it past. */
    TEST_ASSERT_EQUAL_UINT32(0x00000005U, xy_os_event_flags_set(ef, 0x00000005U));
    TEST_ASSERT_EQUAL_UINT32(XY_OS_EVENT_FLAGS_ERROR,
                             xy_os_event_flags_set(ef, 0x0000000AU | XY_OS_EVENT_FLAGS_ERROR));
    TEST_ASSERT_EQUAL_UINT32(0x00000005U, xy_os_event_flags_get(ef));
    xy_os_event_flags_delete(ef);
}

static void test_event_flags_set_get(void)
{
    xy_os_event_flags_id_t ef = xy_os_event_flags_new(NULL);
    TEST_ASSERT_NOT_NULL(ef);
    TEST_ASSERT_BITS_HIGH(0x80000000u, xy_os_event_flags_get(NULL));
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

/* msg_count * msg_size was computed with no overflow check. Sixteen bits each multiply to zero, so the
 * size test passed and the queue was built on the small embedded buffer while still reporting the huge
 * capacity and message size - every later put writes far past it. The only safe answer is to refuse the
 * parameters. */
static void test_msgqueue_dimensions_that_overflow_are_refused(void)
{
    TEST_ASSERT_NULL(xy_os_msgqueue_new(0x10000U, 0x10000U, NULL));
    TEST_ASSERT_NULL(xy_os_msgqueue_new(0xFFFFFFFFU, 2U, NULL));
    /* The boundary itself is still accepted. */
    xy_os_msgqueue_id_t ok = xy_os_msgqueue_new(4U, 4U, NULL);
    TEST_ASSERT_NOT_NULL(ok);
    TEST_ASSERT_EQUAL_UINT32(4U, xy_os_msgqueue_get_capacity(ok));
    xy_os_msgqueue_delete(ok);
    /* Zero dimensions were already refused; keep that pinned so the overflow fix does not relax it. */
    TEST_ASSERT_NULL(xy_os_msgqueue_new(0U, 4U, NULL));
    TEST_ASSERT_NULL(xy_os_msgqueue_new(4U, 0U, NULL));
}

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
    /* Full and told not to wait: that is a resource condition, not a timeout - nothing was waited
     * for. This assertion used to accept either code, which is how the two backends disagreed without
     * anybody noticing. */
    TEST_ASSERT_EQUAL(XY_OS_ERROR_RESOURCE, xy_os_msgqueue_put(q, &v, 0, 0));
    /* A finite wait cannot be exercised here: this backend polls xy_tick_get(), and on the host
     * nothing advances the tick, so any timeout above zero spins forever. That is a property of the
     * backend worth knowing - its waits depend entirely on an external tick source - but it belongs in
     * the contract, not in a test that would hang the suite. */
    xy_os_msgqueue_delete(q);
}

static void test_msgqueue_empty_get(void)
{
    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(2, sizeof(uint32_t), NULL);
    TEST_ASSERT_NOT_NULL(q);
    uint32_t out = 0;
    /* Empty and told not to wait: nothing was waited for, so it is not a timeout. */
    TEST_ASSERT_EQUAL(XY_OS_ERROR_RESOURCE, xy_os_msgqueue_get(q, &out, NULL, 0));
    /* The waited-for variant is documented as XY_OS_ERROR_TIMEOUT but cannot be run here; see the
     * note in test_msgqueue_full. */
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

static void test_msgqueue_binary_fixed_size_message(void)
{
    typedef struct {
        uint8_t bytes[8];
    } binary_msg_t;

    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(2, sizeof(binary_msg_t), NULL);
    TEST_ASSERT_NOT_NULL(q);

    binary_msg_t in = { .bytes = { 0x00, 0x41, 0x00, 0x42, 0xFF, 0x10, 0x00, 0x7E } };
    binary_msg_t out;
    memset(&out, 0xA5, sizeof(out));

    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_put(q, &in, 0, 0));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_get(q, &out, NULL, 0));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(in.bytes, out.bytes, sizeof(in.bytes));
    xy_os_msgqueue_delete(q);
}

/* No backend implements priority ordering and a FIFO queue cannot preserve a priority contract, so a
 * non-zero priority is refused rather than silently dropped. This test used to assert the opposite -
 * that the value was ignored and the message queued - while the FreeRTOS backend refused the same
 * call and a guard required it to. Two contracts for one API, each with its own test. The one with a
 * stated reason wins: a caller that believes messages are ordered by priority hears nothing when they
 * are not. */
static void test_msgqueue_priority_is_refused_contract(void)
{
    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(2, sizeof(uint32_t), NULL);
    TEST_ASSERT_NOT_NULL(q);

    uint32_t low = 1;
    uint32_t high = 2;
    uint32_t out = 0;
    uint8_t prio = 0xA5;

    TEST_ASSERT_EQUAL(XY_OS_ERROR_PARAMETER, xy_os_msgqueue_put(q, &low, 1, 0));
    TEST_ASSERT_EQUAL(XY_OS_ERROR_PARAMETER, xy_os_msgqueue_put(q, &high, 255, 0));
    /* Nothing was queued by either refused call. */
    TEST_ASSERT_EQUAL_UINT32(0U, xy_os_msgqueue_get_count(q));

    /* Zero means "no priority" and is the only accepted value; the queue is FIFO. */
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_put(q, &low, 0, 0));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_put(q, &high, 0, 0));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_get(q, &out, &prio, 0));
    TEST_ASSERT_EQUAL_UINT32(low, out);
    TEST_ASSERT_EQUAL_UINT8(0U, prio);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_get(q, &out, &prio, 0));
    TEST_ASSERT_EQUAL_UINT32(high, out);
    xy_os_msgqueue_delete(q);
}

static void test_msgqueue_static_cb_and_storage(void)
{
    static uint8_t cb_mem[512];
    static uint8_t mq_mem[3 * sizeof(uint16_t)];
    xy_os_msgqueue_attr_t attr = {
        .name = "extMq",
        .cb_mem = cb_mem,
        .cb_size = sizeof(cb_mem),
        .mq_mem = mq_mem,
        .mq_size = sizeof(mq_mem),
    };

    xy_os_msgqueue_id_t q = xy_os_msgqueue_new(3, sizeof(uint16_t), &attr);
    TEST_ASSERT_EQUAL_PTR((xy_os_msgqueue_id_t)cb_mem, q);
    TEST_ASSERT_EQUAL_STRING("extMq", xy_os_msgqueue_get_name(q));
    TEST_ASSERT_EQUAL_UINT32(3U, xy_os_msgqueue_get_capacity(q));
    TEST_ASSERT_EQUAL_UINT32(sizeof(uint16_t), xy_os_msgqueue_get_msg_size(q));

    uint16_t in = 0xBEEF;
    uint16_t out = 0;
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_put(q, &in, 0, 0));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_get(q, &out, NULL, 0));
    TEST_ASSERT_EQUAL_HEX16(in, out);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_msgqueue_delete(q));
}

/* ========== Memory Pool ========== */

/* The same product as the message queue, computed the same way and with the same absence of a check:
 * block_count * block_size wrapped to zero, the embedded region looked big enough, and the pool
 * reported a capacity it could never hold. Fixing the queue alone left the sibling broken. */
/* Blocks carry a free-list pointer inside them, and the stride was block_size (rounded up only to
 * sizeof(void *), never to an alignment). Nine-byte blocks therefore put the next block's link at
 * offset 9, and writing a pointer there is an unaligned store - tolerated on Cortex-M, a fault on the
 * RISC-V core this project also targets. The FreeRTOS backend rounds the stride up to pointer alignment
 * for exactly this reason, so the same pool works there and faults here. */
/* A block from somewhere else was accepted: free() wrote the free-list head through whatever pointer it
 * was given, so a stale or foreign pointer became an arbitrary write, and it reported success. The pool
 * knows its own region, so it can refuse anything outside it - and refuse a pointer that is inside but
 * not on a block boundary, which is the same write in a smaller form. */
static void test_mempool_free_refuses_a_pointer_it_never_handed_out(void)
{
    xy_os_mempool_id_t mp = xy_os_mempool_new(4U, 16U, NULL);
    TEST_ASSERT_NOT_NULL(mp);

    uint32_t victim = 0xDEADBEEFU;
    /* A stack address is the cheapest foreign pointer to try, and the write lands somewhere we can
     * inspect rather than somewhere that crashes. */
    TEST_ASSERT_EQUAL(XY_OS_ERROR_PARAMETER, xy_os_mempool_free(mp, &victim));
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEFU, victim);

    /* Inside the pool's region but not on a block boundary: still not a block. */
    void *block = xy_os_mempool_alloc(mp, 0U);
    TEST_ASSERT_NOT_NULL(block);
    TEST_ASSERT_EQUAL(XY_OS_ERROR_PARAMETER, xy_os_mempool_free(mp, (uint8_t *)block + 1U));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mempool_free(mp, block));
    xy_os_mempool_delete(mp);
}

/* Freeing the same block twice put it on the free-list twice, so two later allocations handed out the
 * same memory to two owners - each of them certain it was theirs alone. */
static void test_mempool_double_free_is_refused(void)
{
    xy_os_mempool_id_t mp = xy_os_mempool_new(2U, 16U, NULL);
    TEST_ASSERT_NOT_NULL(mp);

    void *block = xy_os_mempool_alloc(mp, 0U);
    TEST_ASSERT_NOT_NULL(block);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mempool_free(mp, block));
    TEST_ASSERT_EQUAL(XY_OS_ERROR_PARAMETER, xy_os_mempool_free(mp, block));

    /* And the two allocations that follow are different memory. */
    void *first = xy_os_mempool_alloc(mp, 0U);
    void *second = xy_os_mempool_alloc(mp, 0U);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_NOT_EQUAL(first, second);
    xy_os_mempool_free(mp, first);
    xy_os_mempool_free(mp, second);
    xy_os_mempool_delete(mp);
}

static void test_mempool_blocks_are_pointer_aligned(void)
{
    xy_os_mempool_id_t mp = xy_os_mempool_new(4U, 9U, NULL);
    TEST_ASSERT_NOT_NULL(mp);

    const uintptr_t align = (uintptr_t)sizeof(void *);
    void *first = xy_os_mempool_alloc(mp, 0U);
    void *second = xy_os_mempool_alloc(mp, 0U);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);
    /* Every block the pool hands out must be able to hold a pointer at its own address. */
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)((uintptr_t)first % align));
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)((uintptr_t)second % align));
    /* And the reported block size is the stride it actually uses, which is at least the request. */
    TEST_ASSERT_TRUE(xy_os_mempool_get_block_size(mp) >= 9U);
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)(xy_os_mempool_get_block_size(mp) % align));
    xy_os_mempool_free(mp, first);
    xy_os_mempool_free(mp, second);
    xy_os_mempool_delete(mp);
}

static void test_mempool_dimensions_that_overflow_are_refused(void)
{
    TEST_ASSERT_NULL(xy_os_mempool_new(0x10000000U, 16U, NULL));
    TEST_ASSERT_NULL(xy_os_mempool_new(0xFFFFFFFFU, 4U, NULL));
    /* An ordinary pool still works, and its reported size is what it can really hold. */
    xy_os_mempool_id_t ok = xy_os_mempool_new(8U, 16U, NULL);
    TEST_ASSERT_NOT_NULL(ok);
    TEST_ASSERT_EQUAL_UINT32(8U, xy_os_mempool_get_capacity(ok));
    TEST_ASSERT_EQUAL_UINT32(16U, xy_os_mempool_get_block_size(ok));
    xy_os_mempool_delete(ok);
    /* Zero dimensions stay refused. */
    TEST_ASSERT_NULL(xy_os_mempool_new(0U, 16U, NULL));
    TEST_ASSERT_NULL(xy_os_mempool_new(8U, 0U, NULL));
}

/* get_owner() was a stub that returned NULL whatever the state of the mutex, so a deadlock check or an
 * assertion built on it was told "nobody holds this" about a mutex that was held. Nothing tested it. */
/* Three queries answered the same thing for every argument they were given: the state was RUNNING, the
 * name was "main" and the priority was normal, even for an id that never existed. A caller asking about
 * a thread that does not exist was told about the main one. The one context here is a real answer - for
 * that id, and NULL means the current thread for get_name - so the tests pin both sides. */
static void test_thread_queries_answer_about_the_id_they_were_given(void)
{
    const xy_os_thread_id_t current = xy_os_thread_get_id();
    const xy_os_thread_id_t unknown = (void *)0x2;

    TEST_ASSERT_EQUAL(XY_OS_THREAD_RUNNING, xy_os_thread_get_state(current));
    TEST_ASSERT_EQUAL(XY_OS_THREAD_ERROR, xy_os_thread_get_state(unknown));
    TEST_ASSERT_EQUAL(XY_OS_THREAD_ERROR, xy_os_thread_get_state(NULL));

    TEST_ASSERT_EQUAL_STRING("main", xy_os_thread_get_name(current));
    /* NULL is documented as "the current thread", so it must keep answering. */
    TEST_ASSERT_EQUAL_STRING("main", xy_os_thread_get_name(NULL));
    TEST_ASSERT_NULL(xy_os_thread_get_name(unknown));

    TEST_ASSERT_EQUAL(XY_OS_PRIORITY_NORMAL, xy_os_thread_get_priority(current));
    TEST_ASSERT_EQUAL(XY_OS_PRIORITY_ERROR, xy_os_thread_get_priority(unknown));
}

static void test_mutex_reports_its_owner(void)
{
    xy_os_mutex_id_t m = xy_os_mutex_new(NULL);
    TEST_ASSERT_NOT_NULL(m);

    /* Free: no owner. */
    TEST_ASSERT_NULL(xy_os_mutex_get_owner(m));

    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_acquire(m, 0U));
    TEST_ASSERT_EQUAL_PTR(xy_os_thread_get_id(), xy_os_mutex_get_owner(m));
    TEST_ASSERT_NOT_NULL(xy_os_mutex_get_owner(m));

    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_release(m));
    TEST_ASSERT_NULL(xy_os_mutex_get_owner(m));

    /* A handle that is not a mutex answers NULL rather than reading through it. */
    TEST_ASSERT_NULL(xy_os_mutex_get_owner(NULL));

    /* A recursive mutex keeps its owner until the last release. */
    xy_os_mutex_attr_t attr = { .attr_bits = XY_OS_MUTEX_RECURSIVE, .name = "rec" };
    xy_os_mutex_id_t r = xy_os_mutex_new(&attr);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_acquire(r, 0U));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_acquire(r, 0U));
    TEST_ASSERT_EQUAL_PTR(xy_os_thread_get_id(), xy_os_mutex_get_owner(r));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_release(r));
    TEST_ASSERT_EQUAL_PTR(xy_os_thread_get_id(), xy_os_mutex_get_owner(r));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mutex_release(r));
    TEST_ASSERT_NULL(xy_os_mutex_get_owner(r));

    xy_os_mutex_delete(r);
    xy_os_mutex_delete(m);
}

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

static void test_mempool_static_cb_and_storage(void)
{
    static uint8_t cb_mem[256];
    static uint8_t mp_mem[3 * 24];
    xy_os_mempool_attr_t attr = {
        .name = "extMp",
        .cb_mem = cb_mem,
        .cb_size = sizeof(cb_mem),
        .mp_mem = mp_mem,
        .mp_size = sizeof(mp_mem),
    };

    xy_os_mempool_id_t mp = xy_os_mempool_new(3, 24, &attr);
    TEST_ASSERT_EQUAL_PTR((xy_os_mempool_id_t)cb_mem, mp);
    TEST_ASSERT_EQUAL_STRING("extMp", xy_os_mempool_get_name(mp));
    TEST_ASSERT_EQUAL_UINT32(3U, xy_os_mempool_get_capacity(mp));
    TEST_ASSERT_EQUAL_UINT32(24U, xy_os_mempool_get_block_size(mp));

    uint8_t *blk = (uint8_t *)xy_os_mempool_alloc(mp, 0);
    TEST_ASSERT_NOT_NULL(blk);
    uintptr_t blk_addr = (uintptr_t)blk;
    uintptr_t mp_start = (uintptr_t)mp_mem;
    uintptr_t mp_end = (uintptr_t)(mp_mem + sizeof(mp_mem));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT64((uint64_t)mp_start, (uint64_t)blk_addr);
    TEST_ASSERT_LESS_THAN_UINT64((uint64_t)mp_end, (uint64_t)blk_addr);
    memset(blk, 0x5A, 24);
    TEST_ASSERT_EQUAL_HEX8(0x5A, blk[0]);
    TEST_ASSERT_EQUAL_HEX8(0x5A, blk[23]);
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mempool_free(mp, blk));
    TEST_ASSERT_EQUAL(XY_OS_OK, xy_os_mempool_delete(mp));
}

/* ========== main ========== */

int main(void)
{
    UNITY_BEGIN();
    xy_os_kernel_init();

    /* Kernel / Thread / Timer */
    RUN_TEST(test_kernel_info_lock_and_ticks);
    RUN_TEST(test_thread_stub_contract);
    RUN_TEST(test_thread_operations_this_backend_cannot_do);
    RUN_TEST(test_timer_handle_misuse_is_a_parameter_error);
    RUN_TEST(test_start_and_stop_does_not_exhaust_the_timer_pool);
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
    RUN_TEST(test_event_flags_reserve_the_error_bit_and_refuse_empty_masks);
    RUN_TEST(test_event_flags_set_get);
    RUN_TEST(test_event_flags_wait_any);
    RUN_TEST(test_event_flags_wait_all);
    RUN_TEST(test_event_flags_no_clear);

    /* Message Queue */
    RUN_TEST(test_msgqueue_dimensions_that_overflow_are_refused);
    RUN_TEST(test_msgqueue_put_get);
    RUN_TEST(test_msgqueue_fifo_order);
    RUN_TEST(test_msgqueue_full);
    RUN_TEST(test_msgqueue_empty_get);
    RUN_TEST(test_msgqueue_reset);
    RUN_TEST(test_msgqueue_binary_fixed_size_message);
    RUN_TEST(test_msgqueue_priority_is_refused_contract);
    RUN_TEST(test_msgqueue_static_cb_and_storage);

    /* Memory Pool */
    RUN_TEST(test_mempool_free_refuses_a_pointer_it_never_handed_out);
    RUN_TEST(test_mempool_double_free_is_refused);
    RUN_TEST(test_mempool_blocks_are_pointer_aligned);
    RUN_TEST(test_mempool_dimensions_that_overflow_are_refused);
    RUN_TEST(test_thread_queries_answer_about_the_id_they_were_given);
    RUN_TEST(test_mutex_reports_its_owner);
    RUN_TEST(test_mempool_alloc_free);
    RUN_TEST(test_mempool_exhaust);
    RUN_TEST(test_mempool_cycle);
    RUN_TEST(test_mempool_write_to_block);
    RUN_TEST(test_mempool_static_cb_and_storage);

    return UNITY_END();
}
