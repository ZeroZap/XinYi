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
#include "../../../clib/xy_clib/inc/xy_bcd.h"
#include "../../../clib/xy_clib/inc/xy_rtc_alarm_mask.h"
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

    if (format != XY_HAL_RTC_FORMAT_BIN && format != XY_HAL_RTC_FORMAT_BCD) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    {
        /* This wrote the caller's bytes straight into the register whatever the format, with the year
         * reduced to two digits by truncation - so a BCD caller put binary digits in a register the
         * HAL reads as BCD, and a year outside the century became a different year silently. */
        uint8_t month = date->month;
        uint8_t day = date->date;
        uint8_t weekday = date->weekday;
        uint16_t year = date->year;
        int year_register;

        if (format == XY_HAL_RTC_FORMAT_BCD) {
            if (!xy_bcd_decode8(date->month, &month) || !xy_bcd_decode8(date->date, &day) ||
                !xy_bcd_decode8(date->weekday, &weekday) || !xy_bcd_decode16(date->year, &year)) {
                return XY_HAL_ERROR_INVALID_PARAM;
            }
        }
        if (month < 1U || month > 12U || day < 1U || day > 31U || weekday > 7U) {
            return XY_HAL_ERROR_INVALID_PARAM;
        }
        year_register = xy_rtc_year_encode(year, 1);
        if (year_register < 0) {
            return XY_HAL_ERROR_INVALID_PARAM;
        }
        sDate.WeekDay = weekday;
        sDate.Month   = xy_bcd_encode8(month);
        sDate.Date    = xy_bcd_encode8(day);
        sDate.Year    = (uint8_t)year_register;
    }

    if (HAL_RTC_SetDate((RTC_HandleTypeDef *)rtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_date(void *rtc, xy_hal_rtc_date_t *date,
                        xy_hal_rtc_format_t format)
{
    if (!rtc || !date) {
        return XY_HAL_ERROR_FAIL;
    }

    RTC_DateTypeDef sDate = { 0 };

    if (format != XY_HAL_RTC_FORMAT_BIN && format != XY_HAL_RTC_FORMAT_BCD) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    /* Read the registers as BCD whatever the caller asked for: the conversion happens once, below. */
    if (HAL_RTC_GetDate((RTC_HandleTypeDef *)rtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    {
        /* Adding 2000 to whatever the register held was wrong in BCD mode - 0x26 read as a number is
         * 38, so the year came back as 2038 - and the dead return after the real one meant nobody
         * noticed. The register is always read as BCD and converted once. */
        int year = xy_rtc_year_decode((uint8_t)((sDate.Year <= 99U) ? sDate.Year : 0xFFU), 1);
        uint8_t month = 0;
        uint8_t day = 0;

        if (year < 0 || !xy_bcd_decode8(sDate.Month, &month) || !xy_bcd_decode8(sDate.Date, &day)) {
            return XY_HAL_ERROR_FAIL;
        }
        if (format == XY_HAL_RTC_FORMAT_BIN) {
            date->weekday = sDate.WeekDay;
            date->month   = month;
            date->date    = day;
            date->year    = (uint16_t)year;
        } else {
            date->weekday = sDate.WeekDay;
            date->month   = sDate.Month;
            date->date    = sDate.Date;
            date->year    = xy_bcd_encode16((uint16_t)year);
        }
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_alarm(void *rtc, const xy_hal_rtc_alarm_t *alarm,
                         char alarm_id)
{
    if (!rtc || !alarm) {
        return XY_HAL_ERROR_FAIL;
    }

    RTC_AlarmTypeDef sAlarm     = { 0 };

    /* Any id that was not 'A' used to become alarm B, so a caller who passed 'C' silently reprogrammed
     * the other alarm. */
    if (alarm_id != 'A' && alarm_id != 'B') {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (!xy_rtc_alarm_mask_valid(alarm->alarm_mask, XY_HAL_RTC_ALARM_MASK_ALL)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    sAlarm.AlarmTime.Hours      = alarm->time.hours;
    sAlarm.AlarmTime.Minutes    = alarm->time.minutes;
    sAlarm.AlarmTime.Seconds    = alarm->time.seconds;
    sAlarm.AlarmTime.SubSeconds = alarm->time.subseconds;
    sAlarm.AlarmDateWeekDay     = alarm->date;
    sAlarm.Alarm                = (alarm_id == 'A') ? RTC_ALARM_A : RTC_ALARM_B;
    {
        /* The mask was never written here, which left the vendor default of zero - every field
         * compared - so a caller asking to ignore seconds got an alarm that matched them. */
        uint8_t skip = xy_rtc_alarm_skip_from_match(alarm->alarm_mask,
                                                    XY_HAL_RTC_ALARM_MASK_ALL);

        if ((skip & XY_HAL_RTC_ALARM_MASK_HOURS) != 0U) {
            sAlarm.AlarmMask |= RTC_ALARMMASK_HOURS;
        }
        if ((skip & XY_HAL_RTC_ALARM_MASK_MINUTES) != 0U) {
            sAlarm.AlarmMask |= RTC_ALARMMASK_MINUTES;
        }
        if ((skip & XY_HAL_RTC_ALARM_MASK_SECONDS) != 0U) {
            sAlarm.AlarmMask |= RTC_ALARMMASK_SECONDS;
        }
        if ((skip & XY_HAL_RTC_ALARM_MASK_WEEKDAY) != 0U) {
            sAlarm.AlarmMask |= RTC_ALARMMASK_DATEWEEKDAY;
            sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
        } else {
            sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY;
        }
    }

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

    if (alarm_id != 'A' && alarm_id != 'B') {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    sAlarm.Alarm = (alarm_id == 'A') ? RTC_ALARM_A : RTC_ALARM_B;

    if (HAL_RTC_GetAlarm((RTC_HandleTypeDef *)rtc, &sAlarm, sAlarm.Alarm, RTC_FORMAT_BIN)
        != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    alarm->time.hours      = sAlarm.AlarmTime.Hours;
    alarm->time.minutes    = sAlarm.AlarmTime.Minutes;
    alarm->time.seconds    = sAlarm.AlarmTime.Seconds;
    alarm->time.subseconds = sAlarm.AlarmTime.SubSeconds;
    alarm->date            = (uint8_t)sAlarm.AlarmDateWeekDay;
    alarm->weekday         = (uint8_t)sAlarm.AlarmDateWeekDay;
    {
        /* The mask was never read back, so the caller's struct kept whatever it arrived with. And it
         * has to be expressed the way the caller spoke: fields that must match. */
        uint8_t skip = 0U;

        if ((sAlarm.AlarmMask & RTC_ALARMMASK_HOURS) != 0U) {
            skip |= XY_HAL_RTC_ALARM_MASK_HOURS;
        }
        if ((sAlarm.AlarmMask & RTC_ALARMMASK_MINUTES) != 0U) {
            skip |= XY_HAL_RTC_ALARM_MASK_MINUTES;
        }
        if ((sAlarm.AlarmMask & RTC_ALARMMASK_SECONDS) != 0U) {
            skip |= XY_HAL_RTC_ALARM_MASK_SECONDS;
        }
        if ((sAlarm.AlarmMask & RTC_ALARMMASK_DATEWEEKDAY) != 0U) {
            skip |= XY_HAL_RTC_ALARM_MASK_WEEKDAY;
        }
        alarm->alarm_mask = xy_rtc_alarm_match_from_skip(skip, XY_HAL_RTC_ALARM_MASK_ALL);
    }

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

/*
 * Backup registers. The STM32L4 wrapper is the reference here: the same pair of calls, the same
 * bounds check, and the backup-domain access enabled before a write. Reading needs no unlock;
 * writing does, and forgetting it silently drops the value while returning success - which is why
 * this was left to the backend that has the domain wiring right rather than invented again.
 */
xy_hal_error_t xy_hal_rtc_backup_read(uint32_t index, uint32_t *value)
{
    RTC_HandleTypeDef backup_rtc = { 0 };

    if (value == NULL || index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    backup_rtc.Instance = RTC;
    *value = HAL_RTCEx_BKUPRead(&backup_rtc, index);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_backup_write(uint32_t index, uint32_t value)
{
    RTC_HandleTypeDef backup_rtc = { 0 };

    if (index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    backup_rtc.Instance = RTC;
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&backup_rtc, index, value);
    return XY_HAL_OK;
}

#endif /* STM32_HAL_ENABLED */
