/**
 * @file xy_hal_rtc_pc.c
 * @brief PC/Linux simulation layer for RTC HAL
 *
 * The host has no crystal, so what is modelled is the part that is a contract: what was written can
 * be read back, the requested format is either honoured or refused, and an alarm id selects which
 * alarm - answering a request for alarm B with alarm A's slot is the defect this backend exists to
 * avoid repeating. Time does not advance on its own; xy_hal_rtc_pc_tick() is how a test asks for a
 * second to pass, which keeps every timestamp assertion deterministic.
 *
 * Backup registers keep their existing behaviour: twenty words, index-checked.
 */

#include "xy_hal_rtc.h"
#include <stdint.h>
#include <string.h>

#define PC_RTC_SLOTS 2U
#define PC_RTC_EVENT_COUNT 4U
#define PC_RTC_ALARM_COUNT 2U

typedef struct {
    void *handle;
    uint8_t used;
    uint8_t initialized;
    xy_hal_rtc_time_t time;
    xy_hal_rtc_date_t date;
    xy_hal_rtc_alarm_t alarms[PC_RTC_ALARM_COUNT];
    uint8_t alarm_enabled[PC_RTC_ALARM_COUNT];
    uint8_t alarm_valid[PC_RTC_ALARM_COUNT];
    xy_hal_rtc_callback_t callbacks[PC_RTC_EVENT_COUNT];
    void *args[PC_RTC_EVENT_COUNT];
} pc_rtc_ctx_t;

static pc_rtc_ctx_t g_rtc_ctx[PC_RTC_SLOTS];
static uint32_t backup_registers[XY_HAL_RTC_BACKUP_REGISTER_COUNT];

static pc_rtc_ctx_t *pc_rtc_find(const void *handle)
{
    unsigned int index;

    if (handle == NULL) {
        return NULL;
    }
    for (index = 0U; index < PC_RTC_SLOTS; index++) {
        if (g_rtc_ctx[index].used != 0U && g_rtc_ctx[index].handle == handle) {
            return &g_rtc_ctx[index];
        }
    }
    return NULL;
}

static pc_rtc_ctx_t *pc_rtc_slot(void)
{
    unsigned int index;

    for (index = 0U; index < PC_RTC_SLOTS; index++) {
        if (g_rtc_ctx[index].used == 0U) {
            g_rtc_ctx[index].used = 1U;
            return &g_rtc_ctx[index];
        }
    }
    return NULL;
}

/* 'A'/'a' -> 0, 'B'/'b' -> 1. Anything else is not an alarm, and must not quietly become one. */
static int pc_rtc_alarm_index(char alarm_id, size_t *index)
{
    if (alarm_id == 'A' || alarm_id == 'a') {
        *index = 0U;
        return 1;
    }
    if (alarm_id == 'B' || alarm_id == 'b') {
        *index = 1U;
        return 1;
    }
    return 0;
}

static int pc_rtc_time_valid(const xy_hal_rtc_time_t *time)
{
    if (time->hours > 23U) return 0;
    if (time->minutes > 59U) return 0;
    if (time->seconds > 59U) return 0;
    return 1;
}

static int pc_rtc_date_valid(const xy_hal_rtc_date_t *date)
{
    if (date->month < 1U || date->month > 12U) return 0;
    if (date->date < 1U || date->date > 31U) return 0;
    if (date->weekday > 7U) return 0;
    return 1;
}

xy_hal_error_t xy_hal_rtc_init(void *rtc)
{
    pc_rtc_ctx_t *ctx;

    if (rtc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_rtc_find(rtc);
    if (ctx != NULL && ctx->initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (ctx == NULL) {
        ctx = pc_rtc_slot();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->handle = rtc;
    }
    memset(&ctx->time, 0, sizeof(ctx->time));
    memset(&ctx->date, 0, sizeof(ctx->date));
    ctx->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_deinit(void *rtc)
{
    pc_rtc_ctx_t *ctx;

    if (rtc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_rtc_find(rtc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    memset(ctx, 0, sizeof(*ctx));
    return XY_HAL_OK;
}

static xy_hal_error_t pc_rtc_ready(const void *rtc, pc_rtc_ctx_t **out)
{
    pc_rtc_ctx_t *ctx = pc_rtc_find(rtc);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    *out = ctx;
    return XY_HAL_OK;
}

/* The format argument is not advice. A backend that cannot produce BCD must refuse rather than
 * return binary digits that the caller will interpret as BCD. */
static xy_hal_error_t pc_rtc_format_ok(xy_hal_rtc_format_t format)
{
    return (format == XY_HAL_RTC_FORMAT_BIN) ? XY_HAL_OK : XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_rtc_set_time(void *rtc, const xy_hal_rtc_time_t *time,
                                   xy_hal_rtc_format_t format)
{
    pc_rtc_ctx_t *ctx;
    xy_hal_error_t status = pc_rtc_ready(rtc, &ctx);

    if (status != XY_HAL_OK) {
        return status;
    }
    if (time == NULL || !pc_rtc_time_valid(time)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    status = pc_rtc_format_ok(format);
    if (status != XY_HAL_OK) {
        return status;
    }
    ctx->time = *time;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *time,
                                   xy_hal_rtc_format_t format)
{
    pc_rtc_ctx_t *ctx;
    xy_hal_error_t status = pc_rtc_ready(rtc, &ctx);

    if (status != XY_HAL_OK) {
        return status;
    }
    if (time == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    status = pc_rtc_format_ok(format);
    if (status != XY_HAL_OK) {
        return status;
    }
    *time = ctx->time;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_date(void *rtc, const xy_hal_rtc_date_t *date,
                                   xy_hal_rtc_format_t format)
{
    pc_rtc_ctx_t *ctx;
    xy_hal_error_t status = pc_rtc_ready(rtc, &ctx);

    if (status != XY_HAL_OK) {
        return status;
    }
    if (date == NULL || !pc_rtc_date_valid(date)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    status = pc_rtc_format_ok(format);
    if (status != XY_HAL_OK) {
        return status;
    }
    ctx->date = *date;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_date(void *rtc, xy_hal_rtc_date_t *date,
                                   xy_hal_rtc_format_t format)
{
    pc_rtc_ctx_t *ctx;
    xy_hal_error_t status = pc_rtc_ready(rtc, &ctx);

    if (status != XY_HAL_OK) {
        return status;
    }
    if (date == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    status = pc_rtc_format_ok(format);
    if (status != XY_HAL_OK) {
        return status;
    }
    *date = ctx->date;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_set_alarm(void *rtc, const xy_hal_rtc_alarm_t *alarm, char alarm_id)
{
    pc_rtc_ctx_t *ctx;
    xy_hal_error_t status = pc_rtc_ready(rtc, &ctx);
    size_t index;

    if (status != XY_HAL_OK) {
        return status;
    }
    if (alarm == NULL || !pc_rtc_time_valid(&alarm->time) || !pc_rtc_alarm_index(alarm_id, &index)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx->alarms[index] = *alarm;
    ctx->alarm_valid[index] = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_get_alarm(void *rtc, xy_hal_rtc_alarm_t *alarm, char alarm_id)
{
    pc_rtc_ctx_t *ctx;
    xy_hal_error_t status = pc_rtc_ready(rtc, &ctx);
    size_t index;

    if (status != XY_HAL_OK) {
        return status;
    }
    if (alarm == NULL || !pc_rtc_alarm_index(alarm_id, &index)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx->alarm_valid[index] == 0U) {
        return XY_HAL_ERROR_NOT_FOUND;
    }
    *alarm = ctx->alarms[index];
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_enable_alarm(void *rtc, char alarm_id)
{
    pc_rtc_ctx_t *ctx;
    xy_hal_error_t status = pc_rtc_ready(rtc, &ctx);
    size_t index;

    if (status != XY_HAL_OK) {
        return status;
    }
    if (!pc_rtc_alarm_index(alarm_id, &index)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx->alarm_valid[index] == 0U) {
        /* Nothing has been written to this alarm, so enabling it would arm a comparison against
         * whatever the register happened to hold. */
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->alarm_enabled[index] = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_disable_alarm(void *rtc, char alarm_id)
{
    pc_rtc_ctx_t *ctx;
    xy_hal_error_t status = pc_rtc_ready(rtc, &ctx);
    size_t index;

    if (status != XY_HAL_OK) {
        return status;
    }
    if (!pc_rtc_alarm_index(alarm_id, &index)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx->alarm_enabled[index] = 0U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_register_callback(void *rtc, xy_hal_rtc_event_t event,
                                            xy_hal_rtc_callback_t callback, void *arg)
{
    pc_rtc_ctx_t *ctx;
    size_t index = (size_t)event;

    if (rtc == NULL || index >= PC_RTC_EVENT_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_rtc_find(rtc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->callbacks[index] = callback;
    ctx->args[index] = arg;
    return XY_HAL_OK;
}

/* The counter is binary seconds since midnight, and the day is not tracked: two fields a caller can
 * set and read back, which is what a host can honestly offer. */
static int64_t pc_rtc_seconds(const pc_rtc_ctx_t *ctx)
{
    return (int64_t)ctx->time.hours * 3600 + (int64_t)ctx->time.minutes * 60 +
           (int64_t)ctx->time.seconds;
}

int64_t xy_hal_rtc_get_timestamp(void *rtc)
{
    pc_rtc_ctx_t *ctx = pc_rtc_find(rtc);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return pc_rtc_seconds(ctx);
}

xy_hal_error_t xy_hal_rtc_set_timestamp(void *rtc, int64_t timestamp)
{
    pc_rtc_ctx_t *ctx;
    xy_hal_error_t status = pc_rtc_ready(rtc, &ctx);

    if (status != XY_HAL_OK) {
        return status;
    }
    if (timestamp < 0 || timestamp > 86399) {
        /* The date is not part of this counter, so a value beyond one day has no meaning here and
         * is refused rather than wrapped into tomorrow's early hours. */
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx->time.hours = (uint8_t)(timestamp / 3600);
    ctx->time.minutes = (uint8_t)((timestamp % 3600) / 60);
    ctx->time.seconds = (uint8_t)(timestamp % 60);
    return XY_HAL_OK;
}

/**
 * @brief Advance the simulated clock by one second.
 *
 * Time does not move on its own here: a host backend that pulled the wall clock would make every
 * second-boundary assertion flaky. A test asks for the tick.
 */
void xy_hal_rtc_pc_tick(void *rtc)
{
    pc_rtc_ctx_t *ctx = pc_rtc_find(rtc);
    int64_t seconds;

    if (ctx == NULL || ctx->initialized == 0U) {
        return;
    }
    seconds = (pc_rtc_seconds(ctx) + 1) % 86400;
    ctx->time.hours = (uint8_t)(seconds / 3600);
    ctx->time.minutes = (uint8_t)((seconds % 3600) / 60);
    ctx->time.seconds = (uint8_t)(seconds % 60);
}

/**
 * @brief Deliver an RTC event on the host.
 *
 * Called by the interrupt entry point. Only a registered callback is invoked, and an alarm event is
 * delivered only for an alarm that is enabled - the same rule the vendor backends follow by enabling
 * one interrupt source.
 */
void xy_hal_rtc_pc_irq(void *rtc, xy_hal_rtc_event_t event)
{
    pc_rtc_ctx_t *ctx = pc_rtc_find(rtc);
    size_t index = (size_t)event;

    if (ctx == NULL || ctx->initialized == 0U || index >= PC_RTC_EVENT_COUNT) {
        return;
    }
    if (index < PC_RTC_ALARM_COUNT && ctx->alarm_enabled[index] == 0U) {
        return;
    }
    if (ctx->callbacks[index] != NULL) {
        ctx->callbacks[index](event, ctx->args[index]);
    }
}

xy_hal_error_t xy_hal_rtc_backup_read(uint32_t index, uint32_t *value)
{
    if (value == NULL || index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    *value = backup_registers[index];
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rtc_backup_write(uint32_t index, uint32_t value)
{
    if (index >= XY_HAL_RTC_BACKUP_REGISTER_COUNT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    backup_registers[index] = value;
    return XY_HAL_OK;
}
