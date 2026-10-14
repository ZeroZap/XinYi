#include <stdint.h>

#include "unity.h"
#include "xy_log.h"

void xy_log_char(char ch) {
    (void)ch;
}

void setUp(void) {}

void tearDown(void) {}

static void test_release_default_and_runtime_ceiling_are_warn(void) {
    TEST_ASSERT_EQUAL_UINT8(XY_LOG_LEVEL_WARN, xy_log_dynamic_level());

    xy_log_set_dynamic_level(XY_LOG_LEVEL_ERROR);
    TEST_ASSERT_EQUAL_UINT8(XY_LOG_LEVEL_ERROR, xy_log_dynamic_level());

    xy_log_set_dynamic_level(XY_LOG_LEVEL_INFO);
    TEST_ASSERT_EQUAL_UINT8(XY_LOG_LEVEL_ERROR, xy_log_dynamic_level());

    xy_log_set_dynamic_level(XY_LOG_LEVEL_WARN);
    TEST_ASSERT_EQUAL_UINT8(XY_LOG_LEVEL_WARN, xy_log_dynamic_level());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_release_default_and_runtime_ceiling_are_warn);
    return UNITY_END();
}
