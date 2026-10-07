#include "unity.h"

#include "xy_device_pm.h"

#include <string.h>

static uint32_t fake_tick;
static unsigned int set_state_calls;
static xy_device_pm_state_t last_requested_state;

uint32_t xy_device_get_tick(void) {
    return fake_tick;
}

static int capture_set_state(xy_device_t* dev, xy_device_pm_state_t state) {
    TEST_ASSERT_NOT_NULL(dev);
    set_state_calls++;
    last_requested_state = state;
    return XY_DEVICE_OK;
}

void setUp(void) {
    fake_tick = 100U;
    set_state_calls = 0U;
    last_requested_state = XY_DEVICE_PM_STATE_ACTIVE;
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
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_device_pm_idle_timeout_uses_device_tick);
    RUN_TEST(test_device_pm_keeps_driver_data_and_isolates_devices);
    return UNITY_END();
}