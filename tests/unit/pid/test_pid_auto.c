#include <math.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "xy_pid_auto.h"

static uint32_t g_tick_ms;

uint32_t xy_os_tick_get(void)
{
    return g_tick_ms;
}

void xy_log_char(char ch)
{
    (void)ch;
}

static xy_pid_config_t default_pid_config(void)
{
    xy_pid_config_t config = {
        .kp = 1.0F,
        .ki = 0.1F,
        .kd = 0.01F,
        .output_min = -100.0F,
        .output_max = 100.0F,
        .integral_min = -10.0F,
        .integral_max = 10.0F,
        .derivative_filter = 0.1F,
    };
    return config;
}

static xy_pid_auto_config_t default_auto_config(void)
{
    xy_pid_auto_config_t config = {
        .method = XY_PID_AUTO_METHOD_ZN,
        .step_amplitude = 10.0F,
        .sample_interval_ms = 5U,
        .num_samples = 4U,
        .tolerance = 0.01F,
    };
    return config;
}

static void test_auto_init_defaults_and_deinit(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_config_t pid_config = default_pid_config();
    xy_pid_auto_config_t config = default_auto_config();

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_init(NULL, &pid, NULL));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_init(&tuner, NULL, NULL));

    config.method = (xy_pid_auto_method_t)-1;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_init(&tuner, &pid, &config));

    config.method = (xy_pid_auto_method_t)99;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_init(&tuner, &pid, &config));

    config = default_auto_config();
    config.tolerance = -0.1F;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_init(&tuner, &pid, &config));

    config = default_auto_config();
    config.step_amplitude = NAN;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_init(&tuner, &pid, &config));

    config = default_auto_config();
    config.tolerance = INFINITY;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_init(&tuner, &pid, &config));

    config = default_auto_config();
    config.num_samples = 1U;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_init(&tuner, &pid, &config));

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_init(&tuner, &pid, NULL));
    TEST_ASSERT_TRUE(tuner.initialized);
    TEST_ASSERT_TRUE(tuner.config.tolerance > 0.0F);
    TEST_ASSERT_EQUAL_PTR(&pid, tuner.pid);
    TEST_ASSERT_EQUAL(XY_PID_AUTO_STATE_IDLE, tuner.state);
    TEST_ASSERT_EQUAL(XY_PID_AUTO_METHOD_ZN, tuner.config.method);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 50.0F, tuner.config.step_amplitude);
    TEST_ASSERT_EQUAL_UINT16(100U, tuner.config.num_samples);
    TEST_ASSERT_NOT_NULL(tuner.samples);
    for (size_t i = 0; i < tuner.config.num_samples * sizeof(*tuner.samples); ++i) {
        TEST_ASSERT_EQUAL_HEX8(0U, ((uint8_t *)tuner.samples)[i]);
    }

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_deinit(&tuner));
    TEST_ASSERT_FALSE(tuner.initialized);
    TEST_ASSERT_NULL(tuner.samples);
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_deinit(NULL));
}

static void test_auto_init_accepts_zero_overrides_as_defaults(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_config_t pid_config = default_pid_config();
    xy_pid_auto_config_t config = default_auto_config();

    config.step_amplitude = 0.0F;
    config.sample_interval_ms = 0U;
    config.num_samples = 0U;

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_init(&tuner, &pid, &config));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 50.0F, tuner.config.step_amplitude);
    TEST_ASSERT_EQUAL_UINT32(100U, tuner.config.sample_interval_ms);
    TEST_ASSERT_EQUAL_UINT16(100U, tuner.config.num_samples);

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_deinit(&tuner));
}

static void test_auto_init_rejects_invalid_config_without_touching_tuner(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_config_t pid_config = default_pid_config();
    xy_pid_auto_config_t config = default_auto_config();

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));

    memset(&tuner, 0xA5, sizeof(tuner));
    config.step_amplitude = -5.0F;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_init(&tuner, &pid, &config));
    TEST_ASSERT_EQUAL_HEX8(0xA5U, ((uint8_t *)&tuner)[0]);

    config = default_auto_config();
    config.method = XY_PID_AUTO_METHOD_IMC;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_init(&tuner, &pid, &config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_METHOD_IMC, tuner.config.method);
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_deinit(&tuner));
}

static void test_auto_start_stop_and_progress_guards(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_config_t pid_config = default_pid_config();
    xy_pid_auto_config_t auto_config = default_auto_config();

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_init(&tuner, &pid, &auto_config));

    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, xy_pid_auto_get_progress(NULL));
    tuner.config.num_samples = 0U;
    tuner.sample_count = 3U;
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, xy_pid_auto_get_progress(&tuner));
    tuner.config.num_samples = auto_config.num_samples;
    tuner.sample_count = 0U;
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, xy_pid_auto_get_progress(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_NOT_READY, xy_pid_auto_loop(&tuner, 1.0F));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_STATE_ERROR, xy_pid_auto_get_state(NULL));

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_set_mode(&pid, XY_PID_MODE_AUTO));
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_set_setpoint(&pid, 42.0F));
    g_tick_ms = 100U;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_start(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_STATE_MEASURING, xy_pid_auto_get_state(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_MODE_MANUAL, xy_pid_get_mode(&pid));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, pid.setpoint);
    TEST_ASSERT_EQUAL_UINT32(100U, tuner.start_time);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, auto_config.step_amplitude, tuner.output_step);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, auto_config.step_amplitude, pid.output);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, pid_config.output_min, pid.config.output_min);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, pid_config.output_max, pid.config.output_max);

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_stop(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_STATE_IDLE, xy_pid_auto_get_state(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_stop(NULL));

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_deinit(&tuner));
}

static void test_auto_loop_completion_result_and_apply(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_auto_result_t result;
    xy_pid_config_t pid_config = default_pid_config();
    xy_pid_auto_config_t auto_config = default_auto_config();

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_init(&tuner, &pid, &auto_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_start(&tuner));

    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_get_result(NULL, &result));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_get_result(&tuner, NULL));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_NOT_READY, xy_pid_auto_get_result(&tuner, &result));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_loop(&tuner, 1.0F));
    TEST_ASSERT_EQUAL_UINT16(0U, tuner.sample_count);

    const float samples[] = {1.0F, 2.0F, 4.0F, 10.0F};
    for (uint16_t i = 0; i < auto_config.num_samples; ++i) {
        g_tick_ms += auto_config.sample_interval_ms;
        TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_loop(&tuner, samples[i]));
    }

    TEST_ASSERT_EQUAL(XY_PID_AUTO_STATE_COMPLETE, xy_pid_auto_get_state(&tuner));
    TEST_ASSERT_EQUAL_UINT16(auto_config.num_samples, tuner.sample_count);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 100.0F, xy_pid_auto_get_progress(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_get_result(&tuner, &result));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 3.12F, result.kp);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 10.0F, result.ki);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 2.5F, result.kd);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 10.0F, result.rise_time);

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_apply(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_MODE_AUTO, xy_pid_get_mode(&pid));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, result.kp, pid.config.kp);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, result.kp / result.ki, pid.config.ki);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, result.kp * result.kd, pid.config.kd);

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_deinit(&tuner));
}

static void test_auto_apply_requires_complete_state(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_config_t pid_config = default_pid_config();
    xy_pid_auto_config_t auto_config = default_auto_config();

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_init(&tuner, &pid, &auto_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_apply(NULL));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_NOT_READY, xy_pid_auto_apply(&tuner));

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_deinit(&tuner));
}

static void test_auto_zn_degenerate_flat_response_enters_error_state(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_auto_result_t result = {0};
    xy_pid_config_t pid_config = default_pid_config();
    xy_pid_auto_config_t auto_config = default_auto_config();

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_init(&tuner, &pid, &auto_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_start(&tuner));

    for (uint16_t i = 0; i < auto_config.num_samples - 1U; ++i) {
        g_tick_ms += auto_config.sample_interval_ms;
        TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_loop(&tuner, 0.0F));
    }
    g_tick_ms += auto_config.sample_interval_ms;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_ERROR, xy_pid_auto_loop(&tuner, 0.0F));

    TEST_ASSERT_EQUAL(XY_PID_AUTO_STATE_ERROR, xy_pid_auto_get_state(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_NOT_READY, xy_pid_auto_get_result(&tuner, &result));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, result.kp);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, result.ki);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, result.kd);

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_deinit(&tuner));
}

static void test_auto_loop_rejects_deinitialized_tuner_without_touching_freed_samples(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_config_t pid_config = default_pid_config();
    xy_pid_auto_config_t auto_config = default_auto_config();

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_init(&tuner, &pid, &auto_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_start(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_deinit(&tuner));

    tuner.state = XY_PID_AUTO_STATE_MEASURING;
    tuner.sample_count = 0U;
    g_tick_ms = auto_config.sample_interval_ms;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_loop(&tuner, 12.0F));
    TEST_ASSERT_EQUAL_UINT16(0U, tuner.sample_count);
    TEST_ASSERT_NULL(tuner.samples);
}

static void test_auto_start_rejects_missing_runtime_storage_without_modifying_state(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_config_t pid_config = default_pid_config();

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));
    memset(&tuner, 0, sizeof(tuner));
    tuner.initialized = true;
    tuner.pid = &pid;
    tuner.samples = NULL;
    tuner.state = XY_PID_AUTO_STATE_IDLE;
    tuner.sample_count = 2U;
    tuner.output_step = -1.0F;
    g_tick_ms = 1234U;

    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_start(&tuner));
    TEST_ASSERT_TRUE(tuner.initialized);
    TEST_ASSERT_EQUAL_PTR(&pid, tuner.pid);
    TEST_ASSERT_NULL(tuner.samples);
    TEST_ASSERT_EQUAL(XY_PID_AUTO_STATE_IDLE, tuner.state);
    TEST_ASSERT_EQUAL_UINT16(2U, tuner.sample_count);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, -1.0F, tuner.output_step);
}

static void test_auto_start_rejects_missing_pid_without_modifying_state(void)
{
    xy_pid_auto_tuner_t tuner;
    xy_pid_auto_config_t auto_config = default_auto_config();

    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_init(&tuner, NULL, &auto_config));

    memset(&tuner, 0, sizeof(tuner));
    tuner.initialized = true;
    tuner.pid = NULL;
    tuner.state = XY_PID_AUTO_STATE_IDLE;
    tuner.sample_count = 2U;
    tuner.output_step = -1.0F;
    g_tick_ms = 1234U;

    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_start(&tuner));
    TEST_ASSERT_TRUE(tuner.initialized);
    TEST_ASSERT_NULL(tuner.pid);
    TEST_ASSERT_EQUAL(XY_PID_AUTO_STATE_IDLE, tuner.state);
    TEST_ASSERT_EQUAL_UINT16(2U, tuner.sample_count);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, -1.0F, tuner.output_step);
}

static void test_auto_apply_rejects_complete_tuner_without_pid(void)
{
    xy_pid_auto_tuner_t tuner;

    memset(&tuner, 0, sizeof(tuner));
    tuner.initialized = true;
    tuner.pid = NULL;
    tuner.state = XY_PID_AUTO_STATE_COMPLETE;
    tuner.result.kp = 1.0F;
    tuner.result.ki = 2.0F;
    tuner.result.kd = 3.0F;

    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_apply(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_STATE_COMPLETE, tuner.state);
    TEST_ASSERT_NULL(tuner.pid);
}

static void test_auto_public_ops_reject_uninitialized_tuner_without_state_changes(void)
{
    xy_pid_auto_tuner_t tuner;
    xy_pid_auto_result_t result = {
        .kp = 11.0F,
        .ki = 22.0F,
        .kd = 33.0F,
    };

    memset(&tuner, 0, sizeof(tuner));
    tuner.initialized = false;
    tuner.state = XY_PID_AUTO_STATE_COMPLETE;
    tuner.sample_count = 2U;
    tuner.config.num_samples = 4U;
    tuner.result.kp = 1.0F;
    tuner.result.ki = 2.0F;
    tuner.result.kd = 3.0F;

    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_stop(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_STATE_COMPLETE, tuner.state);
    TEST_ASSERT_EQUAL_UINT16(2U, tuner.sample_count);

    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_get_result(&tuner, &result));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 11.0F, result.kp);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 22.0F, result.ki);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 33.0F, result.kd);

    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_apply(&tuner));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.0F, xy_pid_auto_get_progress(&tuner));
}

static void test_auto_progress_clamps_inconsistent_sample_count(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_config_t pid_config = default_pid_config();
    xy_pid_auto_config_t auto_config = default_auto_config();

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_init(&tuner, &pid, &auto_config));

    tuner.sample_count = auto_config.num_samples + 3U;
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 100.0F, xy_pid_auto_get_progress(&tuner));

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_deinit(&tuner));
}

static void test_auto_apply_handles_invalid_pid_tuning_without_mode_change(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_config_t pid_config = default_pid_config();
    xy_pid_auto_config_t auto_config = default_auto_config();

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_init(&tuner, &pid, &auto_config));
    tuner.state = XY_PID_AUTO_STATE_COMPLETE;
    tuner.result.kp = 4.0F;
    tuner.result.ki = 0.0F;
    tuner.result.kd = 2.0F;

    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_apply(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_MODE_MANUAL, xy_pid_get_mode(&pid));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, pid_config.kp, pid.config.kp);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, pid_config.ki, pid.config.ki);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, pid_config.kd, pid.config.kd);

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_deinit(&tuner));
}

static void test_auto_apply_preserves_existing_tuning_when_pid_rejects_result(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_config_t pid_config = default_pid_config();
    xy_pid_auto_config_t auto_config = default_auto_config();

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));
    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_set_mode(&pid, XY_PID_MODE_MANUAL));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_init(&tuner, &pid, &auto_config));
    tuner.state = XY_PID_AUTO_STATE_COMPLETE;
    tuner.result.kp = -1.0F;
    tuner.result.ki = 2.0F;
    tuner.result.kd = 3.0F;

    TEST_ASSERT_EQUAL(XY_PID_AUTO_INVALID_PARAM, xy_pid_auto_apply(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_MODE_MANUAL, xy_pid_get_mode(&pid));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, pid_config.kp, pid.config.kp);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, pid_config.ki, pid.config.ki);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, pid_config.kd, pid.config.kd);

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_deinit(&tuner));
}

static void run_auto_declared_method_fallback_case(xy_pid_auto_method_t method)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_auto_result_t result = {0};
    xy_pid_config_t pid_config = default_pid_config();
    xy_pid_auto_config_t auto_config = default_auto_config();
    auto_config.method = method;

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_init(&tuner, &pid, &auto_config));
    TEST_ASSERT_EQUAL(method, tuner.config.method);
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_start(&tuner));

    const float samples[] = {1.0F, 2.0F, 4.0F, 10.0F};
    for (uint16_t i = 0; i < auto_config.num_samples; ++i) {
        g_tick_ms += auto_config.sample_interval_ms;
        TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_loop(&tuner, samples[i]));
    }

    TEST_ASSERT_EQUAL(XY_PID_AUTO_STATE_COMPLETE, xy_pid_auto_get_state(&tuner));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_get_result(&tuner, &result));
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 3.12F, result.kp);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 10.0F, result.ki);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 2.5F, result.kd);

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_deinit(&tuner));
}

static void test_auto_cohen_completion_uses_degenerate_zn_fallback(void)
{
    run_auto_declared_method_fallback_case(XY_PID_AUTO_METHOD_COHEN);
}

static void test_auto_imc_completion_uses_degenerate_zn_fallback(void)
{
    run_auto_declared_method_fallback_case(XY_PID_AUTO_METHOD_IMC);
}

static void test_auto_loop_wraparound_tick_still_samples_at_interval(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_config_t pid_config = default_pid_config();
    xy_pid_auto_config_t auto_config = default_auto_config();

    TEST_ASSERT_EQUAL(XY_PID_OK, xy_pid_init(&pid, &pid_config));
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_init(&tuner, &pid, &auto_config));

    g_tick_ms = UINT32_MAX - 1U;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_start(&tuner));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX - 1U, tuner.start_time);

    g_tick_ms = 1U;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_loop(&tuner, 11.0F));
    TEST_ASSERT_EQUAL_UINT16(0U, tuner.sample_count);
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX - 1U, tuner.start_time);

    g_tick_ms = auto_config.sample_interval_ms;
    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_loop(&tuner, 12.0F));
    TEST_ASSERT_EQUAL_UINT16(1U, tuner.sample_count);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 12.0F, tuner.samples[0]);
    TEST_ASSERT_EQUAL_UINT32(auto_config.sample_interval_ms, tuner.start_time);

    TEST_ASSERT_EQUAL(XY_PID_AUTO_OK, xy_pid_auto_deinit(&tuner));
}

void setUp(void)
{
    g_tick_ms = 0U;
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_auto_init_defaults_and_deinit);
    RUN_TEST(test_auto_init_accepts_zero_overrides_as_defaults);
    RUN_TEST(test_auto_init_rejects_invalid_config_without_touching_tuner);
    RUN_TEST(test_auto_start_stop_and_progress_guards);
    RUN_TEST(test_auto_loop_completion_result_and_apply);
    RUN_TEST(test_auto_apply_requires_complete_state);
    RUN_TEST(test_auto_zn_degenerate_flat_response_enters_error_state);
    RUN_TEST(test_auto_loop_rejects_deinitialized_tuner_without_touching_freed_samples);
    RUN_TEST(test_auto_start_rejects_missing_runtime_storage_without_modifying_state);
    RUN_TEST(test_auto_start_rejects_missing_pid_without_modifying_state);
    RUN_TEST(test_auto_apply_rejects_complete_tuner_without_pid);
    RUN_TEST(test_auto_public_ops_reject_uninitialized_tuner_without_state_changes);
    RUN_TEST(test_auto_progress_clamps_inconsistent_sample_count);
    RUN_TEST(test_auto_apply_handles_invalid_pid_tuning_without_mode_change);
    RUN_TEST(test_auto_apply_preserves_existing_tuning_when_pid_rejects_result);
    RUN_TEST(test_auto_cohen_completion_uses_degenerate_zn_fallback);
    RUN_TEST(test_auto_imc_completion_uses_degenerate_zn_fallback);
    RUN_TEST(test_auto_loop_wraparound_tick_still_samples_at_interval);
    return UNITY_END();
}
