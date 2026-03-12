/**
 * @file test_device.c
 * @brief Device Framework Unit Tests
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "xy_device.h"
#include "xy_device_core.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) static void name(void)
#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    tests_run++; \
    name(); \
    tests_passed++; \
    printf("PASSED\n"); \
} while(0)

#define ASSERT(cond) do { if (!(cond)) { \
    printf("FAILED: %s:%d - %s\n", __FILE__, __LINE__, #cond); \
    exit(1); \
} } while(0)

/* Test: Device registry initialization */
TEST(test_device_registry_init)
{
    int ret = xy_device_registry_init();
    ASSERT(ret == 0);
    ASSERT(xy_device_get_count() == 0);
}

/* Test: Device registration */
TEST(test_device_register)
{
    static xy_i2c_device_t test_dev;
    
    xy_i2c_device_init(&test_dev, NULL, 0x44, 1000);
    test_dev.base.name = "test_device";
    test_dev.base.type = XY_DEVICE_TYPE_SENSOR;
    
    int ret = xy_device_registry_register(&test_dev.base);
    ASSERT(ret == XY_DEVICE_OK);
    ASSERT(xy_device_get_count() == 1);
}

/* Test: Find device by name */
TEST(test_device_find_by_name)
{
    xy_device_t *dev = xy_device_find_by_name("test_device");
    ASSERT(dev != NULL);
    ASSERT(strcmp(dev->name, "test_device") == 0);
}

/* Test: Find device by type */
TEST(test_device_find_by_type)
{
    xy_device_t *dev = xy_device_find_by_type(XY_DEVICE_TYPE_SENSOR, 0);
    ASSERT(dev != NULL);
    ASSERT(dev->type == XY_DEVICE_TYPE_SENSOR);
}

/* Test: Device power management */
TEST(test_device_power_management)
{
    xy_device_t *dev = xy_device_find_by_name("test_device");
    ASSERT(dev != NULL);
    
    int ret = xy_device_sleep(dev);
    ASSERT(ret == XY_DEVICE_OK);
    
    xy_device_pm_state_t state = xy_device_get_pm_state(dev);
    ASSERT(state == XY_DEVICE_PM_SLEEP_STATE);
    
    ret = xy_device_wake(dev);
    ASSERT(ret == XY_DEVICE_OK);
    
    state = xy_device_get_pm_state(dev);
    ASSERT(state == XY_DEVICE_PM_ACTIVE);
}

/* Test: Device statistics */
TEST(test_device_stats)
{
    xy_device_stats_t stats;
    int ret = xy_device_get_stats(&stats);
    ASSERT(ret == XY_DEVICE_OK);
    ASSERT(stats.total_devices >= 1);
    ASSERT(stats.sensor_count >= 1);
}

/* Test: Duplicate registration should fail */
TEST(test_device_duplicate_register)
{
    static xy_i2c_device_t test_dev2;
    
    xy_i2c_device_init(&test_dev2, NULL, 0x45, 1000);
    test_dev2.base.name = "test_device";  /* Same name */
    test_dev2.base.type = XY_DEVICE_TYPE_SENSOR;
    
    int ret = xy_device_registry_register(&test_dev2.base);
    ASSERT(ret == XY_DEVICE_ERROR);  /* Should fail */
}

int main(void)
{
    printf("=== Device Framework Unit Tests ===\n\n");
    
    RUN_TEST(test_device_registry_init);
    RUN_TEST(test_device_register);
    RUN_TEST(test_device_find_by_name);
    RUN_TEST(test_device_find_by_type);
    RUN_TEST(test_device_power_management);
    RUN_TEST(test_device_stats);
    RUN_TEST(test_device_duplicate_register);
    
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n");
        return 1;
    }
}
