/**
 * @file xy_time_civil.h
 * @brief Civil date arithmetic shared by every backend that reads a calendar
 *
 * Header-only on purpose: it is a few pure functions with no state, and a header cannot go missing
 * from a build's source list. The alternative - a .c in the clib library, which globs its sources -
 * depends on every build directory being re-configured after the file appears, and a symbol that is
 * silently absent at link time is worse than a duplicated few dozen bytes of code.
 *
 * Why this exists: the STM32U5 RTC backend converted a calendar date to seconds by treating every
 * month as 30 days and every year as 365. That answer is plausible - it has the right magnitude, it
 * moves forward, it looks like a timestamp - and it is wrong by weeks. Nothing caught it because
 * nothing ever compared a timestamp against a known date. These functions are small enough to test
 * exhaustively on the host, which is where that comparison belongs.
 *
 * All arithmetic is IEEE-754-free integer arithmetic on the proleptic Gregorian calendar, with
 * 1970-01-01 as day zero, matching the Unix epoch.
 */

#ifndef XY_TIME_CIVIL_H
#define XY_TIME_CIVIL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XY_SECONDS_PER_DAY 86400

/**
 * @brief Days from 1970-01-01 to the given civil date.
 *
 * @param year  Full year (1970, 2026, ...)
 * @param month 1-12
 * @param day   1-31
 * @return Days since the epoch; negative before 1970. Out-of-range months are still total (the
 *         calculation rolls them over) rather than undefined, so a caller that clamps nothing gets a
 *         wrong answer that a test can compare, not a crash.
 */
static inline int64_t xy_civil_days_from_epoch(int year, unsigned int month, unsigned int day)
{
    int era;
    unsigned int year_of_era;
    unsigned int day_of_year;
    unsigned int day_of_era;

    year -= (month <= 2U) ? 1 : 0;
    era = (year >= 0 ? year : year - 399) / 400;
    year_of_era = (unsigned int)(year - era * 400);
    day_of_year = (153U * (month + (month > 2U ? (unsigned int)-3 : 9U)) + 2U) / 5U + day - 1U;
    day_of_era = year_of_era * 365U + year_of_era / 4U - year_of_era / 100U + day_of_year;
    return (int64_t)era * 146097 + (int64_t)day_of_era - 719468;
}

/**
 * @brief Seconds from 1970-01-01T00:00:00 to the given civil date and time of day.
 *
 * The time of day is not range-checked: a caller that passes 24 hours gets tomorrow at midnight,
 * which is arithmetic rather than an error, and clamping here would hide the caller's mistake.
 */
static inline int64_t xy_civil_seconds_from_epoch(int year, unsigned int month, unsigned int day,
                                                  unsigned int hour, unsigned int minute,
                                                  unsigned int second)
{
    return xy_civil_days_from_epoch(year, month, day) * XY_SECONDS_PER_DAY +
           (int64_t)hour * 3600 + (int64_t)minute * 60 + (int64_t)second;
}

/**
 * @brief Civil date for a count of days from 1970-01-01.
 *
 * The inverse of xy_civil_days_from_epoch(); present because a backend that could not write a
 * timestamp did so for want of this, and returning the weekday alongside a date is the other reason.
 */
static inline void xy_civil_from_days(int64_t days, int *year, unsigned int *month,
                                      unsigned int *day)
{
    int64_t era_days = days + 719468;
    int era = (int)((era_days >= 0 ? era_days : era_days - 146096) / 146097);
    unsigned int day_of_era = (unsigned int)(era_days - (int64_t)era * 146097);
    unsigned int year_of_era =
        (day_of_era - day_of_era / 1460U + day_of_era / 36524U - day_of_era / 146096U) / 365U;
    unsigned int day_of_year = day_of_era - (365U * year_of_era + year_of_era / 4U -
                                             year_of_era / 100U);
    unsigned int month_of_year = (5U * day_of_year + 2U) / 153U;
    unsigned int day_of_month = day_of_year - (153U * month_of_year + 2U) / 5U + 1U;
    unsigned int month_out = (month_of_year < 10U) ? month_of_year + 3U : month_of_year - 9U;
    int year_out = (int)year_of_era + era * 400;

    year_out += (month_out <= 2U) ? 1 : 0;
    if (year != NULL) {
        *year = year_out;
    }
    if (month != NULL) {
        *month = month_out;
    }
    if (day != NULL) {
        *day = day_of_month;
    }
}

/**
 * @brief Day of week for a count of days from the epoch, 1 = Monday through 7 = Sunday.
 *
 * 1970-01-01 was a Thursday, which is the only fact needed: (days + 3) makes Thursday land on 4.
 */
static inline unsigned int xy_civil_weekday_from_days(int64_t days)
{
    int64_t index = (days + 3) % 7;

    if (index < 0) {
        index += 7;
    }
    return (unsigned int)index + 1U;
}

#ifdef __cplusplus
}
#endif

#endif /* XY_TIME_CIVIL_H */
