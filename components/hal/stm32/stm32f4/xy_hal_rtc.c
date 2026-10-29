/**
 * @file xy_hal_rtc_stm32.c
 * @brief RTC HAL STM32 Implementation
 * @version 1.0
 * @date 2025-10-26
 */

#include "../inc/xy_hal_rtc.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"
/* Shared calendar arithmetic; included by path so it resolves in every build shape. */
#include "../../../clib/xy_clib/inc/xy_time_civil.h"

static uint32_t xy_to_stm32_format(xy_hal_rtc_format_t format)
{
    return (format == XY_HAL_RTC_FORMAT_BIN) ? RTC_FORMAT_BIN : RTC_FORMAT_BCD;
}

xy_hal_error_t xy_hal_rtc_init(void *rtc)
{
    if (!rtc) {
        return XY_HAL_ERROR_FAIL;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)rtc;

    hrtc->Init.HourFormat     = RTC_HOURFORMAT_24;
    hrtc->Init.AsynchPrediv   = 127;
    hrtc->Init.SynchPrediv    = 255;
    hrtc->Init.OutPut         = RTC_OUTPUT_DISABLE;
    hrtc->Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc->Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(hrtc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_deinit(void *rtc)
{
    if (!rtc) {
        return XY_HAL_ERROR_FAIL;
    }

    if (HAL_RTC_DeInit((RTC_HandleTypeDef *)rtc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_time(void *rtc, const xy_hal_rtc_time_t *time,
                        xy_hal_rtc_format_t format)
{
    if (!rtc || !time) {
        return XY_HAL_ERROR_FAIL;
    }

    RTC_TimeTypeDef sTime = { 0 };
    sTime.Hours           = time->hours;
    sTime.Minutes         = time->minutes;
    sTime.Seconds         = time->seconds;
    sTime.SubSeconds      = time->subseconds;
    sTime.DayLightSaving  = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation  = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(
            (RTC_HandleTypeDef *)rtc, &sTime, xy_to_stm32_format(format))
        != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *time,
                        xy_hal_rtc_format_t format)
{
    if (!rtc || !time) {
        return XY_HAL_ERROR_FAIL;
    }

    RTC_TimeTypeDef sTime = { 0 };

    if (HAL_RTC_GetTime(
            (RTC_HandleTypeDef *)rtc, &sTime, xy_to_stm32_format(format))
        != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    time->hours      = sTime.Hours;
    time->minutes    = sTime.Minutes;
    time->seconds    = sTime.Seconds;
    time->subseconds = sTime.SubSeconds;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_date(void *rtc, const xy_hal_rtc_date_t *date,
                        xy_hal_rtc_format_t format)
{
    if (!rtc || !date) {
        return XY_HAL_ERROR_FAIL;
    }

    RTC_DateTypeDef sDate = { 0 };
    sDate.WeekDay         = date->weekday;
    sDate.Month           = date->month;
    sDate.Date            = date->date;
    sDate.Year            = date->year % 100; /* STM32 uses 2-digit year */

    if (HAL_RTC_SetDate(
            (RTC_HandleTypeDef *)rtc, &sDate, xy_to_stm32_format(format))
        != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_rtc_get_date(void *rtc, xy_hal_rtc_date_t *date,
                        xy_hal_rtc_format_t format)
{
    if (!rtc || !date) {
        return XY_HAL_ERROR_FAIL;
    }

    RTC_DateTypeDef sDate = { 0 };

    if (HAL_RTC_GetDate(
            (RTC_HandleTypeDef *)rtc, &sDate, xy_to_stm32_format(format))
        != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    date->weekday = sDate.WeekDay;
    date->month   = sDate.Month;
    date->date    = sDate.Date;
    date->year    = sDate.Year + 2000; /* Convert back to 4-digit */

    return XY_HAL_OK;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_rtc_set_alarm(void *rtc, const xy_hal_rtc_alarm_t *alarm,
                         char alarm_id)
{
    if (!rtc || !alarm) {
        return XY_HAL_ERROR_FAIL;
    }

    RTC_AlarmTypeDef sAlarm     = { 0 };
    sAlarm.AlarmTime.Hours      = alarm->time.hours;
    sAlarm.AlarmTime.Minutes    = alarm->time.minutes;
    sAlarm.AlarmTime.Seconds    = alarm->time.seconds;
    sAlarm.AlarmTime.SubSeconds = alarm->time.subseconds;
    sAlarm.AlarmDateWeekDay     = alarm->date;
    sAlarm.Alarm                = (alarm_id == 'A') ? RTC_ALARM_A : RTC_ALARM_B;

    if (HAL_RTC_SetAlarm((RTC_HandleTypeDef *)rtc, &sAlarm, RTC_FORMAT_BIN)
        != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_alarm(void *rtc, xy_hal_rtc_alarm_t *alarm, char alarm_id)
{
    if (!rtc || !alarm) {
        return XY_HAL_ERROR_FAIL;
    }

    RTC_AlarmTypeDef sAlarm = { 0 };
    sAlarm.Alarm            = (alarm_id == 'A') ? RTC_ALARM_A : RTC_ALARM_B;

    if (HAL_RTC_GetAlarm(
            (RTC_HandleTypeDef *)rtc, &sAlarm, sAlarm.Alarm, RTC_FORMAT_BIN)
        != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    alarm->time.hours      = sAlarm.AlarmTime.Hours;
    alarm->time.minutes    = sAlarm.AlarmTime.Minutes;
    alarm->time.seconds    = sAlarm.AlarmTime.Seconds;
    alarm->time.subseconds = sAlarm.AlarmTime.SubSeconds;
    alarm->date            = sAlarm.AlarmDateWeekDay;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_enable_alarm(void *rtc, char alarm_id)
{
    if (!rtc) {
        return XY_HAL_ERROR_FAIL;
    }

    RTC_AlarmTypeDef sAlarm = { 0 };
    sAlarm.Alarm            = (alarm_id == 'A') ? RTC_ALARM_A : RTC_ALARM_B;

    if (HAL_RTC_SetAlarm_IT((RTC_HandleTypeDef *)rtc, &sAlarm, RTC_FORMAT_BIN)
        != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_disable_alarm(void *rtc, char alarm_id)
{
    if (!rtc) {
        return XY_HAL_ERROR_FAIL;
    }

    uint32_t alarm = (alarm_id == 'A') ? RTC_ALARM_A : RTC_ALARM_B;

    if (HAL_RTC_DeactivateAlarm((RTC_HandleTypeDef *)rtc, alarm) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_register_callback(void *rtc, xy_hal_rtc_event_t event,
                                 xy_hal_rtc_callback_t callback, void *arg)
{
    /* Store callback in context */
    return XY_HAL_OK;
}

int64_t xy_hal_rtc_get_timestamp(void *rtc)
{
    if (!rtc) {
        return XY_HAL_ERROR_FAIL;
    }

    xy_hal_rtc_time_t time;
    xy_hal_rtc_date_t date;

    if (xy_hal_rtc_get_time(rtc, &time, XY_HAL_RTC_FORMAT_BIN) != XY_HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    if (xy_hal_rtc_get_date(rtc, &date, XY_HAL_RTC_FORMAT_BIN) != XY_HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* This built a struct tm and called mktime(). On a bare-metal target that is newlib's mktime,
     * which reads the struct as *local* time, so the answer depends on a TZ nobody sets; it also
     * returns -1 for an error, indistinguishable from a real one second before the epoch. The shared
     * arithmetic is exact, has no timezone and has no failure mode. */
    return xy_civil_seconds_from_epoch((int)date.year, (unsigned int)date.month,
                                       (unsigned int)date.date, (unsigned int)time.hours,
                                       (unsigned int)time.minutes, (unsigned int)time.seconds);
}

xy_hal_error_t xy_hal_rtc_set_timestamp(void *rtc, int64_t timestamp)
{
    if (!rtc) {
        return XY_HAL_ERROR_FAIL;
    }

    int64_t days;
    int64_t rest;
    int year = 0;
    unsigned int month = 0;
    unsigned int day = 0;

    /* Same reasoning as the read path: localtime() reads a TZ nobody sets and can return NULL for a
     * value it cannot represent, which the old code dereferenced without checking. */
    if (timestamp < 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    days = timestamp / 86400;
    rest = timestamp % 86400;
    xy_civil_from_days(days, &year, &month, &day);

    xy_hal_rtc_time_t time = { .hours      = (uint8_t)(rest / 3600),
                               .minutes    = (uint8_t)((rest % 3600) / 60),
                               .seconds    = (uint8_t)(rest % 60),
                               .subseconds = 0 };

    xy_hal_rtc_date_t date = { .weekday = (uint8_t)xy_civil_weekday_from_days(days),
                               .month   = (uint8_t)month,
                               .date    = (uint8_t)day,
                               .year    = (uint16_t)year };

    if (xy_hal_rtc_set_time(rtc, &time, XY_HAL_RTC_FORMAT_BIN) != XY_HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    if (xy_hal_rtc_set_date(rtc, &date, XY_HAL_RTC_FORMAT_BIN) != XY_HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

#endif /* STM32_HAL_ENABLED */
