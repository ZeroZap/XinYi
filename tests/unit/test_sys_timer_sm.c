#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

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
    assert(xy_timer_get_tick() == 0);
    assert(xy_timer_get_tick_from_isr() == 0);
    assert(xy_timer_get_nexttick() == 0);
    assert(xy_timer_create(1, 0, NULL, NULL) == NULL);

    g_tick_callback_count = 0;
    g_periodic_count = 0;
    g_callback_param_value = 0;
    g_callback_timer = NULL;

    int callback_param = 42;

    xy_timer_ref once = xy_timer_create(3, 0, once_timer_cb, &callback_param);
    xy_timer_ref periodic = xy_timer_create(5, 2, periodic_timer_cb, NULL);
    assert(once != NULL);
    assert(periodic != NULL);
    assert(xy_timer_get_nexttick() == 3);
    assert(xy_timer_get_func(once) == once_timer_cb);

    g_xy_tick = 2;
    xy_timer_ticks();
    assert(g_tick_callback_count == 0);
    assert(g_periodic_count == 0);
    assert(xy_timer_get_nexttick() == 1);

    g_xy_tick = 3;
    xy_timer_ticks();
    assert(g_tick_callback_count == 1);
    assert(g_callback_param_value == 42);
    assert(g_callback_timer == once);

    g_xy_tick = 5;
    xy_timer_ticks();
    assert(g_periodic_count == 1);

    g_xy_tick = 7;
    xy_timer_ticks();
    assert(g_periodic_count == 2);

    xy_timer_kill(periodic);
}

static void test_timer_change_and_self_kill(void)
{
    xy_timer_init();
    xy_timer_set_tick(0);
    g_tick_callback_count = 0;

    xy_timer_ref t = xy_timer_create(10, 0, once_timer_cb, NULL);
    assert(t != NULL);
    xy_timer_change_cnt(t, 2);
    xy_timer_change_func(t, self_kill_timer_cb);
    xy_timer_change_reload(t, 0);
    assert(xy_timer_get_func(t) == self_kill_timer_cb);

    g_xy_tick = 2;
    xy_timer_ticks();
    assert(g_tick_callback_count == 1);
    assert(xy_timer_get_nexttick() == 0);
}

static void test_state_machine_transitions_and_timeout(void)
{
    xy_sm_t sm;
    assert(xy_sm_get_process(NULL) == NULL);
    assert(xy_sm_get_timeout_remain(NULL) == 0);
    assert(!xy_sm_is_timeout_active(NULL));
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
    assert(xy_sm_get_process(&sm) == NULL);
    assert(xy_sm_get_timeout_remain(&sm) == 0);
    assert(!xy_sm_is_timeout_active(&sm));

    xy_sm_transition(&sm, entry_cb, process_cb, exit_cb);
    assert(g_enter_count == 1);
    assert(xy_sm_get_process(&sm) == process_cb);

    xy_sm_process_sample(&sm, 1);
    assert(g_state_process_count == 1);

    xy_sm_transition_timeout(&sm, entry_cb, process_cb, exit_cb,
                             timeout_entry_cb, timeout_process_cb, timeout_exit_cb, 5);
    assert(g_exit_count == 1);
    assert(g_enter_count == 2);
    assert(xy_sm_is_timeout_active(&sm));
    assert(xy_sm_get_timeout_remain(&sm) == 5);

    xy_sm_process_sample(&sm, 2);
    assert(g_state_process_count == 2);
    assert(xy_sm_get_timeout_remain(&sm) == 3);
    xy_sm_reset_timeout(&sm);
    assert(xy_sm_get_timeout_remain(&sm) == 5);

    xy_sm_process_sample(&sm, 5);
    assert(g_timeout_entry_count == 1);
    assert(xy_sm_get_process(&sm) == timeout_process_cb);
    assert(!xy_sm_is_timeout_active(&sm));
    assert(xy_sm_get_timeout_remain(&sm) == 0);
    xy_sm_reset_timeout(&sm);
    assert(!xy_sm_is_timeout_active(&sm));
    assert(xy_sm_get_timeout_remain(&sm) == 0);

    xy_sm_process_sample(&sm, 1);
    assert(g_timeout_process_count == 1);
    xy_sm_transition(&sm, entry_cb, process_cb, exit_cb);
    assert(g_timeout_exit_count == 1);
    assert(g_enter_count == 3);

    xy_sm_transition_delay(&sm, timeout_entry_cb, timeout_process_cb, timeout_exit_cb, 4);
    assert(xy_sm_is_timeout_active(&sm));
    assert(xy_sm_get_process(&sm) == process_cb);
    xy_sm_process_sample(&sm, 2);
    assert(g_state_process_count == 4);
    assert(xy_sm_get_timeout_remain(&sm) == 2);
    xy_sm_cancel_timeout(&sm);
    assert(!xy_sm_is_timeout_active(&sm));
}

int main(void)
{
    test_timer_lifecycle_and_ordering();
    test_timer_change_and_self_kill();
    test_state_machine_transitions_and_timeout();
    return 0;
}
