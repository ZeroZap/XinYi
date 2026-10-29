/**
 * @file test_xy_rtc_alarm_mask.c
 * @brief The alarm mask inversion, pinned against the defect it exists to prevent
 *
 * The cases read as the two backends' mistakes: a mask passed straight through (matching on the
 * fields the caller asked to ignore) and a mask never written at all (matching on everything).
 */

#include "unity.h"
#include "xy_rtc_alarm_mask.h"

void setUp(void) {}
void tearDown(void) {}

#define ALL XY_RTC_ALARM_FIELD_ALL

/* Asking for every field to match means nothing is skipped. */
static void test_requiring_everything_skips_nothing(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x00, xy_rtc_alarm_skip_from_match(ALL, ALL));
}

/* Asking for nothing to match means every field is skipped - the alarm fires every period. */
static void test_requiring_nothing_skips_everything(void)
{
    TEST_ASSERT_EQUAL_HEX8(ALL, xy_rtc_alarm_skip_from_match(0x00, ALL));
}

/* The inversion, field by field: asking for hours to match must skip the others, not hours. */
static void test_the_inversion_is_per_field(void)
{
    TEST_ASSERT_EQUAL_HEX8(XY_RTC_ALARM_FIELD_MINUTES | XY_RTC_ALARM_FIELD_SECONDS |
                               XY_RTC_ALARM_FIELD_WEEKDAY,
                           xy_rtc_alarm_skip_from_match(XY_RTC_ALARM_FIELD_HOURS, ALL));
    TEST_ASSERT_EQUAL_HEX8(ALL & ~XY_RTC_ALARM_FIELD_SECONDS,
                           xy_rtc_alarm_skip_from_match(XY_RTC_ALARM_FIELD_SECONDS, ALL));
    TEST_ASSERT_EQUAL_HEX8(ALL & ~XY_RTC_ALARM_FIELD_WEEKDAY,
                           xy_rtc_alarm_skip_from_match(XY_RTC_ALARM_FIELD_WEEKDAY, ALL));
}

/* Passing the mask through unchanged is the defect: it compares exactly what was asked to be ignored. */
static void test_passing_the_mask_through_would_compare_the_ignored_fields(void)
{
    uint8_t must_match = XY_RTC_ALARM_FIELD_HOURS;
    uint8_t passed_through = must_match;
    uint8_t correct_skip = xy_rtc_alarm_skip_from_match(must_match, ALL);

    TEST_ASSERT_TRUE(passed_through != correct_skip);
    /* Read as a skip mask, the caller's "hours must match" becomes "do not compare hours" - the one
     * field they asked for is the one field the hardware then ignores, and the alarm fires at the
     * first matching minute of every hour. The correct skip set never contains the field it requires
     * and always contains the ones it does not. */
    TEST_ASSERT_EQUAL_HEX8(XY_RTC_ALARM_FIELD_HOURS, passed_through);
    TEST_ASSERT_EQUAL_HEX8(0x00U, correct_skip & XY_RTC_ALARM_FIELD_HOURS);
    TEST_ASSERT_EQUAL_HEX8(XY_RTC_ALARM_FIELD_MINUTES, correct_skip & XY_RTC_ALARM_FIELD_MINUTES);
    TEST_ASSERT_EQUAL_HEX8(XY_RTC_ALARM_FIELD_SECONDS, correct_skip & XY_RTC_ALARM_FIELD_SECONDS);
}

static void test_every_mask_round_trips(void)
{
    int mask;

    for (mask = 0; mask <= (int)ALL; ++mask) {
        uint8_t skip = xy_rtc_alarm_skip_from_match((uint8_t)mask, ALL);

        TEST_ASSERT_EQUAL_HEX8((uint8_t)mask, xy_rtc_alarm_match_from_skip(skip, ALL));
    }
}

/* A backend that can only compare some fields must say so rather than quietly drop the others. */
static void test_a_mask_is_valid_only_for_the_fields_the_backend_has(void)
{
    TEST_ASSERT_TRUE(xy_rtc_alarm_mask_valid(XY_RTC_ALARM_FIELD_HOURS, ALL));
    TEST_ASSERT_TRUE(xy_rtc_alarm_mask_valid(0x00, ALL));
    TEST_ASSERT_TRUE(xy_rtc_alarm_mask_valid(ALL, ALL));
    /* A backend with hours and minutes only cannot honour a request to match seconds. */
    TEST_ASSERT_FALSE(xy_rtc_alarm_mask_valid(
        XY_RTC_ALARM_FIELD_SECONDS, XY_RTC_ALARM_FIELD_HOURS | XY_RTC_ALARM_FIELD_MINUTES));
    /* And the bits it does have are still fine. */
    TEST_ASSERT_TRUE(xy_rtc_alarm_mask_valid(
        XY_RTC_ALARM_FIELD_HOURS, XY_RTC_ALARM_FIELD_HOURS | XY_RTC_ALARM_FIELD_MINUTES));
}

/* Fields outside what the backend can compare are not reported as skipped - they never take part. */
static void test_fields_outside_the_backend_are_not_skipped(void)
{
    uint8_t hours_and_minutes = XY_RTC_ALARM_FIELD_HOURS | XY_RTC_ALARM_FIELD_MINUTES;

    TEST_ASSERT_EQUAL_HEX8(0x00, xy_rtc_alarm_skip_from_match(hours_and_minutes, hours_and_minutes));
    TEST_ASSERT_EQUAL_HEX8(XY_RTC_ALARM_FIELD_HOURS,
                           xy_rtc_alarm_skip_from_match(XY_RTC_ALARM_FIELD_MINUTES,
                                                        hours_and_minutes));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_requiring_everything_skips_nothing);
    RUN_TEST(test_requiring_nothing_skips_everything);
    RUN_TEST(test_the_inversion_is_per_field);
    RUN_TEST(test_passing_the_mask_through_would_compare_the_ignored_fields);
    RUN_TEST(test_every_mask_round_trips);
    RUN_TEST(test_a_mask_is_valid_only_for_the_fields_the_backend_has);
    RUN_TEST(test_fields_outside_the_backend_are_not_skipped);
    return UNITY_END();
}
