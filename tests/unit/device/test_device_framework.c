/**
 * @file test_device.c
 * @brief Device framework end-to-end tests against the unified registry.
 *
 * Exercises the post-refactor register helpers and the static-array
 * registry: xy_i2c_device_register / find / unregister / PM.
 */

#include <string.h>

#include "unity.h"
#include "xy_device.h"
#include "xy_device_core.h"

void setUp(void)
{
}

void tearDown(void)
{
}

/* A non-NULL placeholder bus handle for the PC-stub I2C HAL. */
static int g_fake_i2c_bus = 1;
static int g_pm_callback_result;

static int capture_pm_event(xy_device_t *dev, xy_device_pm_event_t event, void *user_data)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(user_data);
    TEST_ASSERT_TRUE(event == XY_DEVICE_PM_SLEEP_EVENT || event == XY_DEVICE_PM_WAKE);
    return g_pm_callback_result;
}

static void test_i2c_helpers_return_status_not_transfer_length(void)
{
    xy_i2c_device_t dev = {0};
    uint8_t data[2] = {0x12, 0x34};

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_init(&dev, &g_fake_i2c_bus, 0x44, 1000));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_i2c_device_write(&dev, data, sizeof(data)));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_i2c_device_read(&dev, data, sizeof(data)));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_i2c_device_write_reg(&dev, 0x10, data, sizeof(data)));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_i2c_device_read_reg(&dev, 0x10, data, sizeof(data)));
}

static void test_device_registry_init(void)
{
    int ret = xy_device_registry_init();
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_UINT(0U, xy_device_registry_count());
}

static void test_device_register_via_helper(void)
{
    static xy_i2c_device_t dev;
    memset(&dev, 0, sizeof(dev));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_i2c_device_init(&dev, &g_fake_i2c_bus, 0x44, 1000));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_i2c_device_register(&dev, "sensor_a", XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_UINT(1U, xy_device_registry_count());
}

static void test_find_by_name(void)
{
    xy_device_t *dev = xy_device_find_by_name("sensor_a");
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_EQUAL_STRING("sensor_a", dev->name);
    TEST_ASSERT_EQUAL(XY_DEV_TYPE_SENSOR, dev->type);
}

static void test_find_by_type(void)
{
    xy_device_t *dev = xy_device_find_by_type(XY_DEV_TYPE_SENSOR, 0);
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_EQUAL(XY_DEV_TYPE_SENSOR, dev->type);
}

static void test_public_find_forwards(void)
{
    /* xy_device_find (public link-list-era API) should forward to the
     * canonical static-array registry after the merge. */
    xy_device_t *dev = xy_device_find("sensor_a");
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_EQUAL_STRING("sensor_a", dev->name);
}

static void test_device_power_management(void)
{
    xy_device_t *dev = xy_device_find_by_name("sensor_a");
    TEST_ASSERT_NOT_NULL(dev);

    /* Mark initialised so xy_device_pm_check considers it. */
    dev->initialized = 1;

    int ret = xy_device_sleep(dev);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, ret);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_SLEEP_STATE, xy_device_get_pm_state(dev));

    ret = xy_device_wake(dev);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, ret);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_ACTIVE, xy_device_get_pm_state(dev));
}

static void test_acquire_propagates_wake_failure_without_leaking_reference(void)
{
    xy_device_t *dev = xy_device_find_by_name("sensor_a");
    int callback_marker = 1;

    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_device_set_pm_callback(dev, capture_pm_event, &callback_marker));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_sleep(dev));

    g_pm_callback_result = XY_DEVICE_IO_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_device_acquire(dev));
    TEST_ASSERT_EQUAL_INT(0, xy_device_registry_ref_count(dev));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_SLEEP_STATE, xy_device_get_pm_state(dev));

    g_pm_callback_result = XY_DEVICE_OK;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_acquire(dev));
    TEST_ASSERT_EQUAL_INT(1, xy_device_registry_ref_count(dev));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_ACTIVE, xy_device_get_pm_state(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_release(dev));
    TEST_ASSERT_EQUAL_INT(0, xy_device_registry_ref_count(dev));
}

/* Open/close must be the same reference as the registry/PM accounting, so a
 * device that is in use is never suspended or unregistered underneath its
 * owner, and a failed driver close leaves the handle usable for a retry. */
static int g_open_calls;
static int g_close_calls;
static xy_error_t g_close_result;

static xy_error_t fake_api_open(xy_device_t *dev, uint32_t flags)
{
    XY_UNUSED(flags);
    TEST_ASSERT_NOT_NULL(dev);
    g_open_calls++;
    return XY_OK;
}

static xy_error_t fake_api_close(xy_device_t *dev)
{
    TEST_ASSERT_NOT_NULL(dev);
    g_close_calls++;
    return g_close_result;
}

static const xy_dev_api_t g_fake_api = {
    .open = fake_api_open,
    .close = fake_api_close,
};

static void test_open_close_shares_registry_reference_accounting(void)
{
    static xy_i2c_device_t openable;
    xy_device_t *dev;

    memset(&openable, 0, sizeof(openable));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_init(&openable, &g_fake_i2c_bus, 0x46, 1000));
    openable.base.api = &g_fake_api;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_register(&openable, "openable", XY_DEV_TYPE_SENSOR));

    dev = xy_device_open("openable", 0);
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_EQUAL_INT(1, g_open_calls);
    TEST_ASSERT_EQUAL(XY_DEV_STATE_OPENED, dev->state);
    TEST_ASSERT_EQUAL_INT(1, dev->ref_count);
    TEST_ASSERT_EQUAL_INT(1, xy_device_registry_ref_count(dev));

    /* An open device is in use: PM and the registry must not retire it. */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_device_sleep(dev));
    dev->initialized = 1;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_set_sleep_timeout(dev, 0U));
    xy_device_pm_check();
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_ACTIVE, xy_device_get_pm_state(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_device_registry_unregister(dev));

    /* A second open shares the device instead of double-initialising it. */
    TEST_ASSERT_NOT_NULL(xy_device_open("openable", 0));
    TEST_ASSERT_EQUAL_INT(2, xy_device_registry_ref_count(dev));

    /* The first close only drops one reference and keeps the driver open. */
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_device_close(dev));
    TEST_ASSERT_EQUAL_INT(1, xy_device_registry_ref_count(dev));
    TEST_ASSERT_EQUAL(XY_DEV_STATE_OPENED, dev->state);
    TEST_ASSERT_EQUAL_INT(0, g_close_calls);

    /* A failed driver close is retryable: no reference is lost. */
    g_close_result = XY_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_ERROR, xy_device_close(dev));
    TEST_ASSERT_EQUAL_INT(1, xy_device_registry_ref_count(dev));
    TEST_ASSERT_EQUAL_INT(1, dev->ref_count);
    TEST_ASSERT_EQUAL(XY_DEV_STATE_OPENED, dev->state);

    g_close_result = XY_OK;
    TEST_ASSERT_EQUAL_INT(XY_OK, xy_device_close(dev));
    TEST_ASSERT_EQUAL_INT(0, xy_device_registry_ref_count(dev));
    TEST_ASSERT_EQUAL_INT(0, dev->ref_count);
    TEST_ASSERT_EQUAL(XY_DEV_STATE_CLOSED, dev->state);
    TEST_ASSERT_EQUAL_INT(2, g_close_calls);

    /* Closing an unopened device fails closed without touching the driver. */
    TEST_ASSERT_EQUAL_INT(XY_ERROR_NOT_READY, xy_device_close(dev));
    TEST_ASSERT_EQUAL_INT(2, g_close_calls);
    TEST_ASSERT_EQUAL(XY_DEV_STATE_CLOSED, dev->state);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_registry_unregister(dev));
}

/* The PM callback may register or unregister devices while it runs (a bus
 * manager retiring an unrelated child is enough). Reference accounting and PM
 * state must stay bound to the device that was addressed, not to the registry
 * index that happens to be reused after the callback shifts the array. */
static xy_device_t *g_pm_retire_target;
static int g_pm_retire_event = -1;
static int g_tail_sleep_calls;

static int retire_other_on_pm_event(xy_device_t *dev, xy_device_pm_event_t event, void *user_data)
{
    XY_UNUSED(dev);
    XY_UNUSED(user_data);

    g_pm_retire_event = (int)event;
    if (g_pm_retire_target != NULL) {
        xy_device_t *victim = g_pm_retire_target;
        g_pm_retire_target = NULL;
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_registry_unregister(victim));
    }
    return XY_DEVICE_OK;
}

static int count_sleep_event(xy_device_t *dev, xy_device_pm_event_t event, void *user_data)
{
    XY_UNUSED(dev);
    XY_UNUSED(user_data);

    if (event == XY_DEVICE_PM_SLEEP_EVENT) {
        g_tail_sleep_calls++;
    }
    return XY_DEVICE_OK;
}

static void register_pm_device(xy_i2c_device_t *dev, const char *name, uint8_t addr)
{
    memset(dev, 0, sizeof(*dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_i2c_device_init(dev, &g_fake_i2c_bus, addr, 1000));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_i2c_device_register(dev, name, XY_DEV_TYPE_SENSOR));
}

static void test_acquire_survives_wake_callback_registry_mutation(void)
{
    static xy_i2c_device_t victim;
    static xy_i2c_device_t subject;

    register_pm_device(&victim, "pm_victim_a", 0x50);
    register_pm_device(&subject, "pm_subject_a", 0x51);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_device_set_pm_callback(&subject.base, retire_other_on_pm_event, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_sleep(&subject.base));

    g_pm_retire_event = -1;
    g_pm_retire_target = &victim.base;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_acquire(&subject.base));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_PM_WAKE, g_pm_retire_event);

    /* The acquired reference must land on the requested device, and the
     * device must be observably awake to the PM layer. */
    TEST_ASSERT_EQUAL_INT(1, xy_device_registry_ref_count(&subject.base));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_ACTIVE, xy_device_get_pm_state(&subject.base));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_device_sleep(&subject.base));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_release(&subject.base));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_registry_unregister(&subject.base));
}

static void test_sleep_state_follows_device_after_callback_mutation(void)
{
    static xy_i2c_device_t victim;
    static xy_i2c_device_t subject;

    register_pm_device(&victim, "pm_victim_b", 0x52);
    register_pm_device(&subject, "pm_subject_b", 0x53);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_device_set_pm_callback(&subject.base, retire_other_on_pm_event, NULL));

    g_pm_retire_event = -1;
    g_pm_retire_target = &victim.base;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_sleep(&subject.base));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_PM_SLEEP_EVENT, g_pm_retire_event);

    /* The hardware was told to sleep, so the framework must report the same
     * device as sleeping instead of publishing into a shifted slot. */
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_SLEEP_STATE, xy_device_get_pm_state(&subject.base));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_wake(&subject.base));
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_ACTIVE, xy_device_get_pm_state(&subject.base));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_registry_unregister(&subject.base));
}

static void test_pm_check_sweeps_every_device_after_callback_mutation(void)
{
    static xy_i2c_device_t victim;
    static xy_i2c_device_t subject;
    static xy_i2c_device_t tail;

    register_pm_device(&victim, "pm_victim_c", 0x54);
    register_pm_device(&subject, "pm_subject_c", 0x55);
    register_pm_device(&tail, "pm_tail_c", 0x56);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_device_set_pm_callback(&subject.base, retire_other_on_pm_event, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_device_set_pm_callback(&tail.base, count_sleep_event, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_set_sleep_timeout(&victim.base, 0U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_set_sleep_timeout(&subject.base, 0U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_set_sleep_timeout(&tail.base, 0U));

    g_tail_sleep_calls = 0;
    g_pm_retire_target = &victim.base;
    xy_device_pm_check();

    TEST_ASSERT_EQUAL(XY_DEVICE_PM_SLEEP_STATE, xy_device_get_pm_state(&subject.base));
    /* One sweep must still reach the device that followed the retired one. */
    TEST_ASSERT_EQUAL_INT(1, g_tail_sleep_calls);
    TEST_ASSERT_EQUAL(XY_DEVICE_PM_SLEEP_STATE, xy_device_get_pm_state(&tail.base));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_registry_unregister(&subject.base));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_registry_unregister(&tail.base));
}

static void test_device_stats(void)
{
    xy_device_stats_t stats;
    int ret = xy_device_get_stats(&stats);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, ret);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(1U, stats.total_devices);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(1U, stats.sensor_count);
}

static void test_duplicate_register_rejected(void)
{
    static xy_i2c_device_t dup;
    memset(&dup, 0, sizeof(dup));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_i2c_device_init(&dup, &g_fake_i2c_bus, 0x45, 1000));
    int ret = xy_i2c_device_register(&dup, "sensor_a", XY_DEV_TYPE_SENSOR);
    TEST_ASSERT_NOT_EQUAL(XY_DEVICE_OK, ret);
}

static void test_unregister_frees_slot(void)
{
    xy_device_t *dev = xy_device_find_by_name("sensor_a");
    TEST_ASSERT_NOT_NULL(dev);

    int ret = xy_device_registry_unregister(dev);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, ret);
    TEST_ASSERT_NULL(xy_device_find_by_name("sensor_a"));
    TEST_ASSERT_EQUAL_UINT(0U, xy_device_registry_count());
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_i2c_helpers_return_status_not_transfer_length);
    RUN_TEST(test_device_registry_init);
    RUN_TEST(test_device_register_via_helper);
    RUN_TEST(test_find_by_name);
    RUN_TEST(test_find_by_type);
    RUN_TEST(test_public_find_forwards);
    RUN_TEST(test_device_power_management);
    RUN_TEST(test_acquire_propagates_wake_failure_without_leaking_reference);
    RUN_TEST(test_open_close_shares_registry_reference_accounting);
    RUN_TEST(test_acquire_survives_wake_callback_registry_mutation);
    RUN_TEST(test_sleep_state_follows_device_after_callback_mutation);
    RUN_TEST(test_pm_check_sweeps_every_device_after_callback_mutation);
    RUN_TEST(test_device_stats);
    RUN_TEST(test_duplicate_register_rejected);
    RUN_TEST(test_unregister_frees_slot);

    return UNITY_END();
}
