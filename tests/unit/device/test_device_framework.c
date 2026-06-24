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

#define TEST(name) static void name(void)
#define ASSERT(cond) TEST_ASSERT_TRUE(cond)

void setUp(void)
{
}

void tearDown(void)
{
}

/* A non-NULL placeholder bus handle for the PC-stub I2C HAL. */
static int g_fake_i2c_bus = 1;

TEST(test_device_registry_init)
{
    int ret = xy_device_registry_init();
    ASSERT(ret == 0);
    ASSERT(xy_device_registry_count() == 0);
}

TEST(test_device_register_via_helper)
{
    static xy_i2c_device_t dev;
    memset(&dev, 0, sizeof(dev));

    ASSERT(xy_i2c_device_init(&dev, &g_fake_i2c_bus, 0x44, 1000) == XY_DEVICE_OK);
    ASSERT(xy_i2c_device_register(&dev, "sensor_a", XY_DEV_TYPE_SENSOR) == XY_DEVICE_OK);
    ASSERT(xy_device_registry_count() == 1);
}

TEST(test_find_by_name)
{
    xy_device_t *dev = xy_device_find_by_name("sensor_a");
    ASSERT(dev != NULL);
    ASSERT(strcmp(dev->name, "sensor_a") == 0);
    ASSERT(dev->type == XY_DEV_TYPE_SENSOR);
}

TEST(test_find_by_type)
{
    xy_device_t *dev = xy_device_find_by_type(XY_DEV_TYPE_SENSOR, 0);
    ASSERT(dev != NULL);
    ASSERT(dev->type == XY_DEV_TYPE_SENSOR);
}

TEST(test_public_find_forwards)
{
    /* xy_device_find (public link-list-era API) should forward to the
     * canonical static-array registry after the merge. */
    xy_device_t *dev = xy_device_find("sensor_a");
    ASSERT(dev != NULL);
    ASSERT(strcmp(dev->name, "sensor_a") == 0);
}

TEST(test_device_power_management)
{
    xy_device_t *dev = xy_device_find_by_name("sensor_a");
    ASSERT(dev != NULL);

    /* Mark initialised so xy_device_pm_check considers it. */
    dev->initialized = 1;

    int ret = xy_device_sleep(dev);
    ASSERT(ret == XY_DEVICE_OK);
    ASSERT(xy_device_get_pm_state(dev) == XY_DEVICE_PM_SLEEP_STATE);

    ret = xy_device_wake(dev);
    ASSERT(ret == XY_DEVICE_OK);
    ASSERT(xy_device_get_pm_state(dev) == XY_DEVICE_PM_ACTIVE);
}

TEST(test_device_stats)
{
    xy_device_stats_t stats;
    int ret = xy_device_get_stats(&stats);
    ASSERT(ret == XY_DEVICE_OK);
    ASSERT(stats.total_devices >= 1);
    ASSERT(stats.sensor_count >= 1);
}

TEST(test_duplicate_register_rejected)
{
    static xy_i2c_device_t dup;
    memset(&dup, 0, sizeof(dup));

    ASSERT(xy_i2c_device_init(&dup, &g_fake_i2c_bus, 0x45, 1000) == XY_DEVICE_OK);
    int ret = xy_i2c_device_register(&dup, "sensor_a", XY_DEV_TYPE_SENSOR);
    ASSERT(ret != XY_DEVICE_OK);
}

TEST(test_unregister_frees_slot)
{
    xy_device_t *dev = xy_device_find_by_name("sensor_a");
    ASSERT(dev != NULL);

    int ret = xy_device_registry_unregister(dev);
    ASSERT(ret == XY_DEVICE_OK);
    ASSERT(xy_device_find_by_name("sensor_a") == NULL);
    ASSERT(xy_device_registry_count() == 0);
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
