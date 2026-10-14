#include <stdint.h>

#include "unity.h"
#include "xy_log.h"

uint8_t g_xy_log_dynamic_level = (uint8_t)(XY_LOG_LEVEL_DEBUG + 1U);

void xy_log_char(char ch) {
    (void)ch;
}

void setUp(void) {}

void tearDown(void) {}

static void test_release_invalid_external_level_fails_closed_to_warn(void) {
    TEST_ASSERT_EQUAL_UINT8(XY_LOG_LEVEL_WARN, xy_log_dynamic_level());

    g_xy_log_dynamic_level = UINT8_MAX;
    TEST_ASSERT_EQUAL_UINT8(XY_LOG_LEVEL_WARN, xy_log_dynamic_level());
}

static void test_release_runtime_ceiling_is_warn(void) {
    xy_log_set_dynamic_level(XY_LOG_LEVEL_WARN);

    xy_log_set_dynamic_level(XY_LOG_LEVEL_ERROR);
    TEST_ASSERT_EQUAL_UINT8(XY_LOG_LEVEL_ERROR, xy_log_dynamic_level());

    xy_log_set_dynamic_level(XY_LOG_LEVEL_INFO);
    TEST_ASSERT_EQUAL_UINT8(XY_LOG_LEVEL_ERROR, xy_log_dynamic_level());

    xy_log_set_dynamic_level(XY_LOG_LEVEL_WARN);
    TEST_ASSERT_EQUAL_UINT8(XY_LOG_LEVEL_WARN, xy_log_dynamic_level());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_release_invalid_external_level_fails_closed_to_warn);
    RUN_TEST(test_release_runtime_ceiling_is_warn);
    return UNITY_END();
}
