#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "unity.h"
#include "xy_timer.h"
#include "xy_st.h"

static int g_enter_count;
static int g_exit_count;
static int g_tick_callback_count;
static int g_periodic_count;
static int g_state_process_count;
static int g_timeout_entry_count;
static int g_timeout_process_count;
static int g_timeout_exit_count;
static int g_callback_param_value;
static xy_timer_ref g_callback_timer;

extern volatile uint32_t g_xy_tick;

void setUp(void)
{
}

void tearDown(void)
{
}

void xy_enter_critical(void) {}
void xy_exit_critical(void) {}

void *xy_mem_malloc(size_t size)
{
    return calloc(1, size);
}

void xy_mem_free(void *ptr)
{
    free(ptr);
}

static void once_timer_cb(xy_timer_ref timer, void *params)
{
    if (params != NULL) {
        g_callback_param_value = *(int *)params;
    }
    g_tick_callback_count++;
    g_callback_timer = timer;
}

static void periodic_timer_cb(xy_timer_ref timer, void *params)
{
    (void)timer;
    (void)params;
    g_periodic_count++;
}

static void self_kill_timer_cb(xy_timer_ref timer, void *params)
{
    (void)params;
    g_tick_callback_count++;
    xy_timer_kill(timer);
}

static void entry_cb(xy_sm_t *self)
{
    (void)self;
    g_enter_count++;
}

static void process_cb(xy_sm_t *self)
{
    (void)self;
    g_state_process_count++;
}

static void exit_cb(xy_sm_t *self)
{
    (void)self;
    g_exit_count++;
}

static void timeout_entry_cb(xy_sm_t *self)
{
    (void)self;
    g_timeout_entry_count++;
}

static void timeout_process_cb(xy_sm_t *self)
{
    (void)self;
    g_timeout_process_count++;
}

static void timeout_exit_cb(xy_sm_t *self)
{
    (void)self;
    g_timeout_exit_count++;
}

static void test_timer_lifecycle_and_ordering(void)
{
    xy_timer_init();
    xy_timer_set_tick(0);
    TEST_ASSERT_EQUAL_UINT32(0U, xy_timer_get_tick());
    TEST_ASSERT_EQUAL_UINT32(0U, xy_timer_get_tick_from_isr());
    TEST_ASSERT_EQUAL_UINT32(0U, xy_timer_get_nexttick());
    TEST_ASSERT_NULL(xy_timer_create(1, 0, NULL, NULL));

    g_tick_callback_count = 0;
    g_periodic_count = 0;
    g_callback_param_value = 0;
    g_callback_timer = NULL;

    int callback_param = 42;

    xy_timer_ref once = xy_timer_create(3, 0, once_timer_cb, &callback_param);
    xy_timer_ref periodic = xy_timer_create(5, 2, periodic_timer_cb, NULL);
    TEST_ASSERT_NOT_NULL(once);
    TEST_ASSERT_NOT_NULL(periodic);
    TEST_ASSERT_EQUAL_UINT32(3U, xy_timer_get_nexttick());
    TEST_ASSERT_EQUAL_PTR(once_timer_cb, xy_timer_get_func(once));

    g_xy_tick = 2;
    xy_timer_ticks();
    TEST_ASSERT_EQUAL_INT(0, g_tick_callback_count);
    TEST_ASSERT_EQUAL_INT(0, g_periodic_count);
    TEST_ASSERT_EQUAL_UINT32(1U, xy_timer_get_nexttick());

    g_xy_tick = 3;
    xy_timer_ticks();
    TEST_ASSERT_EQUAL_INT(1, g_tick_callback_count);
    TEST_ASSERT_EQUAL_INT(42, g_callback_param_value);
    TEST_ASSERT_EQUAL_PTR(once, g_callback_timer);

    g_xy_tick = 5;
    xy_timer_ticks();
    TEST_ASSERT_EQUAL_INT(1, g_periodic_count);

    g_xy_tick = 7;
    xy_timer_ticks();
    TEST_ASSERT_EQUAL_INT(2, g_periodic_count);

    xy_timer_kill(periodic);
}

static void test_timer_change_and_self_kill(void)
{
    xy_timer_init();
    xy_timer_set_tick(0);
    g_tick_callback_count = 0;

    xy_timer_ref timer = xy_timer_create(10, 0, once_timer_cb, NULL);
    TEST_ASSERT_NOT_NULL(timer);
    xy_timer_change_cnt(timer, 2);
    xy_timer_change_func(timer, self_kill_timer_cb);
    xy_timer_change_reload(timer, 0);
    TEST_ASSERT_EQUAL_PTR(self_kill_timer_cb, xy_timer_get_func(timer));

    g_xy_tick = 2;
    xy_timer_ticks();
    TEST_ASSERT_EQUAL_INT(1, g_tick_callback_count);
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

    g_enter_count = 0;
    g_exit_count = 0;
    g_state_process_count = 0;
    g_timeout_entry_count = 0;
    g_timeout_process_count = 0;
    g_timeout_exit_count = 0;

    xy_sm_init(&sm);
    TEST_ASSERT_NULL(xy_sm_get_process(&sm));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_sm_get_timeout_remain(&sm));
    TEST_ASSERT_FALSE(xy_sm_is_timeout_active(&sm));

    xy_sm_transition(&sm, entry_cb, process_cb, exit_cb);
    TEST_ASSERT_EQUAL_INT(1, g_enter_count);
    TEST_ASSERT_EQUAL_PTR(process_cb, xy_sm_get_process(&sm));

    xy_sm_process_sample(&sm, 1);
    TEST_ASSERT_EQUAL_INT(1, g_state_process_count);

    xy_sm_transition_timeout(&sm, entry_cb, process_cb, exit_cb,
                             timeout_entry_cb, timeout_process_cb, timeout_exit_cb, 5);
    TEST_ASSERT_EQUAL_INT(1, g_exit_count);
    TEST_ASSERT_EQUAL_INT(2, g_enter_count);
    TEST_ASSERT_TRUE(xy_sm_is_timeout_active(&sm));
    TEST_ASSERT_EQUAL_UINT32(5U, xy_sm_get_timeout_remain(&sm));

    xy_sm_process_sample(&sm, 2);
    TEST_ASSERT_EQUAL_INT(2, g_state_process_count);
    TEST_ASSERT_EQUAL_UINT32(3U, xy_sm_get_timeout_remain(&sm));
    xy_sm_reset_timeout(&sm);
    TEST_ASSERT_EQUAL_UINT32(5U, xy_sm_get_timeout_remain(&sm));

    xy_sm_process_sample(&sm, 5);
    TEST_ASSERT_EQUAL_INT(1, g_timeout_entry_count);
    TEST_ASSERT_EQUAL_PTR(timeout_process_cb, xy_sm_get_process(&sm));
    TEST_ASSERT_FALSE(xy_sm_is_timeout_active(&sm));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_sm_get_timeout_remain(&sm));
    xy_sm_reset_timeout(&sm);
    TEST_ASSERT_FALSE(xy_sm_is_timeout_active(&sm));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_sm_get_timeout_remain(&sm));

    xy_sm_process_sample(&sm, 1);
    TEST_ASSERT_EQUAL_INT(1, g_timeout_process_count);
    xy_sm_transition(&sm, entry_cb, process_cb, exit_cb);
    TEST_ASSERT_EQUAL_INT(1, g_timeout_exit_count);
    TEST_ASSERT_EQUAL_INT(3, g_enter_count);

    xy_sm_transition_delay(&sm, timeout_entry_cb, timeout_process_cb, timeout_exit_cb, 4);
    TEST_ASSERT_TRUE(xy_sm_is_timeout_active(&sm));
    TEST_ASSERT_EQUAL_PTR(process_cb, xy_sm_get_process(&sm));
    xy_sm_process_sample(&sm, 2);
    TEST_ASSERT_EQUAL_INT(4, g_state_process_count);
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
