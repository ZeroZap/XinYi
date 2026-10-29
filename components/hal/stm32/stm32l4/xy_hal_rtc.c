/**
 * @file xy_hal_rtc.c
 * @brief RTC HAL STM32L4 implementation
 *
 * This backend had two of the fifteen entries - the backup registers - and nothing else, on the MCU
 * the Pandora board actually runs. The rest is here now, held to the contract in xy_hal_rtc.h:
 * `alarm_id` selects the alarm, `format` decides what comes back rather than being decorative, and a
 * caller that asks for something this backend cannot do is told so.
 *
 * Two things are deliberately not done here:
 *
 * - The NVIC line is not armed. Enabling an alarm interrupt whose vector nobody provides is how a
 *   board ends up with an interrupt firing into an unpopulated handler; the board enables
 *   RTC_Alarm_IRQn and supplies the vector that calls HAL_RTC_AlarmIRQHandler, exactly as the DMA
 *   wrapper leaves its request line to the caller.
 * - `set_timestamp` is NOT_SUPPORTED. Turning a Unix timestamp back into a date means computing the
 *   weekday, and a wrong weekday stays in the date registers. Reading a timestamp is exact and is
 *   implemented; writing one is refused rather than approximated.
 */

#include "../../inc/xy_hal_rtc.h"

#if defined(STM32L4) || defined(STM32L4xx)

#include "stm32l4xx_hal.h"
/* Shared calendar arithmetic. Included by path so it resolves in every build shape - the HAL
 * component and the board targets do not agree on include directories. */
#include "../../../clib/xy_clib/inc/xy_bcd.h"
#include "../../../clib/xy_clib/inc/xy_time_civil.h"
#include <string.h>

/*
 * LSE is 32.768 kHz: (127 + 1) * (255 + 1) = 32768, so the calendar ticks at 1 Hz. The board HAL
 * configuration defines this pair; the fallback keeps the wrapper compiling in builds that do not go
 * through a board, rather than making the RTC body depend on which include path came first.
 */
#ifndef RTC_ASYNCH_PREDIV
#define RTC_ASYNCH_PREDIV 127U
#endif
#ifndef RTC_SYNCH_PREDIV
#define RTC_SYNCH_PREDIV 255U
#endif

#define MAX_RTC_INSTANCES 2U
#define RTC_EVENT_COUNT 4U

typedef struct {
    RTC_HandleTypeDef *hrtc;
    uint8_t initialized;
    xy_hal_rtc_callback_t callbacks[RTC_EVENT_COUNT];
    void *args[RTC_EVENT_COUNT];
} rtc_ctx_t;

static rtc_ctx_t g_rtc_ctx[MAX_RTC_INSTANCES];
static RTC_HandleTypeDef backup_rtc = {
    .Instance = RTC,
};

static rtc_ctx_t *find_rtc_ctx(const void *rtc)
{
    size_t index;

    if (rtc == NULL) {
        return NULL;
    }
    for (index = 0U; index < MAX_RTC_INSTANCES; index++) {
        if (g_rtc_ctx[index].hrtc == rtc) {
            return &g_rtc_ctx[index];
        }
    }
    return NULL;
}

static rtc_ctx_t *alloc_rtc_ctx(void)
{
    size_t index;

    for (index = 0U; index < MAX_RTC_INSTANCES; index++) {
        if (g_rtc_ctx[index].hrtc == NULL) {
            return &g_rtc_ctx[index];
        }
    }
    return NULL;
}

static uint8_t bin_to_bcd(uint8_t value)
{
    return (uint8_t)(((value / 10U) << 4) | (value % 10U));
}

static uint8_t bcd_to_bin(uint8_t value)
{
    return (uint8_t)(((value >> 4) * 10U) + (value & 0x0FU));
}

/* 'A'/'B' select the alarm; anything else is not an alarm and must not become one. */
static int rtc_alarm_select(char alarm_id, uint32_t *alarm)
{
    if (alarm_id == 'A' || alarm_id == 'a') {
        *alarm = RTC_ALARM_A;
        return 0;
    }
    if (alarm_id == 'B' || alarm_id == 'b') {
        *alarm = RTC_ALARM_B;
        return 0;
    }
    return -1;
}

/*
 * xy_hal_rtc_alarm_t.alarm_mask says which fields must match. The HAL says the opposite - a set
 * RTC_ALARMMASK_* bit means "do not compare this field" - so the two are inverted here rather than
 * passed through, which would match on exactly what the caller asked to ignore.
 */
static void rtc_alarm_mask(uint8_t alarm_mask, uint32_t *hal_mask, uint32_t *date_weekday_sel)
{
    uint32_t mask = 0U;

    if ((alarm_mask & XY_HAL_RTC_ALARM_MASK_HOURS) == 0U) {
        mask |= RTC_ALARMMASK_HOURS;
    }
    if ((alarm_mask & XY_HAL_RTC_ALARM_MASK_MINUTES) == 0U) {
        mask |= RTC_ALARMMASK_MINUTES;
    }
    if ((alarm_mask & XY_HAL_RTC_ALARM_MASK_SECONDS) == 0U) {
        mask |= RTC_ALARMMASK_SECONDS;
    }
    if ((alarm_mask & XY_HAL_RTC_ALARM_MASK_WEEKDAY) == 0U) {
        mask |= RTC_ALARMMASK_DATEWEEKDAY;
        *date_weekday_sel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    } else {
        *date_weekday_sel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY;
    }
    *hal_mask = mask;
}

xy_hal_error_t xy_hal_rtc_init(void *rtc)
{
    rtc_ctx_t *ctx;
    RTC_HandleTypeDef *hrtc;

    if (rtc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    hrtc = (RTC_HandleTypeDef *)rtc;
    ctx = find_rtc_ctx(rtc);
    if (ctx != NULL && ctx->initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (ctx == NULL) {
        ctx = alloc_rtc_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    hrtc->Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc->Init.AsynchPrediv = RTC_ASYNCH_PREDIV;
    hrtc->Init.SynchPrediv = RTC_SYNCH_PREDIV;
    hrtc->Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc->Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    hrtc->Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc->Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(hrtc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->hrtc = hrtc;
    ctx->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_deinit(void *rtc)
{
    rtc_ctx_t *ctx = find_rtc_ctx(rtc);

    if (rtc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (HAL_RTC_DeInit((RTC_HandleTypeDef *)rtc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    memset(ctx, 0, sizeof(*ctx));
    return XY_HAL_OK;
}

static xy_hal_error_t rtc_ready(const void *rtc, rtc_ctx_t **out)
{
    rtc_ctx_t *ctx = find_rtc_ctx(rtc);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    *out = ctx;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_time(void *rtc, const xy_hal_rtc_time_t *time,
                                   xy_hal_rtc_format_t format)
{
    rtc_ctx_t *ctx;
    xy_hal_error_t status = rtc_ready(rtc, &ctx);
    RTC_TimeTypeDef s_time = { 0 };

    if (status != XY_HAL_OK) {
        return status;
    }
    if (time == NULL || time->hours > 23U || time->minutes > 59U || time->seconds > 59U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (format != XY_HAL_RTC_FORMAT_BIN && format != XY_HAL_RTC_FORMAT_BCD) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    /* The HAL is always handed BCD so what reaches the registers is unambiguous; the caller's format
     * decides whether the values are converted on the way in. */
    if (format == XY_HAL_RTC_FORMAT_BIN) {
        s_time.Hours = bin_to_bcd(time->hours);
        s_time.Minutes = bin_to_bcd(time->minutes);
        s_time.Seconds = bin_to_bcd(time->seconds);
    } else {
        s_time.Hours = time->hours;
        s_time.Minutes = time->minutes;
        s_time.Seconds = time->seconds;
    }
    s_time.SubSeconds = time->subseconds;
    s_time.TimeFormat = RTC_HOURFORMAT_24;
    s_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    s_time.StoreOperation = RTC_STOREOPERATION_RESET;

    return HAL_RTC_SetTime((RTC_HandleTypeDef *)rtc, &s_time, RTC_FORMAT_BCD) == HAL_OK
               ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *time,
                                   xy_hal_rtc_format_t format)
{
    rtc_ctx_t *ctx;
    xy_hal_error_t status = rtc_ready(rtc, &ctx);
    RTC_TimeTypeDef s_time = { 0 };

    if (status != XY_HAL_OK) {
        return status;
    }
    if (time == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (format != XY_HAL_RTC_FORMAT_BIN && format != XY_HAL_RTC_FORMAT_BCD) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (HAL_RTC_GetTime((RTC_HandleTypeDef *)rtc, &s_time, RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    if (format == XY_HAL_RTC_FORMAT_BIN) {
        time->hours = bcd_to_bin(s_time.Hours);
        time->minutes = bcd_to_bin(s_time.Minutes);
        time->seconds = bcd_to_bin(s_time.Seconds);
    } else {
        time->hours = s_time.Hours;
        time->minutes = s_time.Minutes;
        time->seconds = s_time.Seconds;
    }
    time->subseconds = s_time.SubSeconds;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_date(void *rtc, const xy_hal_rtc_date_t *date,
                                   xy_hal_rtc_format_t format)
{
    rtc_ctx_t *ctx;
    xy_hal_error_t status = rtc_ready(rtc, &ctx);
    RTC_DateTypeDef s_date = { 0 };

    if (status != XY_HAL_OK) {
        return status;
    }
    if (date == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (format != XY_HAL_RTC_FORMAT_BIN && format != XY_HAL_RTC_FORMAT_BCD) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    {
        /* Decode before validating, in that order for a reason: the range checks are about months
         * and years, not about bytes that happen to hold their digits. The year is a full year
         * whichever format arrived, and the register gets two digits. */
        uint8_t month = date->month;
        uint8_t day = date->date;
        uint16_t year = date->year;
        int year_register;

        if (format == XY_HAL_RTC_FORMAT_BCD) {
            if (!xy_bcd_decode8(date->month, &month) || !xy_bcd_decode8(date->date, &day) ||
                !xy_bcd_decode16(date->year, &year)) {
                return XY_HAL_ERROR_INVALID_PARAM;
            }
        }
        if (month < 1U || month > 12U || day < 1U || day > 31U || date->weekday > 7U) {
            return XY_HAL_ERROR_INVALID_PARAM;
        }
        year_register = xy_rtc_year_encode(year, 1);
        if (year_register < 0) {
            /* Two digits of room: a year outside the base century would be truncated into a
             * different date rather than refused. */
            return XY_HAL_ERROR_INVALID_PARAM;
        }
        s_date.WeekDay = date->weekday;
        s_date.Month = xy_bcd_encode8(month);
        s_date.Date = xy_bcd_encode8(day);
        s_date.Year = (uint8_t)year_register;
    }

    return HAL_RTC_SetDate((RTC_HandleTypeDef *)rtc, &s_date, RTC_FORMAT_BCD) == HAL_OK
               ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_rtc_get_date(void *rtc, xy_hal_rtc_date_t *date,
                                   xy_hal_rtc_format_t format)
{
    rtc_ctx_t *ctx;
    xy_hal_error_t status = rtc_ready(rtc, &ctx);
    RTC_DateTypeDef s_date = { 0 };

    if (status != XY_HAL_OK) {
        return status;
    }
    if (date == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (format != XY_HAL_RTC_FORMAT_BIN && format != XY_HAL_RTC_FORMAT_BCD) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (HAL_RTC_GetDate((RTC_HandleTypeDef *)rtc, &s_date, RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    {
        /* The register holds two digits and the century is this board's convention, so the caller is
         * always given a full year - the format decides only how the digits are spelled. Reading the
         * vendor in BCD and converting once means the two branches cannot drift apart, which is what
         * they had done: binary reported 2026 and BCD reported 0x26 for the same register. */
        const uint8_t two_digit =
            (uint8_t)((s_date.Year <= 99U) ? s_date.Year : 0xFFU);
        int year = xy_rtc_year_decode(two_digit, 1);

        if (year < 0) {
            /* A register holding something that is not a date is a bus or clock problem, and a
             * plausible year here would hide it. */
            return XY_HAL_ERROR_FAIL;
        }
        if (format == XY_HAL_RTC_FORMAT_BIN) {
            date->weekday = s_date.WeekDay; /* 1-7 binary, not BCD, in the vendor struct */
            date->month = bcd_to_bin(s_date.Month);
            date->date = bcd_to_bin(s_date.Date);
            date->year = (uint16_t)year;
        } else {
            date->weekday = s_date.WeekDay;
            date->month = s_date.Month;
            date->date = s_date.Date;
            date->year = xy_bcd_encode16((uint16_t)year);
        }
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_alarm(void *rtc, const xy_hal_rtc_alarm_t *alarm, char alarm_id)
{
    rtc_ctx_t *ctx;
    xy_hal_error_t status = rtc_ready(rtc, &ctx);
    RTC_AlarmTypeDef s_alarm = { 0 };
    uint32_t selected = 0U;
    uint32_t hal_mask = 0U;
    uint32_t date_weekday_sel = RTC_ALARMDATEWEEKDAYSEL_DATE;

    if (status != XY_HAL_OK) {
        return status;
    }
    if (alarm == NULL || rtc_alarm_select(alarm_id, &selected) != 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (alarm->time.hours > 23U || alarm->time.minutes > 59U || alarm->time.seconds > 59U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    rtc_alarm_mask(alarm->alarm_mask, &hal_mask, &date_weekday_sel);

    s_alarm.AlarmTime.Hours = bin_to_bcd(alarm->time.hours);
    s_alarm.AlarmTime.Minutes = bin_to_bcd(alarm->time.minutes);
    s_alarm.AlarmTime.Seconds = bin_to_bcd(alarm->time.seconds);
    s_alarm.AlarmTime.SubSeconds = alarm->time.subseconds;
    s_alarm.AlarmTime.TimeFormat = RTC_HOURFORMAT_24;
    s_alarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    s_alarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
    s_alarm.AlarmMask = hal_mask;
    s_alarm.AlarmDateWeekDaySel = date_weekday_sel;
    s_alarm.AlarmDateWeekDay = (alarm->weekday != 0U) ? alarm->weekday
                                                     : ((alarm->date != 0U) ? alarm->date : 1U);
    s_alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;

    return HAL_RTC_SetAlarm((RTC_HandleTypeDef *)rtc, &s_alarm, selected) == HAL_OK
               ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_rtc_get_alarm(void *rtc, xy_hal_rtc_alarm_t *alarm, char alarm_id)
{
    rtc_ctx_t *ctx;
    xy_hal_error_t status = rtc_ready(rtc, &ctx);
    RTC_AlarmTypeDef s_alarm = { 0 };
    uint32_t selected = 0U;

    if (status != XY_HAL_OK) {
        return status;
    }
    if (alarm == NULL || rtc_alarm_select(alarm_id, &selected) != 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (HAL_RTC_GetAlarm((RTC_HandleTypeDef *)rtc, &s_alarm, selected, RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    alarm->time.hours = bcd_to_bin(s_alarm.AlarmTime.Hours);
    alarm->time.minutes = bcd_to_bin(s_alarm.AlarmTime.Minutes);
    alarm->time.seconds = bcd_to_bin(s_alarm.AlarmTime.Seconds);
    alarm->time.subseconds = s_alarm.AlarmTime.SubSeconds;
    alarm->weekday = (s_alarm.AlarmDateWeekDaySel == RTC_ALARMDATEWEEKDAYSEL_WEEKDAY)
                         ? (uint8_t)s_alarm.AlarmDateWeekDay : 0U;
    alarm->date = (s_alarm.AlarmDateWeekDaySel == RTC_ALARMDATEWEEKDAYSEL_DATE)
                      ? (uint8_t)s_alarm.AlarmDateWeekDay : 0U;
    /* Reported back in the caller's sense: the fields the HAL does not mask are the ones it compares,
     * which is what "must match" means on this side. */
    alarm->alarm_mask = 0U;
    if ((s_alarm.AlarmMask & RTC_ALARMMASK_HOURS) == 0U) {
        alarm->alarm_mask |= XY_HAL_RTC_ALARM_MASK_HOURS;
    }
    if ((s_alarm.AlarmMask & RTC_ALARMMASK_MINUTES) == 0U) {
        alarm->alarm_mask |= XY_HAL_RTC_ALARM_MASK_MINUTES;
    }
    if ((s_alarm.AlarmMask & RTC_ALARMMASK_SECONDS) == 0U) {
        alarm->alarm_mask |= XY_HAL_RTC_ALARM_MASK_SECONDS;
    }
    if ((s_alarm.AlarmMask & RTC_ALARMMASK_DATEWEEKDAY) == 0U) {
        alarm->alarm_mask |= XY_HAL_RTC_ALARM_MASK_WEEKDAY;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_enable_alarm(void *rtc, char alarm_id)
{
    rtc_ctx_t *ctx;
    xy_hal_error_t status = rtc_ready(rtc, &ctx);
    RTC_AlarmTypeDef s_alarm = { 0 };
    uint32_t selected = 0U;

    if (status != XY_HAL_OK) {
        return status;
    }
    if (rtc_alarm_select(alarm_id, &selected) != 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    /* This series has no ActivateAlarm: an alarm is armed by programming it through the interrupt
     * variant, so the alarm is read back and re-set with interrupts enabled. The NVIC line is the
     * board's - a board that forgets RTC_Alarm_IRQn gets no callback, and xy_hal_rtc.h says so. */
    if (HAL_RTC_GetAlarm((RTC_HandleTypeDef *)rtc, &s_alarm, selected, RTC_FORMAT_BCD) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    return HAL_RTC_SetAlarm_IT((RTC_HandleTypeDef *)rtc, &s_alarm, RTC_FORMAT_BCD) == HAL_OK
               ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_rtc_disable_alarm(void *rtc, char alarm_id)
{
    rtc_ctx_t *ctx;
    xy_hal_error_t status = rtc_ready(rtc, &ctx);
    uint32_t selected = 0U;

    if (status != XY_HAL_OK) {
        return status;
    }
    if (rtc_alarm_select(alarm_id, &selected) != 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return HAL_RTC_DeactivateAlarm((RTC_HandleTypeDef *)rtc, selected) == HAL_OK
               ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_rtc_register_callback(void *rtc, xy_hal_rtc_event_t event,
                                            xy_hal_rtc_callback_t callback, void *arg)
{
    rtc_ctx_t *ctx;
    size_t index = (size_t)event;

    if (rtc == NULL || index >= RTC_EVENT_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = find_rtc_ctx(rtc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    /* Each event keeps its own callback: registering alarm B must not replace alarm A's. */
    ctx->callbacks[index] = callback;
    ctx->args[index] = arg;
    return XY_HAL_OK;
}

int64_t xy_hal_rtc_get_timestamp(void *rtc)
{
    xy_hal_rtc_time_t time = { 0 };
    xy_hal_rtc_date_t date = { 0 };

    if (rtc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (xy_hal_rtc_get_time(rtc, &time, XY_HAL_RTC_FORMAT_BIN) != XY_HAL_OK) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (xy_hal_rtc_get_date(rtc, &date, XY_HAL_RTC_FORMAT_BIN) != XY_HAL_OK) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    return xy_civil_seconds_from_epoch((int)date.year, (unsigned int)date.month,
                                       (unsigned int)date.date, (unsigned int)time.hours,
                                       (unsigned int)time.minutes, (unsigned int)time.seconds);
}

xy_hal_error_t xy_hal_rtc_set_timestamp(void *rtc, int64_t timestamp)
{
    rtc_ctx_t *ctx;
    xy_hal_error_t status = rtc_ready(rtc, &ctx);

    (void)timestamp;
    if (status != XY_HAL_OK) {
        return status;
    }
    /* Writing back means computing the weekday, and a wrong weekday stays in the date registers.
     * Reading is exact and implemented; this is refused rather than approximated. */
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_rtc_backup_read(uint32_t index, uint32_t *value)
{
    if (value == NULL || index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
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
    HAL_RTCEx_BKUPWrite(&backup_rtc, index, value);
    return XY_HAL_OK;
}

/* ==================== HAL callbacks ==================== */

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    rtc_ctx_t *ctx = find_rtc_ctx(hrtc);

    if (ctx != NULL && ctx->callbacks[XY_HAL_RTC_EVENT_ALARM_A] != NULL) {
        ctx->callbacks[XY_HAL_RTC_EVENT_ALARM_A](XY_HAL_RTC_EVENT_ALARM_A,
                                                 ctx->args[XY_HAL_RTC_EVENT_ALARM_A]);
    }
}

void HAL_RTC_AlarmBEventCallback(RTC_HandleTypeDef *hrtc)
{
    rtc_ctx_t *ctx = find_rtc_ctx(hrtc);

    if (ctx != NULL && ctx->callbacks[XY_HAL_RTC_EVENT_ALARM_B] != NULL) {
        ctx->callbacks[XY_HAL_RTC_EVENT_ALARM_B](XY_HAL_RTC_EVENT_ALARM_B,
                                                 ctx->args[XY_HAL_RTC_EVENT_ALARM_B]);
    }
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    rtc_ctx_t *ctx = find_rtc_ctx(hrtc);

    if (ctx != NULL && ctx->callbacks[XY_HAL_RTC_EVENT_WAKEUP] != NULL) {
        ctx->callbacks[XY_HAL_RTC_EVENT_WAKEUP](XY_HAL_RTC_EVENT_WAKEUP,
                                                ctx->args[XY_HAL_RTC_EVENT_WAKEUP]);
    }
}

#endif /* STM32L4 || STM32L4xx */
