#include "unity.h"

#include "xy_device_pm.h"

#include <string.h>

static uint32_t fake_tick;
static unsigned int set_state_calls;
static xy_device_pm_state_t last_requested_state;
static int set_state_result;
static unsigned int set_wakeup_calls;
static bool last_requested_wakeup;
static int set_wakeup_result;
static unsigned int get_consumption_calls;
static uint32_t reported_consumption;
static int get_consumption_result;
static unsigned int get_state_calls;
static xy_device_pm_state_t reported_state;
static int get_state_result;

uint32_t xy_device_get_tick(void) {
    return fake_tick;
}

static int capture_set_state(xy_device_t* dev, xy_device_pm_state_t state) {
    TEST_ASSERT_NOT_NULL(dev);
    set_state_calls++;
    last_requested_state = state;
    return set_state_result;
}

static int capture_set_wakeup(xy_device_t* dev, bool enable) {
    TEST_ASSERT_NOT_NULL(dev);
    set_wakeup_calls++;
    last_requested_wakeup = enable;
    return set_wakeup_result;
}

static int capture_get_consumption(xy_device_t* dev, uint32_t* uw) {
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(uw);
    get_consumption_calls++;
    *uw = reported_consumption;
    return get_consumption_result;
}

static int capture_get_state(xy_device_t* dev, xy_device_pm_state_t* state) {
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(state);
    get_state_calls++;
    *state = reported_state;
    return get_state_result;
}

void setUp(void) {
    fake_tick = 100U;
    set_state_calls = 0U;
    last_requested_state = XY_DEVICE_PM_STATE_ACTIVE;
    set_state_result = XY_DEVICE_OK;
    set_wakeup_calls = 0U;
    last_requested_wakeup = false;
    set_wakeup_result = XY_DEVICE_OK;
    get_consumption_calls = 0U;
    reported_consumption = 1234U;
    get_consumption_result = XY_DEVICE_OK;
    get_state_calls = 0U;
    reported_state = XY_DEVICE_PM_STATE_DEEP_SLEEP;
    get_state_result = XY_DEVICE_OK;
}

void tearDown(void) {}

static void test_device_pm_idle_timeout_uses_device_tick(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {.set_state = capture_set_state};

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_set_idle_timeout(&dev, 50U));

    fake_tick = 149U;
    xy_device_pm_check_idle(&dev);
    TEST_ASSERT_EQUAL_UINT(0U, set_state_calls);

    fake_tick = 150U;
    xy_device_pm_check_idle(&dev);
    TEST_ASSERT_EQUAL_UINT(1U, set_state_calls);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_SLEEP, last_requested_state);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_enabling_idle_timeout_starts_a_fresh_window(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {.set_state = capture_set_state};

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));

    fake_tick = 1000U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_set_idle_timeout(&dev, 50U));

    fake_tick = 1049U;
    xy_device_pm_check_idle(&dev);
    TEST_ASSERT_EQUAL_UINT(0U, set_state_calls);

    fake_tick = 1050U;
    xy_device_pm_check_idle(&dev);
    TEST_ASSERT_EQUAL_UINT(1U, set_state_calls);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_SLEEP, last_requested_state);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_keeps_driver_data_and_isolates_devices(void) {
    xy_device_t first;
    xy_device_t second;
    const xy_device_pm_ops_t ops = {.set_state = capture_set_state};
    uint32_t first_driver_data = 0x11223344U;
    uint32_t second_driver_data = 0x55667788U;
    xy_device_pm_state_t state = XY_DEVICE_PM_STATE_OFF;

    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    first.data = &first_driver_data;
    second.data = &second_driver_data;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&first, &ops));
    TEST_ASSERT_EQUAL_PTR(&first_driver_data, first.data);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&second, &ops));
    TEST_ASSERT_EQUAL_PTR(&second_driver_data, second.data);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_sleep(&first));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&first, &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_SLEEP, state);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&second, &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_ACTIVE, state);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&first));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&second));
}

static void test_device_pm_deinit_reclaims_capacity(void) {
    xy_device_t devices[17];
    const xy_device_pm_ops_t ops = {.set_state = capture_set_state};
    xy_device_pm_state_t state = XY_DEVICE_PM_STATE_OFF;

    memset(devices, 0, sizeof(devices));
    for (size_t i = 0; i < 16U; ++i) {
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&devices[i], &ops));
    }
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NO_MEM, xy_device_pm_init(&devices[16], &ops));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&devices[7]));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_INIT, xy_device_pm_get_state(&devices[7], &state));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&devices[16], &ops));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&devices[16], &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_ACTIVE, state);

    for (size_t i = 0; i < 17U; ++i) {
        if (i != 7U) {
            TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&devices[i]));
        }
    }
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_INIT, xy_device_pm_deinit(&devices[7]));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_device_pm_deinit(NULL));
}

static void test_device_pm_rejects_invalid_enum_values_without_callbacks(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {
        .set_state = capture_set_state,
        .set_wakeup = capture_set_wakeup,
    };
    xy_device_pm_state_t state = XY_DEVICE_PM_STATE_OFF;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_device_pm_set_state(&dev, (xy_device_pm_state_t)99));
    TEST_ASSERT_EQUAL_UINT(0U, set_state_calls);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_ACTIVE, state);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_device_pm_set_policy(&dev, (xy_device_pm_policy_t)99));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_POLICY_AUTO, xy_device_pm_get_policy(&dev));
    TEST_ASSERT_EQUAL_UINT(0U, set_wakeup_calls);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_wakeup_callback_failure_is_retryable(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {.set_wakeup = capture_set_wakeup};

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));

    set_wakeup_result = XY_DEVICE_IO_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_device_pm_set_wakeup(&dev, true));
    TEST_ASSERT_EQUAL_UINT(1U, set_wakeup_calls);
    TEST_ASSERT_TRUE(last_requested_wakeup);

    set_wakeup_result = XY_DEVICE_OK;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_set_wakeup(&dev, true));
    TEST_ASSERT_EQUAL_UINT(2U, set_wakeup_calls);
    TEST_ASSERT_TRUE(last_requested_wakeup);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_consumption_callback_failure_preserves_output(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {.get_power_consumption = capture_get_consumption};
    uint32_t consumption = 0xA5A5A5A5U;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));

    get_consumption_result = XY_DEVICE_IO_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_device_pm_get_consumption(&dev, &consumption));
    TEST_ASSERT_EQUAL_UINT(1U, get_consumption_calls);
    TEST_ASSERT_EQUAL_HEX32(0xA5A5A5A5U, consumption);

    get_consumption_result = XY_DEVICE_OK;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_consumption(&dev, &consumption));
    TEST_ASSERT_EQUAL_UINT(2U, get_consumption_calls);
    TEST_ASSERT_EQUAL_UINT32(reported_consumption, consumption);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_state_callback_failure_preserves_output(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {.get_state = capture_get_state};
    xy_device_pm_state_t state = XY_DEVICE_PM_STATE_OFF;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));

    get_state_result = XY_DEVICE_IO_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL_UINT(1U, get_state_calls);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_OFF, state);

    get_state_result = XY_DEVICE_OK;
    reported_state = (xy_device_pm_state_t)99;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL_UINT(2U, get_state_calls);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_OFF, state);

    reported_state = XY_DEVICE_PM_STATE_DEEP_SLEEP;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL_UINT(3U, get_state_calls);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_DEEP_SLEEP, state);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_driver_reported_sleep_wakes_to_previous_active_state(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {
        .set_state = capture_set_state,
        .get_state = capture_get_state,
    };
    xy_device_pm_state_t state = XY_DEVICE_PM_STATE_OFF;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));

    reported_state = XY_DEVICE_PM_STATE_SLEEP;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_SLEEP, state);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_wakeup(&dev));
    TEST_ASSERT_EQUAL_UINT(1U, set_state_calls);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_ACTIVE, last_requested_state);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_wakeup_from_nested_low_power_state_returns_active(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {.set_state = capture_set_state};
    xy_device_pm_state_t state = XY_DEVICE_PM_STATE_OFF;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_sleep(&dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_device_pm_set_state(&dev, XY_DEVICE_PM_STATE_DEEP_SLEEP));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_wakeup(&dev));
    TEST_ASSERT_EQUAL_UINT(3U, set_state_calls);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_ACTIVE, last_requested_state);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_ACTIVE, state);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_reinit_preserves_live_state(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {.set_state = capture_set_state};
    const xy_device_pm_ops_t replacement_ops = {.set_state = capture_set_state};
    xy_device_pm_state_t state = XY_DEVICE_PM_STATE_OFF;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_sleep(&dev));
    TEST_ASSERT_EQUAL_UINT(1U, set_state_calls);

    fake_tick = 500U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ALREADY_INIT, xy_device_pm_init(&dev, &replacement_ops));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_SLEEP, state);
    TEST_ASSERT_EQUAL_UINT(1U, set_state_calls);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_wakeup(&dev));
    TEST_ASSERT_EQUAL_UINT(2U, set_state_calls);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_ACTIVE, last_requested_state);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_always_on_policy_wakes_atomically(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {.set_state = capture_set_state};
    xy_device_pm_state_t state = XY_DEVICE_PM_STATE_OFF;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_sleep(&dev));

    set_state_result = XY_DEVICE_IO_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_device_pm_set_policy(&dev, XY_DEVICE_PM_POLICY_ALWAYS_ON));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_POLICY_AUTO, xy_device_pm_get_policy(&dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_SLEEP, state);

    set_state_result = XY_DEVICE_OK;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_device_pm_set_policy(&dev, XY_DEVICE_PM_POLICY_ALWAYS_ON));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_POLICY_ALWAYS_ON, xy_device_pm_get_policy(&dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_ACTIVE, state);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_auto_policy_starts_a_fresh_idle_window(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {.set_state = capture_set_state};

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_set_idle_timeout(&dev, 50U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_set_policy(&dev, XY_DEVICE_PM_POLICY_MANUAL));

    fake_tick = 1000U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_set_policy(&dev, XY_DEVICE_PM_POLICY_AUTO));
    fake_tick = 1049U;
    xy_device_pm_check_idle(&dev);
    TEST_ASSERT_EQUAL_UINT(0U, set_state_calls);
    fake_tick = 1050U;
    xy_device_pm_check_idle(&dev);
    TEST_ASSERT_EQUAL_UINT(1U, set_state_calls);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_SLEEP, last_requested_state);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_idle_sleep_failure_preserves_active_state_for_retry(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {.set_state = capture_set_state};
    xy_device_pm_state_t state = XY_DEVICE_PM_STATE_OFF;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_set_idle_timeout(&dev, 50U));

    fake_tick = 150U;
    set_state_result = XY_DEVICE_IO_ERROR;
    xy_device_pm_check_idle(&dev);
    TEST_ASSERT_EQUAL_UINT(1U, set_state_calls);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_ACTIVE, state);

    set_state_result = XY_DEVICE_OK;
    xy_device_pm_check_idle(&dev);
    TEST_ASSERT_EQUAL_UINT(2U, set_state_calls);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_SLEEP, last_requested_state);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_SLEEP, state);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_always_on_policy_rejects_low_power_states(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {.set_state = capture_set_state};
    xy_device_pm_state_t state = XY_DEVICE_PM_STATE_OFF;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_device_pm_set_policy(&dev, XY_DEVICE_PM_POLICY_ALWAYS_ON));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_device_pm_set_state(&dev, XY_DEVICE_PM_STATE_SLEEP));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY,
                          xy_device_pm_set_state(&dev, XY_DEVICE_PM_STATE_DEEP_SLEEP));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_device_pm_set_state(&dev, XY_DEVICE_PM_STATE_OFF));
    TEST_ASSERT_EQUAL_UINT(0U, set_state_calls);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_ACTIVE, state);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_always_on_policy_rejects_driver_reported_sleep(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {.get_state = capture_get_state};
    xy_device_pm_state_t state = XY_DEVICE_PM_STATE_OFF;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_device_pm_set_policy(&dev, XY_DEVICE_PM_POLICY_ALWAYS_ON));

    reported_state = XY_DEVICE_PM_STATE_SLEEP;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL_UINT(1U, get_state_calls);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_OFF, state);

    reported_state = XY_DEVICE_PM_STATE_ACTIVE;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL_UINT(2U, get_state_calls);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_ACTIVE, state);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

static void test_device_pm_record_activity_propagates_wakeup_failure(void) {
    xy_device_t dev;
    const xy_device_pm_ops_t ops = {.set_state = capture_set_state};
    xy_device_pm_state_t state = XY_DEVICE_PM_STATE_OFF;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_device_pm_record_activity(NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_INIT, xy_device_pm_record_activity(&dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_init(&dev, &ops));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_sleep(&dev));

    set_state_result = XY_DEVICE_IO_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_device_pm_record_activity(&dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_SLEEP, state);

    set_state_result = XY_DEVICE_OK;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_record_activity(&dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_get_state(&dev, &state));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_STATE_ACTIVE, state);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_pm_deinit(&dev));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_device_pm_idle_timeout_uses_device_tick);
    RUN_TEST(test_device_pm_enabling_idle_timeout_starts_a_fresh_window);
    RUN_TEST(test_device_pm_keeps_driver_data_and_isolates_devices);
    RUN_TEST(test_device_pm_deinit_reclaims_capacity);
    RUN_TEST(test_device_pm_rejects_invalid_enum_values_without_callbacks);
    RUN_TEST(test_device_pm_wakeup_callback_failure_is_retryable);
    RUN_TEST(test_device_pm_consumption_callback_failure_preserves_output);
    RUN_TEST(test_device_pm_state_callback_failure_preserves_output);
    RUN_TEST(test_device_pm_driver_reported_sleep_wakes_to_previous_active_state);
    RUN_TEST(test_device_pm_wakeup_from_nested_low_power_state_returns_active);
    RUN_TEST(test_device_pm_reinit_preserves_live_state);
    RUN_TEST(test_device_pm_always_on_policy_wakes_atomically);
    RUN_TEST(test_device_pm_auto_policy_starts_a_fresh_idle_window);
    RUN_TEST(test_device_pm_idle_sleep_failure_preserves_active_state_for_retry);
    RUN_TEST(test_device_pm_always_on_policy_rejects_low_power_states);
    RUN_TEST(test_device_pm_always_on_policy_rejects_driver_reported_sleep);
    RUN_TEST(test_device_pm_record_activity_propagates_wakeup_failure);
    return UNITY_END();
}
