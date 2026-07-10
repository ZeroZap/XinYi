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

    RUN_TEST(test_device_registry_init);
    RUN_TEST(test_device_register_via_helper);
    RUN_TEST(test_find_by_name);
    RUN_TEST(test_find_by_type);
    RUN_TEST(test_public_find_forwards);
    RUN_TEST(test_device_power_management);
    RUN_TEST(test_device_stats);
    RUN_TEST(test_duplicate_register_rejected);
    RUN_TEST(test_unregister_frees_slot);

    return UNITY_END();
}
