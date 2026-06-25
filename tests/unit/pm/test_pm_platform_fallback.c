#include "unity.h"

#include <stdint.h>
#include <string.h>

#include "xy_pm.h"

void xy_pm_platform_set_fallback_tick(uint32_t tick);
int xy_pm_platform_get_charger_enable_level(void);

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_fallback_platform_identity(void)
{
    TEST_ASSERT_EQUAL_STRING("Unknown", xy_pm_get_platform_name());
    TEST_ASSERT_FALSE(xy_pm_is_platform(XY_PLATFORM_ID_PC));
    TEST_ASSERT_FALSE(xy_pm_is_platform(XY_PLATFORM_ID_STM32));
    TEST_ASSERT_FALSE(xy_pm_is_platform(XY_PLATFORM_ID_WCH));
    TEST_ASSERT_FALSE(xy_pm_is_platform(XY_PLATFORM_ID_HC32));
    TEST_ASSERT_FALSE(xy_pm_is_platform(XY_PLATFORM_ID_UNKNOWN));
}

static void test_fallback_tick_has_no_read_side_effect(void)
{
    xy_pm_platform_set_fallback_tick(1234U);
    TEST_ASSERT_EQUAL_UINT32(1234U, xy_pm_tick_get());
    TEST_ASSERT_EQUAL_UINT32(1234U, xy_pm_tick_get());

    xy_pm_platform_set_fallback_tick(4321U);
    TEST_ASSERT_EQUAL_UINT32(4321U, xy_pm_tick_get());
}

static void test_fallback_charger_hooks_are_safe_noops(void)
{
    TEST_ASSERT_EQUAL(XY_PM_OK, xy_charger_hw_init());
    TEST_ASSERT_EQUAL(XY_PM_OK, xy_charger_hw_enable(1));
    TEST_ASSERT_EQUAL_INT(1, xy_pm_platform_get_charger_enable_level());
    TEST_ASSERT_EQUAL(XY_PM_OK, xy_charger_hw_disable());
    TEST_ASSERT_EQUAL_INT(0, xy_pm_platform_get_charger_enable_level());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fallback_platform_identity);
    RUN_TEST(test_fallback_tick_has_no_read_side_effect);
    RUN_TEST(test_fallback_charger_hooks_are_safe_noops);
    return UNITY_END();
}
