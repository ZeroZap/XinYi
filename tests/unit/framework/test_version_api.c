/**
 * @file test_version_api.c
 * @brief The release macros, the string, and the shipped implementation must agree.
 *
 * `components/xy_version.h` is the checked mirror that `tools/scripts/check_release_facts.py`
 * compares against `VERSION`. That checker reads the header; nothing compared the *header*
 * against the *implementation* in `components/xy_version.c`, which no target compiled at all.
 * These cases pin the three facts to each other and pin the direction of the compatibility
 * gate, which is the one place where a wrong answer silently grants an API that does not exist.
 */

#include "unity.h"

#include <stdio.h>

#include "xy_version.h"

void setUp(void)
{
}

void tearDown(void)
{
}

/* Bumping one macro and forgetting the others is the ordinary way these drift apart. */
static void test_the_string_and_the_numeric_macros_describe_the_same_version(void)
{
    unsigned int major = 0U;
    unsigned int minor = 0U;
    unsigned int patch = 0U;

    TEST_ASSERT_EQUAL_INT(3, sscanf(XY_VERSION_STRING, "%u.%u.%u", &major, &minor, &patch));
    TEST_ASSERT_EQUAL_UINT((unsigned int)XY_VERSION_MAJOR, major);
    TEST_ASSERT_EQUAL_UINT((unsigned int)XY_VERSION_MINOR, minor);
    TEST_ASSERT_EQUAL_UINT((unsigned int)XY_VERSION_PATCH, patch);
}

static void test_the_packed_number_matches_the_component_macros(void)
{
    TEST_ASSERT_EQUAL_UINT(((unsigned int)XY_VERSION_MAJOR << 16) |
                               ((unsigned int)XY_VERSION_MINOR << 8) |
                               (unsigned int)XY_VERSION_PATCH,
                           (unsigned int)XY_VERSION_NUMBER);
}

static void test_the_comparison_macros_answer_from_the_packed_number(void)
{
    TEST_ASSERT_TRUE(XY_VERSION_AT_LEAST(XY_VERSION_MAJOR, XY_VERSION_MINOR, XY_VERSION_PATCH));
    TEST_ASSERT_TRUE(XY_VERSION_IS(XY_VERSION_MAJOR, XY_VERSION_MINOR, XY_VERSION_PATCH));
    TEST_ASSERT_TRUE(XY_VERSION_IS_COMPATIBLE(XY_VERSION_MAJOR));

    TEST_ASSERT_FALSE(XY_VERSION_IS(XY_VERSION_MAJOR, XY_VERSION_MINOR, XY_VERSION_PATCH + 1));
    TEST_ASSERT_FALSE(XY_VERSION_AT_LEAST(XY_VERSION_MAJOR, XY_VERSION_MINOR, XY_VERSION_PATCH + 1));
    TEST_ASSERT_FALSE(XY_VERSION_IS_COMPATIBLE(XY_VERSION_MAJOR + 1));
}

/* The getters must report the macros the consumer compiled against, not a second copy. */
static void test_the_getters_report_the_compiled_macros(void)
{
    TEST_ASSERT_EQUAL_STRING(XY_VERSION_STRING, xy_version_get_string());
    TEST_ASSERT_EQUAL_INT(XY_VERSION_MAJOR, xy_version_get_major());
    TEST_ASSERT_EQUAL_INT(XY_VERSION_MINOR, xy_version_get_minor());
    TEST_ASSERT_EQUAL_INT(XY_VERSION_PATCH, xy_version_get_patch());
}

static void test_an_exact_version_request_is_compatible(void)
{
    TEST_ASSERT_EQUAL_INT(1, xy_version_check_compatibility(XY_VERSION_MAJOR, XY_VERSION_MINOR,
                                                           XY_VERSION_PATCH));
}

static void test_an_older_request_is_compatible(void)
{
    if (XY_VERSION_MINOR > 0) {
        TEST_ASSERT_EQUAL_INT(
            1, xy_version_check_compatibility(XY_VERSION_MAJOR, XY_VERSION_MINOR - 1, 0));
    }
    if (XY_VERSION_PATCH > 0) {
        TEST_ASSERT_EQUAL_INT(1, xy_version_check_compatibility(XY_VERSION_MAJOR, XY_VERSION_MINOR,
                                                                XY_VERSION_PATCH - 1));
    }
}

/* A request this build cannot satisfy must not be answered "compatible": that is the
 * direction that hands a caller an API which does not exist in the binary it linked. */
static void test_a_newer_request_is_refused(void)
{
    TEST_ASSERT_EQUAL_INT(0, xy_version_check_compatibility(XY_VERSION_MAJOR, XY_VERSION_MINOR,
                                                           XY_VERSION_PATCH + 1));
    TEST_ASSERT_EQUAL_INT(0, xy_version_check_compatibility(XY_VERSION_MAJOR, XY_VERSION_MINOR + 1,
                                                           0));
    TEST_ASSERT_EQUAL_INT(0, xy_version_check_compatibility(XY_VERSION_MAJOR + 1, 0, 0));
}

static void test_a_different_or_negative_major_is_refused(void)
{
    TEST_ASSERT_EQUAL_INT(0, xy_version_check_compatibility(XY_VERSION_MAJOR - 1, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, xy_version_check_compatibility(-1, 0, 0));
}

/* Negative minor/patch are not "older", they are not versions at all. */
static void test_negative_fields_are_refused(void)
{
    TEST_ASSERT_EQUAL_INT(0, xy_version_check_compatibility(XY_VERSION_MAJOR, -1, 0));
    TEST_ASSERT_EQUAL_INT(0, xy_version_check_compatibility(XY_VERSION_MAJOR, XY_VERSION_MINOR, -1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_the_string_and_the_numeric_macros_describe_the_same_version);
    RUN_TEST(test_the_packed_number_matches_the_component_macros);
    RUN_TEST(test_the_comparison_macros_answer_from_the_packed_number);
    RUN_TEST(test_the_getters_report_the_compiled_macros);
    RUN_TEST(test_an_exact_version_request_is_compatible);
    RUN_TEST(test_an_older_request_is_compatible);
    RUN_TEST(test_a_newer_request_is_refused);
    RUN_TEST(test_a_different_or_negative_major_is_refused);
    RUN_TEST(test_negative_fields_are_refused);
    return UNITY_END();
}
