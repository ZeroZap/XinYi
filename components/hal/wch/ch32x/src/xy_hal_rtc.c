/**
 * @file xy_hal_rtc.c
 * @brief WCH CH32V30x RTC HAL implementation
 */

#include "xy_hal_rtc.h"

#ifdef MCU_CH32

#include "ch32v30x.h"
#include "ch32v30x_bkp.h"
#include "ch32v30x_pwr.h"
#include "ch32v30x_rtc.h"

#define WCH_RTC_LSI_HZ 40000U

static xy_hal_rtc_callback_t s_alarm_callback;
static void* s_alarm_arg;
static uint8_t s_initialized;

static int valid_rtc(void* rtc) {
    return rtc == RTC;
}
static uint8_t bcd_to_bin(uint8_t v) {
    return (uint8_t)(((v >> 4U) * 10U) + (v & 0x0FU));
}
static uint8_t bin_to_bcd(uint8_t v) {
    return (uint8_t)(((v / 10U) << 4U) | (v % 10U));
}
static int leap(uint32_t year) {
    return (year % 4U == 0U && year % 100U != 0U) || year % 400U == 0U;
}
static uint8_t month_days(uint32_t year, uint8_t month) {
    static const uint8_t days[] = {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
    return (uint8_t)(days[month - 1U] + ((month == 2U && leap(year)) ? 1U : 0U));
}

static int valid_date(const xy_hal_rtc_date_t* date) {
    uint32_t year;
    if (date == NULL || date->month < 1U || date->month > 12U || date->weekday < 1U ||
        date->weekday > 7U)
        return 0;
    year = date->year < 100U ? date->year + 2000U : date->year;
    return year >= 1970U && year <= 2106U && date->date >= 1U &&
           date->date <= month_days(year, date->month);
}

static int valid_time(const xy_hal_rtc_time_t* time) {
    return time != NULL && time->hours < 24U && time->minutes < 60U && time->seconds < 60U;
}

static uint32_t to_timestamp(const xy_hal_rtc_date_t* date, const xy_hal_rtc_time_t* time) {
    uint32_t year = date->year < 100U ? date->year + 2000U : date->year;
    uint32_t days = 0U;
    for (uint32_t y = 1970U; y < year; ++y)
        days += leap(y) ? 366U : 365U;
    for (uint8_t m = 1U; m < date->month; ++m)
        days += month_days(year, m);
    days += date->date - 1U;
    return days * 86400U + (uint32_t)time->hours * 3600U + (uint32_t)time->minutes * 60U +
           time->seconds;
}

static void from_timestamp(uint32_t stamp, xy_hal_rtc_date_t* date, xy_hal_rtc_time_t* time) {
    uint32_t days = stamp / 86400U;
    uint32_t seconds = stamp % 86400U;
    uint32_t year = 1970U;
    uint8_t month = 1U;
    if (time != NULL) {
        time->hours = (uint8_t)(seconds / 3600U);
        time->minutes = (uint8_t)((seconds % 3600U) / 60U);
        time->seconds = (uint8_t)(seconds % 60U);
        time->subseconds = 0U;
    }
    if (date == NULL)
        return;
    while (days >= (uint32_t)(leap(year) ? 366U : 365U))
        days -= leap(year++) ? 366U : 365U;
    while (days >= month_days(year, month))
        days -= month_days(year, month++);
    date->year = (uint16_t)year;
    date->month = month;
    date->date = (uint8_t)(days + 1U);
    date->weekday = (uint8_t)(((stamp / 86400U + 3U) % 7U) + 1U);
}

xy_hal_error_t xy_hal_rtc_init(void* rtc) {
    uint32_t wait = 1000000U;
    if (!valid_rtc(rtc))
        return XY_HAL_ERROR_INVALID_PARAM;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);
    RCC_LSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET && wait-- != 0U) {
    }
    if (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
        return XY_HAL_ERROR_TIMEOUT;
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
    RCC_RTCCLKCmd(ENABLE);
    RTC_WaitForSynchro();
    RTC_WaitForLastTask();
    RTC_EnterConfigMode();
    RTC_SetPrescaler(WCH_RTC_LSI_HZ - 1U);
    RTC_ExitConfigMode();
    RTC_WaitForLastTask();
    s_initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_deinit(void* rtc) {
    if (!valid_rtc(rtc))
        return XY_HAL_ERROR_INVALID_PARAM;
    RTC_ITConfig(RTC_IT_ALR | RTC_IT_SEC | RTC_IT_OW, DISABLE);
    s_initialized = 0U;
    s_alarm_callback = NULL;
    s_alarm_arg = NULL;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_time(void* rtc, const xy_hal_rtc_time_t* time,
                                   xy_hal_rtc_format_t format) {
    xy_hal_rtc_time_t value;
    xy_hal_rtc_date_t date;
    if (!valid_rtc(rtc) || !s_initialized || time == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    value = *time;
    if (format == XY_HAL_RTC_FORMAT_BCD) {
        value.hours = bcd_to_bin(value.hours);
        value.minutes = bcd_to_bin(value.minutes);
        value.seconds = bcd_to_bin(value.seconds);
    } else if (format != XY_HAL_RTC_FORMAT_BIN)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!valid_time(&value))
        return XY_HAL_ERROR_INVALID_PARAM;
    from_timestamp(RTC_GetCounter(), &date, NULL);
    RTC_SetCounter(to_timestamp(&date, &value));
    RTC_WaitForLastTask();
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_time(void* rtc, xy_hal_rtc_time_t* time, xy_hal_rtc_format_t format) {
    if (!valid_rtc(rtc) || !s_initialized || time == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    from_timestamp(RTC_GetCounter(), NULL, time);
    if (format == XY_HAL_RTC_FORMAT_BCD) {
        time->hours = bin_to_bcd(time->hours);
        time->minutes = bin_to_bcd(time->minutes);
        time->seconds = bin_to_bcd(time->seconds);
    } else if (format != XY_HAL_RTC_FORMAT_BIN)
        return XY_HAL_ERROR_INVALID_PARAM;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_date(void* rtc, const xy_hal_rtc_date_t* date,
                                   xy_hal_rtc_format_t format) {
    xy_hal_rtc_date_t value;
    xy_hal_rtc_time_t time;
    if (!valid_rtc(rtc) || !s_initialized || date == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    value = *date;
    if (format == XY_HAL_RTC_FORMAT_BCD) {
        value.month = bcd_to_bin(value.month);
        value.date = bcd_to_bin(value.date);
        value.year = bcd_to_bin((uint8_t)value.year);
    } else if (format != XY_HAL_RTC_FORMAT_BIN)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!valid_date(&value))
        return XY_HAL_ERROR_INVALID_PARAM;
    from_timestamp(RTC_GetCounter(), NULL, &time);
    RTC_SetCounter(to_timestamp(&value, &time));
    RTC_WaitForLastTask();
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_date(void* rtc, xy_hal_rtc_date_t* date, xy_hal_rtc_format_t format) {
    if (!valid_rtc(rtc) || !s_initialized || date == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    from_timestamp(RTC_GetCounter(), date, NULL);
    if (format == XY_HAL_RTC_FORMAT_BCD) {
        date->month = bin_to_bcd(date->month);
        date->date = bin_to_bcd(date->date);
        date->year = bin_to_bcd((uint8_t)(date->year % 100U));
    } else if (format != XY_HAL_RTC_FORMAT_BIN)
        return XY_HAL_ERROR_INVALID_PARAM;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_alarm(void* rtc, const xy_hal_rtc_alarm_t* alarm, char alarm_id) {
    xy_hal_rtc_date_t date;
    if (!valid_rtc(rtc) || !s_initialized || alarm == NULL || alarm_id != 'A' ||
        !valid_time(&alarm->time))
        return alarm_id == 'B' ? XY_HAL_ERROR_NOT_SUPPORTED : XY_HAL_ERROR_INVALID_PARAM;
    from_timestamp(RTC_GetCounter(), &date, NULL);
    if (alarm->date != 0U)
        date.date = alarm->date;
    RTC_SetAlarm(to_timestamp(&date, &alarm->time));
    RTC_WaitForLastTask();
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_alarm(void* rtc, xy_hal_rtc_alarm_t* alarm, char alarm_id) {
    (void)rtc;
    (void)alarm;
    (void)alarm_id;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_rtc_enable_alarm(void* rtc, char alarm_id) {
    if (!valid_rtc(rtc) || !s_initialized || alarm_id != 'A')
        return alarm_id == 'B' ? XY_HAL_ERROR_NOT_SUPPORTED : XY_HAL_ERROR_INVALID_PARAM;
    RTC_ITConfig(RTC_IT_ALR, ENABLE);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_rtc_disable_alarm(void* rtc, char alarm_id) {
    if (!valid_rtc(rtc) || !s_initialized || alarm_id != 'A')
        return alarm_id == 'B' ? XY_HAL_ERROR_NOT_SUPPORTED : XY_HAL_ERROR_INVALID_PARAM;
    RTC_ITConfig(RTC_IT_ALR, DISABLE);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_rtc_register_callback(void* rtc, xy_hal_rtc_event_t event,
                                            xy_hal_rtc_callback_t callback, void* arg) {
    if (!valid_rtc(rtc) || event != XY_HAL_RTC_EVENT_ALARM_A || callback == NULL)
        return event == XY_HAL_RTC_EVENT_ALARM_B || event == XY_HAL_RTC_EVENT_WAKEUP ||
                       event == XY_HAL_RTC_EVENT_TIMESTAMP
                   ? XY_HAL_ERROR_NOT_SUPPORTED
                   : XY_HAL_ERROR_INVALID_PARAM;
    s_alarm_callback = callback;
    s_alarm_arg = arg;
    return XY_HAL_OK;
}
int64_t xy_hal_rtc_get_timestamp(void* rtc) {
    return valid_rtc(rtc) && s_initialized ? (int64_t)RTC_GetCounter()
                                           : (int64_t)XY_HAL_ERROR_INVALID_PARAM;
}
xy_hal_error_t xy_hal_rtc_set_timestamp(void* rtc, int64_t timestamp) {
    if (!valid_rtc(rtc) || !s_initialized || timestamp < 0 || timestamp > UINT32_MAX)
        return XY_HAL_ERROR_INVALID_PARAM;
    RTC_SetCounter((uint32_t)timestamp);
    RTC_WaitForLastTask();
    return XY_HAL_OK;
}

static uint16_t backup_reg(uint32_t half_index) {
    return half_index < 10U ? (uint16_t)(BKP_DR1 + half_index * 4U)
                            : (uint16_t)(BKP_DR11 + (half_index - 10U) * 4U);
}
xy_hal_error_t xy_hal_rtc_backup_read(uint32_t index, uint32_t* value) {
    if (index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT || value == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    *value = (uint32_t)BKP_ReadBackupRegister(backup_reg(index * 2U)) |
             ((uint32_t)BKP_ReadBackupRegister(backup_reg(index * 2U + 1U)) << 16U);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_rtc_backup_write(uint32_t index, uint32_t value) {
    if (index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);
    BKP_WriteBackupRegister(backup_reg(index * 2U), (uint16_t)value);
    BKP_WriteBackupRegister(backup_reg(index * 2U + 1U), (uint16_t)(value >> 16U));
    return XY_HAL_OK;
}

void RTC_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void RTC_IRQHandler(void) {
    if (RTC_GetITStatus(RTC_IT_ALR) != RESET) {
        RTC_ClearITPendingBit(RTC_IT_ALR);
        if (s_alarm_callback != NULL)
            s_alarm_callback(XY_HAL_RTC_EVENT_ALARM_A, s_alarm_arg);
    }
}

#endif /* MCU_CH32 */
