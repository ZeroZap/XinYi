/**
 * @file test_xy_time_civil.c
 * @brief Calendar arithmetic, checked against dates that can be verified by hand
 *
 * The reason this file exists: the STM32U5 RTC backend turned a calendar date into a timestamp by
 * treating every month as 30 days and every year as 365. That is wrong by weeks and nothing noticed,
 * because no test ever compared its output against a known date. Every case below is a date whose
 * Unix timestamp is independently known.
 */

#include "unity.h"
#include "xy_time_civil.h"

void setUp(void) {}
void tearDown(void) {}

/* Epoch, and the day either side of it, where sign handling goes wrong. */
static void test_the_epoch_is_day_zero(void)
{
    TEST_ASSERT_EQUAL_INT64(0, xy_civil_days_from_epoch(1970, 1, 1));
    TEST_ASSERT_EQUAL_INT64(-1, xy_civil_days_from_epoch(1969, 12, 31));
    TEST_ASSERT_EQUAL_INT64(1, xy_civil_days_from_epoch(1970, 1, 2));
    TEST_ASSERT_EQUAL_INT64(-365, xy_civil_days_from_epoch(1969, 1, 1));
}

/* Dates whose timestamps are worth knowing by heart. */
static void test_known_dates(void)
{
    TEST_ASSERT_EQUAL_INT64(11017, xy_civil_days_from_epoch(2000, 3, 1));
    TEST_ASSERT_EQUAL_INT64(365, xy_civil_days_from_epoch(1971, 1, 1));
    TEST_ASSERT_EQUAL_INT64(10957, xy_civil_days_from_epoch(2000, 1, 1));
    TEST_ASSERT_EQUAL_INT64(19723, xy_civil_days_from_epoch(2024, 1, 1));
    TEST_ASSERT_EQUAL_INT64(20423, xy_civil_days_from_epoch(2025, 12, 1));
    TEST_ASSERT_EQUAL_INT64(20753, xy_civil_days_from_epoch(2026, 10, 27));
}

/* A 30-day month and a 365-day year give the same answer here; they must not. */
static void test_months_are_not_thirty_days(void)
{
    int64_t jan31 = xy_civil_days_from_epoch(2026, 1, 31);
    int64_t mar1 = xy_civil_days_from_epoch(2026, 3, 1);

    /* February 2026 has 28 days, so 31 January to 1 March is 29 days, not 31. */
    TEST_ASSERT_EQUAL_INT64(jan31 + 1, xy_civil_days_from_epoch(2026, 2, 1));
    TEST_ASSERT_EQUAL_INT64(jan31 + 29, mar1);
    TEST_ASSERT_EQUAL_INT64(jan31 + 31, xy_civil_days_from_epoch(2026, 3, 3));
    /* March through December is 306 days, not 300. */
    TEST_ASSERT_EQUAL_INT64(306, xy_civil_days_from_epoch(2026, 12, 31) - mar1 + 1);
}

/* Leap years: divisible by 4, not by 100 unless by 400. */
static void test_leap_years(void)
{
    TEST_ASSERT_EQUAL_INT64(366, xy_civil_days_from_epoch(2024, 12, 31) -
                                     xy_civil_days_from_epoch(2023, 12, 31));
    TEST_ASSERT_EQUAL_INT64(365, xy_civil_days_from_epoch(2025, 12, 31) -
                                     xy_civil_days_from_epoch(2024, 12, 31));
    TEST_ASSERT_EQUAL_INT64(365, xy_civil_days_from_epoch(1900, 12, 31) -
                                     xy_civil_days_from_epoch(1899, 12, 31));
    TEST_ASSERT_EQUAL_INT64(366, xy_civil_days_from_epoch(2000, 12, 31) -
                                     xy_civil_days_from_epoch(1999, 12, 31));
    /* 29 February 2024 exists; the day after 28 February is 29 February. */
    TEST_ASSERT_EQUAL_INT64(xy_civil_days_from_epoch(2024, 2, 28) + 1,
                            xy_civil_days_from_epoch(2024, 2, 29));
}

static void test_seconds_include_the_time_of_day(void)
{
    TEST_ASSERT_EQUAL_INT64(0, xy_civil_seconds_from_epoch(1970, 1, 1, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT64(86399, xy_civil_seconds_from_epoch(1970, 1, 1, 23, 59, 59));
    TEST_ASSERT_EQUAL_INT64(86400, xy_civil_seconds_from_epoch(1970, 1, 2, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT64(1704067200, xy_civil_seconds_from_epoch(2024, 1, 1, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT64(1793059200, xy_civil_seconds_from_epoch(2026, 10, 27, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT64(1793059200 + 3661,
                            xy_civil_seconds_from_epoch(2026, 10, 27, 1, 1, 1));
}

static void test_the_inverse_round_trips(void)
{
    int year = 0;
    unsigned int month = 0;
    unsigned int day = 0;
    int64_t days;

    for (days = -2000; days <= 30000; days += 7) {
        xy_civil_from_days(days, &year, &month, &day);
        TEST_ASSERT_EQUAL_INT64(days, xy_civil_days_from_epoch(year, month, day));
    }
}

static void test_the_inverse_on_known_dates(void)
{
    int year = 0;
    unsigned int month = 0;
    unsigned int day = 0;

    xy_civil_from_days(0, &year, &month, &day);
    TEST_ASSERT_EQUAL_INT(1970, year);
    TEST_ASSERT_EQUAL_UINT(1, month);
    TEST_ASSERT_EQUAL_UINT(1, day);

    xy_civil_from_days(11017, &year, &month, &day);
    TEST_ASSERT_EQUAL_INT(2000, year);
    TEST_ASSERT_EQUAL_UINT(3, month);
    TEST_ASSERT_EQUAL_UINT(1, day);

    xy_civil_from_days(-1, &year, &month, &day);
    TEST_ASSERT_EQUAL_INT(1969, year);
    TEST_ASSERT_EQUAL_UINT(12, month);
    TEST_ASSERT_EQUAL_UINT(31, day);
}

/* 1970-01-01 was a Thursday. */
static void test_weekday(void)
{
    TEST_ASSERT_EQUAL_UINT(4, xy_civil_weekday_from_days(0));
    TEST_ASSERT_EQUAL_UINT(4, xy_civil_weekday_from_days(7));
    TEST_ASSERT_EQUAL_UINT(5, xy_civil_weekday_from_days(1));
    TEST_ASSERT_EQUAL_UINT(3, xy_civil_weekday_from_days(-1));
    /* 2026-10-27 is a Tuesday. */
    TEST_ASSERT_EQUAL_UINT(2, xy_civil_weekday_from_days(xy_civil_days_from_epoch(2026, 10, 27)));
}

/*
 * The formula the STM32U5 backend shipped, kept here so the size of the error is on the record.
 * It is not merely approximate: it multiplies (year - 70) by 365 days, which treats the years elapsed
 * since 1970 as if the year number itself were the count. The result is not a Unix timestamp at all -
 * it lands about 1900 years away from the correct answer, and it was returned as one.
 */
static void test_the_old_formula_was_not_a_timestamp(void)
{
    int64_t approximate = (int64_t)(2026 - 70) * 365 * 24 * 3600 +
                          (int64_t)(10 - 1) * 30 * 24 * 3600 +
                          (int64_t)(27 - 1) * 24 * 3600;
    int64_t exact = xy_civil_seconds_from_epoch(2026, 10, 27, 0, 0, 0);

    TEST_ASSERT_TRUE(approximate != exact);
    /* Off by more than a century of seconds, either way - that is what "not a timestamp" means. */
    TEST_ASSERT_TRUE(approximate - exact > 100 * 365 * XY_SECONDS_PER_DAY);
    /* And the 30-day month error on its own is still weeks, which is what a reader would notice. */
    TEST_ASSERT_TRUE(xy_civil_days_from_epoch(2026, 12, 31) -
                     xy_civil_days_from_epoch(2026, 1, 1) > 11 * 30);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_the_epoch_is_day_zero);
    RUN_TEST(test_known_dates);
    RUN_TEST(test_months_are_not_thirty_days);
    RUN_TEST(test_leap_years);
    RUN_TEST(test_seconds_include_the_time_of_day);
    RUN_TEST(test_the_inverse_round_trips);
    RUN_TEST(test_the_inverse_on_known_dates);
    RUN_TEST(test_weekday);
    RUN_TEST(test_the_old_formula_was_not_a_timestamp);
    return UNITY_END();
}
