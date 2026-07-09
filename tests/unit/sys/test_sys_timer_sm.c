#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "unity.h"
#include "fff.h"
#include "xy_timer.h"
#include "xy_st.h"

static int g_callback_param_value;
static xy_timer_ref g_callback_timer;

extern volatile uint32_t g_xy_tick;

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(xy_enter_critical)
FAKE_VOID_FUNC(xy_exit_critical)
FAKE_VOID_FUNC(once_timer_cb, xy_timer_ref, void *)
FAKE_VOID_FUNC(periodic_timer_cb, xy_timer_ref, void *)
FAKE_VOID_FUNC(self_kill_timer_cb, xy_timer_ref, void *)
FAKE_VOID_FUNC(entry_cb, xy_sm_t *)
FAKE_VOID_FUNC(process_cb, xy_sm_t *)
FAKE_VOID_FUNC(exit_cb, xy_sm_t *)
FAKE_VOID_FUNC(timeout_entry_cb, xy_sm_t *)
FAKE_VOID_FUNC(timeout_process_cb, xy_sm_t *)
FAKE_VOID_FUNC(timeout_exit_cb, xy_sm_t *)

void *xy_mem_malloc(size_t size)
{
    return calloc(1, size);
}

void xy_mem_free(void *ptr)
{
    free(ptr);
}

static void once_timer_cb_impl(xy_timer_ref timer, void *params)
{
    if (params != NULL) {
        g_callback_param_value = *(int *)params;
    }
    g_callback_timer = timer;
}

static void self_kill_timer_cb_impl(xy_timer_ref timer, void *params)
{
    (void)params;
    xy_timer_kill(timer);
}

void setUp(void)
{
    RESET_FAKE(xy_enter_critical);
    RESET_FAKE(xy_exit_critical);
    RESET_FAKE(once_timer_cb);
    RESET_FAKE(periodic_timer_cb);
    RESET_FAKE(self_kill_timer_cb);
    RESET_FAKE(entry_cb);
    RESET_FAKE(process_cb);
    RESET_FAKE(exit_cb);
    RESET_FAKE(timeout_entry_cb);
    RESET_FAKE(timeout_process_cb);
    RESET_FAKE(timeout_exit_cb);
    FFF_RESET_HISTORY();

    once_timer_cb_fake.custom_fake = once_timer_cb_impl;
    self_kill_timer_cb_fake.custom_fake = self_kill_timer_cb_impl;

    g_callback_param_value = 0;
    g_callback_timer = NULL;
}

void tearDown(void)
{
}

static void test_timer_lifecycle_and_ordering(void)
{
    xy_timer_init();
    xy_timer_set_tick(0);
    TEST_ASSERT_EQUAL_UINT32(0U, xy_timer_get_tick());
    TEST_ASSERT_EQUAL_UINT32(0U, xy_timer_get_tick_from_isr());
    TEST_ASSERT_EQUAL_UINT32(0U, xy_timer_get_nexttick());
    TEST_ASSERT_NULL(xy_timer_create(1, 0, NULL, NULL));

    int callback_param = 42;

    xy_timer_ref once = xy_timer_create(3, 0, once_timer_cb, &callback_param);
    xy_timer_ref periodic = xy_timer_create(5, 2, periodic_timer_cb, NULL);
    TEST_ASSERT_NOT_NULL(once);
    TEST_ASSERT_NOT_NULL(periodic);
    TEST_ASSERT_EQUAL_UINT32(3U, xy_timer_get_nexttick());
    TEST_ASSERT_EQUAL_PTR(once_timer_cb, xy_timer_get_func(once));

    g_xy_tick = 2;
    xy_timer_ticks();
    TEST_ASSERT_EQUAL_UINT(0U, once_timer_cb_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, periodic_timer_cb_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(1U, xy_timer_get_nexttick());

    g_xy_tick = 3;
    xy_timer_ticks();
    TEST_ASSERT_EQUAL_UINT(1U, once_timer_cb_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(once, once_timer_cb_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&callback_param, once_timer_cb_fake.arg1_val);
    TEST_ASSERT_EQUAL_INT(42, g_callback_param_value);
    TEST_ASSERT_EQUAL_PTR(once, g_callback_timer);

    g_xy_tick = 5;
    xy_timer_ticks();
    TEST_ASSERT_EQUAL_UINT(1U, periodic_timer_cb_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(periodic, periodic_timer_cb_fake.arg0_val);
    TEST_ASSERT_NULL(periodic_timer_cb_fake.arg1_val);

    g_xy_tick = 7;
    xy_timer_ticks();
    TEST_ASSERT_EQUAL_UINT(2U, periodic_timer_cb_fake.call_count);

    xy_timer_kill(periodic);
    TEST_ASSERT_GREATER_THAN_UINT(0U, xy_enter_critical_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(xy_enter_critical_fake.call_count, xy_exit_critical_fake.call_count);
}

static void test_timer_change_and_self_kill(void)
{
    xy_timer_init();
    xy_timer_set_tick(0);

    xy_timer_ref timer = xy_timer_create(10, 0, once_timer_cb, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    xy_timer_change_cnt(timer, 2);
    xy_timer_change_func(timer, self_kill_timer_cb);
    xy_timer_change_reload(timer, 0);
    TEST_ASSERT_EQUAL_PTR(self_kill_timer_cb, xy_timer_get_func(timer));

    g_xy_tick = 2;
    xy_timer_ticks();
    TEST_ASSERT_EQUAL_UINT(1U, self_kill_timer_cb_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(timer, self_kill_timer_cb_fake.arg0_val);
    TEST_ASSERT_NULL(self_kill_timer_cb_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(0U, xy_timer_get_nexttick());
}

static void test_state_machine_transitions_and_timeout(void)
{
    xy_sm_t sm;
    TEST_ASSERT_NULL(xy_sm_get_process(NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_sm_get_timeout_remain(NULL));
    TEST_ASSERT_FALSE(xy_sm_is_timeout_active(NULL));
    xy_sm_init(NULL);
    xy_sm_transition(NULL, entry_cb, process_cb, exit_cb);
    xy_sm_transition_timeout(NULL, entry_cb, process_cb, exit_cb,
                             timeout_entry_cb, timeout_process_cb, timeout_exit_cb, 1);
    xy_sm_transition_delay(NULL, timeout_entry_cb, timeout_process_cb, timeout_exit_cb, 1);
    xy_sm_process_sample(NULL, 1);
    xy_sm_cancel_timeout(NULL);
    xy_sm_reset_timeout(NULL);

    xy_sm_init(&sm);
    TEST_ASSERT_NULL(xy_sm_get_process(&sm));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_sm_get_timeout_remain(&sm));
    TEST_ASSERT_FALSE(xy_sm_is_timeout_active(&sm));

    xy_sm_transition(&sm, entry_cb, process_cb, exit_cb);
    TEST_ASSERT_EQUAL_UINT(1U, entry_cb_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sm, entry_cb_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(process_cb, xy_sm_get_process(&sm));

    xy_sm_process_sample(&sm, 1);
    TEST_ASSERT_EQUAL_UINT(1U, process_cb_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sm, process_cb_fake.arg0_val);

    xy_sm_transition_timeout(&sm, entry_cb, process_cb, exit_cb,
                             timeout_entry_cb, timeout_process_cb, timeout_exit_cb, 5);
    TEST_ASSERT_EQUAL_UINT(1U, exit_cb_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sm, exit_cb_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(2U, entry_cb_fake.call_count);
    TEST_ASSERT_TRUE(xy_sm_is_timeout_active(&sm));
    TEST_ASSERT_EQUAL_UINT32(5U, xy_sm_get_timeout_remain(&sm));

    xy_sm_process_sample(&sm, 2);
    TEST_ASSERT_EQUAL_UINT(2U, process_cb_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(3U, xy_sm_get_timeout_remain(&sm));
    xy_sm_reset_timeout(&sm);
    TEST_ASSERT_EQUAL_UINT32(5U, xy_sm_get_timeout_remain(&sm));

    xy_sm_process_sample(&sm, 5);
    TEST_ASSERT_EQUAL_UINT(1U, timeout_entry_cb_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sm, timeout_entry_cb_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(timeout_process_cb, xy_sm_get_process(&sm));
    TEST_ASSERT_FALSE(xy_sm_is_timeout_active(&sm));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_sm_get_timeout_remain(&sm));
    xy_sm_reset_timeout(&sm);
    TEST_ASSERT_FALSE(xy_sm_is_timeout_active(&sm));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_sm_get_timeout_remain(&sm));

    xy_sm_process_sample(&sm, 1);
    TEST_ASSERT_EQUAL_UINT(1U, timeout_process_cb_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sm, timeout_process_cb_fake.arg0_val);
    xy_sm_transition(&sm, entry_cb, process_cb, exit_cb);
    TEST_ASSERT_EQUAL_UINT(1U, timeout_exit_cb_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sm, timeout_exit_cb_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(3U, entry_cb_fake.call_count);

    xy_sm_transition_delay(&sm, timeout_entry_cb, timeout_process_cb, timeout_exit_cb, 4);
    TEST_ASSERT_TRUE(xy_sm_is_timeout_active(&sm));
    TEST_ASSERT_EQUAL_PTR(process_cb, xy_sm_get_process(&sm));
    xy_sm_process_sample(&sm, 2);
    TEST_ASSERT_EQUAL_UINT(4U, process_cb_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(2U, xy_sm_get_timeout_remain(&sm));
    xy_sm_cancel_timeout(&sm);
    TEST_ASSERT_FALSE(xy_sm_is_timeout_active(&sm));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_timer_lifecycle_and_ordering);
    RUN_TEST(test_timer_change_and_self_kill);
    RUN_TEST(test_state_machine_transitions_and_timeout);
    return UNITY_END();
}
