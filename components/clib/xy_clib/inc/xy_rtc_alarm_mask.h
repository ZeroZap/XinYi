/**
 * @file xy_rtc_alarm_mask.h
 * @brief The alarm mask inversion, in one place
 *
 * Two vocabularies mean opposite things by the same four bits:
 *
 *   - `xy_hal_rtc_alarm_t.alarm_mask` says which fields **must match** for the alarm to fire.
 *   - The STM32 `RTC_ALARMMASK_*` bits say which fields to **skip**; a set bit means "do not compare
 *     this one".
 *
 * Passing one through as the other makes the alarm compare exactly the fields the caller asked to
 * ignore - and it still fires, at the wrong time, so nothing looks broken until the alarm is late.
 * Two of the three STM32 backends got this wrong in different ways: one passed the mask through
 * unchanged, the other never wrote the register at all and left every field compared.
 *
 * These helpers keep the numbering of the public header (0x01 weekday, 0x02 hours, 0x04 minutes,
 * 0x08 seconds) and take the "all fields" value from the caller, so a backend translates to its
 * vendor's constants on its own side of the boundary. Header-only: four pure functions with nothing
 * to mis-configure.
 */

#ifndef XY_RTC_ALARM_MASK_H
#define XY_RTC_ALARM_MASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Field bits, matching xy_hal_rtc.h's XY_HAL_RTC_ALARM_MASK_* values. */
#define XY_RTC_ALARM_FIELD_WEEKDAY 0x01U
#define XY_RTC_ALARM_FIELD_HOURS   0x02U
#define XY_RTC_ALARM_FIELD_MINUTES 0x04U
#define XY_RTC_ALARM_FIELD_SECONDS 0x08U
#define XY_RTC_ALARM_FIELD_ALL     0x0FU

/**
 * @brief The set of fields to skip, from the set the caller requires to match.
 *
 * @param must_match Fields that must match (public-header numbering).
 * @param all_fields The fields this backend can compare, e.g. XY_HAL_RTC_ALARM_MASK_ALL.
 *
 * Fields outside `all_fields` cannot be compared either way, so they are not reported as skipped -
 * they simply never take part.
 */
static inline uint8_t xy_rtc_alarm_skip_from_match(uint8_t must_match, uint8_t all_fields)
{
    return (uint8_t)(all_fields & (uint8_t)~must_match);
}

/** The inverse, for reading a vendor register back out to a caller. */
static inline uint8_t xy_rtc_alarm_match_from_skip(uint8_t skip, uint8_t all_fields)
{
    return (uint8_t)(all_fields & (uint8_t)~skip);
}

/**
 * @brief Whether a caller's mask is representable on this backend.
 *
 * A mask that asks for a field the hardware cannot compare would otherwise be silently dropped from
 * the match set, and the alarm would fire on a coarser condition than the caller asked for.
 */
static inline int xy_rtc_alarm_mask_valid(uint8_t must_match, uint8_t all_fields)
{
    return ((must_match & (uint8_t)~all_fields) == 0U) ? 1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* XY_RTC_ALARM_MASK_H */
