#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "xy_pm.h"

void xy_pm_platform_set_fallback_tick(uint32_t tick);
int xy_pm_platform_get_charger_enable_level(void);

static void test_fallback_platform_identity(void)
{
    assert(strcmp(xy_pm_get_platform_name(), "Unknown") == 0);
    assert(!xy_pm_is_platform(XY_PLATFORM_ID_PC));
    assert(!xy_pm_is_platform(XY_PLATFORM_ID_STM32));
    assert(!xy_pm_is_platform(XY_PLATFORM_ID_WCH));
    assert(!xy_pm_is_platform(XY_PLATFORM_ID_HC32));
    assert(!xy_pm_is_platform(XY_PLATFORM_ID_UNKNOWN));
}

static void test_fallback_tick_has_no_read_side_effect(void)
{
    xy_pm_platform_set_fallback_tick(1234U);
    assert(xy_pm_tick_get() == 1234U);
    assert(xy_pm_tick_get() == 1234U);

    xy_pm_platform_set_fallback_tick(4321U);
    assert(xy_pm_tick_get() == 4321U);
}

static void test_fallback_charger_hooks_are_safe_noops(void)
{
    assert(xy_charger_hw_init() == XY_PM_OK);
    assert(xy_charger_hw_enable(1) == XY_PM_OK);
    assert(xy_pm_platform_get_charger_enable_level() == 1);
    assert(xy_charger_hw_disable() == XY_PM_OK);
    assert(xy_pm_platform_get_charger_enable_level() == 0);
}

int main(void)
{
    test_fallback_platform_identity();
    test_fallback_tick_has_no_read_side_effect();
    test_fallback_charger_hooks_are_safe_noops();
    puts("PM platform fallback tests passed");
    return 0;
}
