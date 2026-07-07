#include "component_under_test.h"
#include "unity.h"

/* Add fff.h only when dependency calls must be observed. */
/*
#include "fff.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, dependency_call, uint8_t, const uint8_t *, uint16_t)

static void reset_dependency_fakes(void)
{
    RESET_FAKE(dependency_call);
    FFF_RESET_HISTORY();
}
*/

void setUp(void)
{
    /* reset_dependency_fakes(); */
}

void tearDown(void)
{
}

static void test_condition_expected_behavior(void)
{
    TEST_ASSERT_TRUE(true);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_condition_expected_behavior);
    return UNITY_END();
}

/*
CMake wiring:

xy_add_unit_test(test_component_feature component_feature UNITY
    ${UNIT_COMPONENT}/test_component_feature.c
    ${COMPONENT}/xy_component.c
)

Add target-specific include directories, link libraries, or compile definitions after
xy_add_unit_test(...) when needed.
*/
