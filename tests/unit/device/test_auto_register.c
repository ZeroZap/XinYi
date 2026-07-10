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

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_first_device_registered_before_main(void)
{
    xy_device_t *dev = xy_device_find_by_name("autoreg_one");
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_EQUAL(XY_DEV_TYPE_SENSOR, dev->type);
    TEST_ASSERT_EQUAL_STRING("autoreg_one", dev->name);
    TEST_ASSERT_EQUAL(XY_DEV_FLAG_RDWR, dev->flags);
    TEST_ASSERT_EQUAL(XY_DEV_STATE_INIT, dev->state);
}

static void test_second_device_registered_before_main(void)
{
    xy_device_t *dev = xy_device_find_by_name("autoreg_two");
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_EQUAL(XY_DEV_TYPE_STORAGE, dev->type);
}

static void test_lazy_registry_init_works(void)
{
    /* Constructors fire before any explicit xy_device_init(); the count
     * must already reflect both auto-registered devices. */
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(2U, xy_device_registry_count());
}

static void test_explicit_init_is_idempotent_after_constructors(void)
{
    /* Calling init after constructors fired must not wipe the registry. */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_init());
    TEST_ASSERT_NOT_NULL(xy_device_find_by_name("autoreg_one"));
    TEST_ASSERT_NOT_NULL(xy_device_find_by_name("autoreg_two"));
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
