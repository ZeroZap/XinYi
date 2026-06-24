#include "component_under_test.h"
#include "unity.h"

/* Add fff.h only when dependency calls must be observed. */
/*
#include "fff.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, dependency_call, uint8_t, const uint8_t *, uint16_t)
*/

void setUp(void)
{
    /*
    RESET_FAKE(dependency_call);
    FFF_RESET_HISTORY();
    */
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
