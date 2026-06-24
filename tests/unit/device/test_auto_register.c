/**
 * @file test_auto_register.c
 * @brief Verify XY_DEVICE_REGISTER auto-registers devices before main().
 *
 * The macro pairs a static xy_device_t with XY_INITIALIZER, which expands
 * to a GCC constructor at XY_INIT_LEVEL_DRIVER.  By the time main() runs
 * the device should already be in the registry — without any explicit
 * xy_device_init() call, since xy_device_registry_register lazy-inits.
 */

#include <string.h>

#include "unity.h"
#include "xy_device.h"
#include "xy_device_core.h"

/* File-scope declarations — constructors fire before main(). */
XY_DEVICE_REGISTER(autoreg_one, XY_DEV_TYPE_SENSOR, NULL, NULL);
XY_DEVICE_REGISTER(autoreg_two, XY_DEV_TYPE_STORAGE, NULL, NULL);

#define TEST(name) static void name(void)
#define ASSERT(cond) TEST_ASSERT_TRUE(cond)

void setUp(void)
{
}

void tearDown(void)
{
}

TEST(test_first_device_registered_before_main)
{
    xy_device_t *dev = xy_device_find_by_name("autoreg_one");
    ASSERT(dev != NULL);
    ASSERT(dev->type == XY_DEV_TYPE_SENSOR);
    ASSERT(strcmp(dev->name, "autoreg_one") == 0);
    ASSERT(dev->flags == XY_DEV_FLAG_RDWR);
    ASSERT(dev->state == XY_DEV_STATE_INIT);
}

TEST(test_second_device_registered_before_main)
{
    xy_device_t *dev = xy_device_find_by_name("autoreg_two");
    ASSERT(dev != NULL);
    ASSERT(dev->type == XY_DEV_TYPE_STORAGE);
}

TEST(test_lazy_registry_init_works)
{
    /* Constructors fire before any explicit xy_device_init(); the count
     * must already reflect both auto-registered devices. */
    ASSERT(xy_device_registry_count() >= 2);
}

TEST(test_explicit_init_is_idempotent_after_constructors)
{
    /* Calling init after constructors fired must not wipe the registry. */
    ASSERT(xy_device_init() == XY_DEVICE_OK);
    ASSERT(xy_device_find_by_name("autoreg_one") != NULL);
    ASSERT(xy_device_find_by_name("autoreg_two") != NULL);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_first_device_registered_before_main);
    RUN_TEST(test_second_device_registered_before_main);
    RUN_TEST(test_lazy_registry_init_works);
    RUN_TEST(test_explicit_init_is_idempotent_after_constructors);

    return UNITY_END();
}
