/**
 * @file xy_hal_rtc.c
 * @brief RTC HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_rtc.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
/* Shared calendar arithmetic. Included by path so it resolves in every build shape - the HAL
 * component and the board targets do not agree on include directories. */
#include "../../../clib/xy_clib/inc/xy_time_civil.h"
#include "../../../clib/xy_clib/inc/xy_bcd.h"
#include <string.h>

/* RTC context structure */
typedef struct {
    RTC_HandleTypeDef *hrtc;
    xy_hal_rtc_callback_t callback;
    void *arg;
    uint8_t initialized;
} rtc_ctx_t;

/* Maximum RTC instances (typically only 1) */
#define MAX_RTC_INSTANCES 2
static rtc_ctx_t g_rtc_ctx[MAX_RTC_INSTANCES];

/**
 * @brief Find RTC context by handle
 */
static rtc_ctx_t *find_rtc_ctx(void *rtc)
{
    for (size_t i = 0; i < MAX_RTC_INSTANCES; i++) {
        if (g_rtc_ctx[i].hrtc == rtc) {
            return &g_rtc_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate new RTC context
 */
static rtc_ctx_t *alloc_rtc_ctx(void)
{
    for (size_t i = 0; i < MAX_RTC_INSTANCES; i++) {
        if (g_rtc_ctx[i].hrtc == NULL) {
            return &g_rtc_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert binary to BCD
 */
static uint8_t bin_to_bcd(uint8_t value)
{
    return ((value / 10) << 4) | (value % 10);
}

/**
 * @brief Convert BCD to binary
 */
static uint8_t bcd_to_bin(uint8_t value)
{
    return ((value >> 4) * 10) + (value & 0x0F);
}

xy_hal_error_t xy_hal_rtc_init(void *rtc)
{
    if (!rtc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)rtc;

    /* Check if already initialized */
    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    /* Allocate context if not exists */
    if (ctx == NULL) {
        ctx = alloc_rtc_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    /* Configure RTC */
    hrtc->Init.HourFormat     = RTC_HOURFORMAT_24;
    hrtc->Init.AsynchPrediv   = RTC_ASYNCH_PREDIV;
    hrtc->Init.SynchPrediv    = RTC_SYNCH_PREDIV;
    hrtc->Init.OutPut         = RTC_OUTPUT_DISABLE;
    hrtc->Init.OutPutRemap    = RTC_OUTPUT_REMAP_NONE;
    hrtc->Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc->Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(hrtc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->hrtc        = hrtc;
    ctx->callback    = NULL;
    ctx->arg         = NULL;
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_deinit(void *rtc)
{
    if (!rtc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_RTC_DeInit((RTC_HandleTypeDef *)rtc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->hrtc        = NULL;
    ctx->callback    = NULL;
    ctx->arg         = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_time(void *rtc, const xy_hal_rtc_time_t *time,
                                   xy_hal_rtc_format_t format)
{
    if (!rtc || !time) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    RTC_TimeTypeDef sTime = { 0 };

    if (format == XY_HAL_RTC_FORMAT_BIN) {
        sTime.Hours   = bin_to_bcd(time->hours);
        sTime.Minutes = bin_to_bcd(time->minutes);
        sTime.Seconds = bin_to_bcd(time->seconds);
    } else {
        sTime.Hours   = time->hours;
        sTime.Minutes = time->minutes;
        sTime.Seconds = time->seconds;
    }

    sTime.SubSeconds    = time->subseconds;
    sTime.TimeFormat    = RTC_HOURFORMAT12_AM;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime((RTC_HandleTypeDef *)rtc, &sTime,
                        RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *time,
                                   xy_hal_rtc_format_t format)
{
    if (!rtc || !time) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    RTC_TimeTypeDef sTime = { 0 };

    if (HAL_RTC_GetTime((RTC_HandleTypeDef *)rtc, &sTime,
                        RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    if (format == XY_HAL_RTC_FORMAT_BIN) {
        time->hours       = bcd_to_bin(sTime.Hours);
        time->minutes     = bcd_to_bin(sTime.Minutes);
        time->seconds     = bcd_to_bin(sTime.Seconds);
        time->subseconds  = sTime.SubSeconds;
    } else {
        time->hours       = sTime.Hours;
        time->minutes     = sTime.Minutes;
        time->seconds     = sTime.Seconds;
        time->subseconds  = sTime.SubSeconds;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_date(void *rtc, const xy_hal_rtc_date_t *date,
                                   xy_hal_rtc_format_t format)
{
    if (!rtc || !date) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    RTC_DateTypeDef sDate = { 0 };

    if (format != XY_HAL_RTC_FORMAT_BIN && format != XY_HAL_RTC_FORMAT_BCD) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    {
        /* This entry had no validation at all: any month, any day, and bin_to_bcd(date->year) with a
         * full year handed to a two-digit encoder, so writing 2026 wrote the low byte of 2026 encoded
         * as BCD. Decode, check, then spell the register. */
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

    if (HAL_RTC_SetDate((RTC_HandleTypeDef *)rtc, &sDate,
                        RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_date(void *rtc, xy_hal_rtc_date_t *date,
                                   xy_hal_rtc_format_t format)
{
    if (!rtc || !date) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    RTC_DateTypeDef sDate = { 0 };

    if (HAL_RTC_GetDate((RTC_HandleTypeDef *)rtc, &sDate,
                        RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    {
        /* Two digits in the register, a full year in the answer, whichever format was asked for. The
         * old code returned bcd_to_bin(Year) - 26 - in binary mode and the packed digits in BCD mode,
         * so no caller could tell a year from a digit pair. */
        int year = xy_rtc_year_decode((uint8_t)((sDate.Year <= 99U) ? sDate.Year : 0xFFU), 1);

        if (year < 0) {
            return XY_HAL_ERROR_FAIL;
        }
        if (format == XY_HAL_RTC_FORMAT_BIN) {
            date->weekday = bcd_to_bin(sDate.WeekDay);
            date->month   = bcd_to_bin(sDate.Month);
            date->date    = bcd_to_bin(sDate.Date);
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

/* The public API identifies the alarm by character, as F4 does: 'A' or 'B'. Mapping anything
 * else to B would answer the wrong alarm while reporting success, which is exactly what happened
 * when this backend ignored the parameter and always used RTC_ALARM_A. */
static int rtc_alarm_select(char alarm_id, uint32_t *alarm)
{
    if (alarm_id == 'A') {
        *alarm = RTC_ALARM_A;
        return 0;
    }
    if (alarm_id == 'B') {
        *alarm = RTC_ALARM_B;
        return 0;
    }
    return -1;
}

xy_hal_error_t xy_hal_rtc_set_alarm(void *rtc, const xy_hal_rtc_alarm_t *alarm,
                                    char alarm_id)
{
    uint32_t selected = 0U;
    if (!rtc || !alarm || rtc_alarm_select(alarm_id, &selected) != 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    RTC_AlarmTypeDef sAlarm = { 0 };

    sAlarm.AlarmTime.Hours   = bin_to_bcd(alarm->time.hours);
    sAlarm.AlarmTime.Minutes = bin_to_bcd(alarm->time.minutes);
    sAlarm.AlarmTime.Seconds = bin_to_bcd(alarm->time.seconds);
    sAlarm.AlarmTime.SubSeconds = alarm->time.subseconds;
    sAlarm.AlarmMask = alarm->alarm_mask;
    sAlarm.Alarm = selected;

    if (HAL_RTC_SetAlarm((RTC_HandleTypeDef *)rtc, &sAlarm,
                         RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_alarm(void *rtc, xy_hal_rtc_alarm_t *alarm,
                                    char alarm_id)
{
    uint32_t selected = 0U;
    if (!rtc || !alarm || rtc_alarm_select(alarm_id, &selected) != 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    RTC_AlarmTypeDef sAlarm = { 0 };

    if (HAL_RTC_GetAlarm((RTC_HandleTypeDef *)rtc, &sAlarm,
                         selected, RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    alarm->time.hours    = bcd_to_bin(sAlarm.AlarmTime.Hours);
    alarm->time.minutes  = bcd_to_bin(sAlarm.AlarmTime.Minutes);
    alarm->time.seconds  = bcd_to_bin(sAlarm.AlarmTime.Seconds);
    alarm->time.subseconds = sAlarm.AlarmTime.SubSeconds;
    alarm->alarm_mask    = sAlarm.AlarmMask;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_enable_alarm(void *rtc, char alarm_id)
{
    uint32_t selected = 0U;
    if (!rtc || rtc_alarm_select(alarm_id, &selected) != 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    __HAL_RTC_ALARM_ENABLE_IT((RTC_HandleTypeDef *)rtc,
                              selected == RTC_ALARM_B ? RTC_IT_ALRB : RTC_IT_ALRA);

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_disable_alarm(void *rtc, char alarm_id)
{
    uint32_t selected = 0U;
    if (!rtc || rtc_alarm_select(alarm_id, &selected) != 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    __HAL_RTC_ALARM_DISABLE_IT((RTC_HandleTypeDef *)rtc,
                               selected == RTC_ALARM_B ? RTC_IT_ALRB : RTC_IT_ALRA);

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_register_callback(void *rtc, xy_hal_rtc_event_t event,
                                            xy_hal_rtc_callback_t callback,
                                            void *arg)
{
    XY_UNUSED(event);
    if (!rtc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_ctx_t *ctx = find_rtc_ctx(rtc);
    if (ctx == NULL) {
        ctx = alloc_rtc_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->hrtc = (RTC_HandleTypeDef *)rtc;
    }

    ctx->callback = callback;
    ctx->arg      = arg;

    return XY_HAL_OK;
}

int64_t xy_hal_rtc_get_timestamp(void *rtc)
{
    if (!rtc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    xy_hal_rtc_time_t time = { 0 };
    xy_hal_rtc_date_t date = { 0 };

    if (xy_hal_rtc_get_time(rtc, &time, XY_HAL_RTC_FORMAT_BIN) != XY_HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    if (xy_hal_rtc_get_date(rtc, &date, XY_HAL_RTC_FORMAT_BIN) != XY_HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* The formula that stood here multiplied (year - 70) by 365 days and treated every month as 30,
     * which is not a Unix timestamp at all - it lands about 1900 years away from the right answer and
     * was returned as one. The shared calendar arithmetic is exact and is tested on the host. */
    return xy_civil_seconds_from_epoch((int)date.year, (unsigned int)date.month,
                                       (unsigned int)date.date, (unsigned int)time.hours,
                                       (unsigned int)time.minutes, (unsigned int)time.seconds);
}

xy_hal_error_t xy_hal_rtc_set_timestamp(void *rtc, int64_t timestamp)
{
    if (!rtc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    /* Convert timestamp to date/time - simplified implementation */
    XY_UNUSED(timestamp);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_rtc_backup_read(uint32_t index, uint32_t *value)
{
    if (value == NULL || index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    RTC_HandleTypeDef backup_rtc = { .Instance = RTC };
    *value = HAL_RTCEx_BKUPRead(&backup_rtc, index);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_backup_write(uint32_t index, uint32_t value)
{
    if (index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    RTC_HandleTypeDef backup_rtc = { .Instance = RTC };
    HAL_RTCEx_BKUPWrite(&backup_rtc, index, value);
    return XY_HAL_OK;
}

/* ==================== HAL Callbacks ==================== */

/**
 * @brief RTC Alarm callback
 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    rtc_ctx_t *ctx = find_rtc_ctx(hrtc);
    if (ctx && ctx->callback) {
        ctx->callback(XY_HAL_RTC_EVENT_ALARM_A, ctx->arg);
    }
}

#endif /* STM32U5 || STM32U5xx */
