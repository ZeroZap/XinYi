#include "xy_autotask.h"
#include "xy_os.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"

static uint32_t fake_tick;
static int task_result;
static int task_call_count;
static int complete_call_count;
static int complete_result;
static void *complete_arg;

uint32_t xy_os_tick_get(void)
{
    return fake_tick;
}

static int capture_task(void *arg)
{
    task_call_count++;
    TEST_ASSERT_EQUAL_PTR((void *)0xCAFE, arg);
    return task_result;
}

static void capture_complete(int result, void *arg)
{
    complete_call_count++;
    complete_result = result;
    complete_arg = arg;
}

void setUp(void)
{
    fake_tick = 1000U;
    task_result = XY_AUTOTASK_OK;
    task_call_count = 0;
    complete_call_count = 0;
    complete_result = 0;
    complete_arg = NULL;
}

void tearDown(void)
{
}

static void test_autotask_init_defaults_and_config_overrides(void)
{
    xy_autotask_scheduler_t scheduler;
    xy_autotask_config_t config = {
        .idle_timeout_ms = 250U,
        .default_type = XY_AUTOTASK_TYPE_CLEANUP,
        .auto_start = false,
        .max_run_time_ms = 500U,
    };

    memset(&scheduler, 0xA5, sizeof(scheduler));

    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_init(NULL, NULL));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_init(&scheduler, &config));
    TEST_ASSERT_TRUE(scheduler.initialized);
    TEST_ASSERT_TRUE(scheduler.activity_monitoring);
    TEST_ASSERT_EQUAL(XY_AUTOTASK_STATE_IDLE, scheduler.state);
    TEST_ASSERT_EQUAL_UINT32(250U, scheduler.config.idle_timeout_ms);
    TEST_ASSERT_EQUAL(XY_AUTOTASK_TYPE_CLEANUP, scheduler.config.default_type);
    TEST_ASSERT_FALSE(scheduler.config.auto_start);
    TEST_ASSERT_EQUAL_UINT32(500U, scheduler.config.max_run_time_ms);
    TEST_ASSERT_EQUAL_UINT32(fake_tick, scheduler.last_activity);

    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_start(&scheduler));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_STATE_IDLE, scheduler.state);
    TEST_ASSERT_EQUAL_UINT32(fake_tick, scheduler.last_activity);

    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_deinit(&scheduler));
    TEST_ASSERT_FALSE(scheduler.initialized);
    TEST_ASSERT_EQUAL(XY_AUTOTASK_STATE_STOPPED, scheduler.state);
    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_deinit(NULL));
}

static void test_autotask_trigger_runs_callback_and_completion(void)
{
    xy_autotask_scheduler_t scheduler;
    xy_autotask_config_t config = {
        .idle_timeout_ms = 1000U,
        .default_type = XY_AUTOTASK_TYPE_CUSTOM,
        .auto_start = false,
        .max_run_time_ms = 2000U,
    };
    uint32_t run_count = 0xAAAAAAAAU;
    uint32_t stop_count = 0xBBBBBBBBU;

    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_init(&scheduler, &config));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK,
                      xy_autotask_register_callback(&scheduler, capture_task,
                                                    (void *)0xCAFE));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK,
                      xy_autotask_register_complete_callback(&scheduler,
                                                             capture_complete));

    task_result = 17;
    fake_tick = 1234U;
    TEST_ASSERT_EQUAL(17, xy_autotask_trigger(&scheduler));
    TEST_ASSERT_EQUAL_INT(1, task_call_count);
    TEST_ASSERT_EQUAL_INT(1, complete_call_count);
    TEST_ASSERT_EQUAL_INT(17, complete_result);
    TEST_ASSERT_EQUAL_PTR((void *)0xCAFE, complete_arg);
    TEST_ASSERT_EQUAL(XY_AUTOTASK_STATE_IDLE, scheduler.state);
    TEST_ASSERT_EQUAL_UINT32(1U, scheduler.run_count);
    TEST_ASSERT_EQUAL_UINT32(fake_tick, scheduler.last_activity);
    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_get_stats(&scheduler, &run_count, &stop_count));
    TEST_ASSERT_EQUAL_UINT32(1U, run_count);
    TEST_ASSERT_EQUAL_UINT32(0U, stop_count);

    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_trigger(NULL));
    scheduler.initialized = false;
    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_trigger(&scheduler));
}

static void test_autotask_scheduler_loop_idle_timeout_and_paused_guards(void)
{
    xy_autotask_scheduler_t scheduler;
    xy_autotask_config_t config = {
        .idle_timeout_ms = 50U,
        .default_type = XY_AUTOTASK_TYPE_CUSTOM,
        .auto_start = false,
        .max_run_time_ms = 500U,
    };

    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_init(&scheduler, &config));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK,
                      xy_autotask_register_callback(&scheduler, capture_task,
                                                    (void *)0xCAFE));

    fake_tick = scheduler.last_activity + 49U;
    xy_autotask_scheduler_loop(&scheduler);
    TEST_ASSERT_EQUAL_INT(0, task_call_count);
    TEST_ASSERT_EQUAL_UINT32(0U, scheduler.run_count);

    fake_tick = scheduler.last_activity + 50U;
    xy_autotask_scheduler_loop(&scheduler);
    TEST_ASSERT_EQUAL_INT(1, task_call_count);
    TEST_ASSERT_EQUAL_UINT32(1U, scheduler.run_count);
    TEST_ASSERT_EQUAL(XY_AUTOTASK_STATE_IDLE, scheduler.state);

    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_pause(&scheduler));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_STATE_IDLE, scheduler.state);
    scheduler.state = XY_AUTOTASK_STATE_RUNNING;
    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_pause(&scheduler));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_STATE_PAUSED, scheduler.state);
    xy_autotask_scheduler_loop(&scheduler);
    TEST_ASSERT_EQUAL_INT(1, task_call_count);

    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_resume(&scheduler));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_STATE_IDLE, scheduler.state);
    TEST_ASSERT_EQUAL_UINT32(fake_tick, scheduler.last_activity);
}

static void test_autotask_idle_and_remaining_time_wraparound(void)
{
    xy_autotask_scheduler_t scheduler;
    xy_autotask_config_t config = {
        .idle_timeout_ms = 100U,
        .default_type = XY_AUTOTASK_TYPE_TODO,
        .auto_start = false,
        .max_run_time_ms = 1000U,
    };

    fake_tick = UINT32_MAX - 5U;
    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_init(&scheduler, &config));
    fake_tick = 9U;
    TEST_ASSERT_EQUAL_UINT32(14U, xy_autotask_get_idle_time(&scheduler));

    scheduler.state = XY_AUTOTASK_STATE_RUNNING;
    scheduler.start_time = 20U;
    fake_tick = 120U;
    TEST_ASSERT_EQUAL_UINT32(900U, xy_autotask_get_remaining_time(&scheduler));
    fake_tick = 1200U;
    TEST_ASSERT_EQUAL_UINT32(0U, xy_autotask_get_remaining_time(&scheduler));

    scheduler.state = XY_AUTOTASK_STATE_IDLE;
    TEST_ASSERT_EQUAL_UINT32(0U, xy_autotask_get_remaining_time(&scheduler));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_autotask_get_idle_time(NULL));
}

static void test_autotask_mutators_validate_inputs_and_preserve_state(void)
{
    xy_autotask_scheduler_t scheduler;
    xy_autotask_config_t config = {
        .idle_timeout_ms = 100U,
        .default_type = XY_AUTOTASK_TYPE_TODO,
        .auto_start = false,
        .max_run_time_ms = 1000U,
    };

    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_init(&scheduler, &config));

    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_record_activity(NULL));
    scheduler.initialized = false;
    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_record_activity(&scheduler));
    scheduler.initialized = true;
    fake_tick = 4321U;
    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_record_activity(&scheduler));
    TEST_ASSERT_EQUAL_UINT32(4321U, scheduler.last_activity);

    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_set_task_type(NULL, XY_AUTOTASK_TYPE_LEARN));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_set_task_type(&scheduler, XY_AUTOTASK_TYPE_LEARN));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_TYPE_LEARN, scheduler.config.default_type);

    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_set_idle_timeout(NULL, 1U));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_set_idle_timeout(&scheduler, 0U));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_set_idle_timeout(&scheduler, 25U));
    TEST_ASSERT_EQUAL_UINT32(25U, scheduler.config.idle_timeout_ms);

    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_register_callback(NULL, capture_task, NULL));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_register_callback(&scheduler, NULL, NULL));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM,
                      xy_autotask_register_complete_callback(NULL, capture_complete));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_OK, xy_autotask_enable_monitoring(&scheduler, false));
    TEST_ASSERT_FALSE(scheduler.activity_monitoring);
    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_enable_monitoring(NULL, true));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_stop(NULL));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_STATE_STOPPED, xy_autotask_get_state(NULL));
    TEST_ASSERT_EQUAL(XY_AUTOTASK_INVALID_PARAM, xy_autotask_get_stats(NULL, NULL, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_autotask_init_defaults_and_config_overrides);
    RUN_TEST(test_autotask_trigger_runs_callback_and_completion);
    RUN_TEST(test_autotask_scheduler_loop_idle_timeout_and_paused_guards);
    RUN_TEST(test_autotask_idle_and_remaining_time_wraparound);
    RUN_TEST(test_autotask_mutators_validate_inputs_and_preserve_state);
    return UNITY_END();
}
