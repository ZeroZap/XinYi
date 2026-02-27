/**
 * @file test_trace.c
 * @brief Trace Component (xy_log) Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* Trace headers */
#include "xy_log.h"

/* ==================== Test Fixtures ==================== */

void setUp(void)
{
    /* Called before each test */
}

void tearDown(void)
{
    /* Called after each test */
}

/* ==================== Log Level Tests ==================== */

void test_log_level_constants(void)
{
    /* Test log level constant values */
    TEST_ASSERT_EQUAL(0, XY_LOG_LEVEL_NEVER);
    TEST_ASSERT_EQUAL(1, XY_LOG_LEVEL_ERROR);
    TEST_ASSERT_EQUAL(2, XY_LOG_LEVEL_WARN);
    TEST_ASSERT_EQUAL(3, XY_LOG_LEVEL_INFO);
    TEST_ASSERT_EQUAL(4, XY_LOG_LEVEL_DEBUG);
    TEST_ASSERT_EQUAL(5, XY_LOG_LEVEL_VERBOSE);
}

void test_local_log_level_defined(void)
{
    /* Test that LOCAL_LOG_LEVEL is defined */
    TEST_ASSERT_TRUE(LOCAL_LOG_LEVEL >= XY_LOG_LEVEL_NEVER);
    TEST_ASSERT_TRUE(LOCAL_LOG_LEVEL <= XY_LOG_LEVEL_VERBOSE);
}

/* ==================== Log Function Tests ==================== */

void test_log_functions_exist(void)
{
    /* Test that log functions are available */
    TEST_ASSERT_NOT_NULL(xy_log_str);
    TEST_ASSERT_NOT_NULL(xy_log_raw);
    TEST_ASSERT_NOT_NULL(xy_log_init);
    TEST_ASSERT_NOT_NULL(xy_log_set_dynamic_level);
    TEST_ASSERT_NOT_NULL(xy_log_dynamic_level);
}

void test_log_init(void)
{
    /* Test log initialization */
    xy_log_init();
    /* If init succeeds without crash, test passes */
    TEST_ASSERT_TRUE(1);
}

void test_log_dynamic_level(void)
{
    uint8_t original_level;
    uint8_t test_level;

    /* Save original level */
    original_level = xy_log_dynamic_level();

    /* Test setting different log levels */
    test_level = XY_LOG_LEVEL_ERROR;
    xy_log_set_dynamic_level(test_level);
    TEST_ASSERT_EQUAL_UINT8(test_level, xy_log_dynamic_level());

    test_level = XY_LOG_LEVEL_DEBUG;
    xy_log_set_dynamic_level(test_level);
    TEST_ASSERT_EQUAL_UINT8(test_level, xy_log_dynamic_level());

    test_level = XY_LOG_LEVEL_VERBOSE;
    xy_log_set_dynamic_level(test_level);
    TEST_ASSERT_EQUAL_UINT8(test_level, xy_log_dynamic_level());

    /* Restore original level */
    xy_log_set_dynamic_level(original_level);
}

void test_log_str(void)
{
    /* Test xy_log_str function */
    char *test_str = "Test log message";
    xy_log_str(test_str);
    /* If function executes without crash, test passes */
    TEST_ASSERT_TRUE(1);
}

void test_log_raw(void)
{
    /* Test xy_log_raw function */
    uint8_t test_data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    xy_log_raw(test_data, sizeof(test_data));
    /* If function executes without crash, test passes */
    TEST_ASSERT_TRUE(1);
}

/* ==================== Log Macro Tests ==================== */

void test_log_macros_compile(void)
{
    /* Test that log macros compile and execute */
    /* These tests verify the macros are properly defined */

    /* Error level logging */
    xy_log_e("Test error message");

    /* Warn level logging */
    xy_log_w("Test warn message");

    /* Info level logging */
    xy_log_i("Test info message");

    /* Debug level logging */
    xy_log_d("Test debug message");

    /* Verbose level logging */
    xy_log_v("Test verbose message");

    /* If we reach here, all macros compiled successfully */
    TEST_ASSERT_TRUE(1);
}

void test_log_macro_format_strings(void)
{
    /* Test log macros with format strings */
    int test_int = 42;
    const char *test_str = "hello";
    float test_float = 3.14f;

    xy_log_d("Integer: %d", test_int);
    xy_log_i("String: %s", test_str);
    xy_log_w("Float: %.2f", (double)test_float);
    xy_log_e("Multiple: %d %s %.2f", test_int, test_str, (double)test_float);

    TEST_ASSERT_TRUE(1);
}

/* ==================== Log Tag Tests ==================== */

void test_log_tag_defined(void)
{
    /* Test that XY_TAG is defined */
#ifdef XY_TAG
    TEST_ASSERT_TRUE(1);
#else
    TEST_FAIL_MESSAGE("XY_TAG is not defined");
#endif
}

/* ==================== Assert Tests ==================== */

void test_assert_macro_exists(void)
{
    /* Test that xy_assert macro is defined */
    /* Note: We don't actually trigger the assert in tests */
#ifdef xy_assert
    TEST_ASSERT_TRUE(1);
#else
    TEST_FAIL_MESSAGE("xy_assert macro is not defined");
#endif
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Log Level Tests */
    RUN_TEST(test_log_level_constants);
    RUN_TEST(test_local_log_level_defined);

    /* Log Function Tests */
    RUN_TEST(test_log_functions_exist);
    RUN_TEST(test_log_init);
    RUN_TEST(test_log_dynamic_level);
    RUN_TEST(test_log_str);
    RUN_TEST(test_log_raw);

    /* Log Macro Tests */
    RUN_TEST(test_log_macros_compile);
    RUN_TEST(test_log_macro_format_strings);

    /* Log Tag Tests */
    RUN_TEST(test_log_tag_defined);

    /* Assert Tests */
    RUN_TEST(test_assert_macro_exists);

    return UNITY_END();
}
