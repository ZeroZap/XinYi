/**
 * @file xy_bcd.h
 * @brief BCD digits, and the one place that decides how an RTC year is spelled
 *
 * The three STM32 RTC backends each converted the year differently, and two of them were wrong:
 *
 *   - L4 read a full year in BCD mode (2000 + bcd_to_bin) but a two-digit year in binary mode, so the
 *     same register produced 2026 or 26 depending on the argument the caller passed.
 *   - U5 read a two-digit year in BCD mode and wrote bin_to_bcd(date->year) - handing 2026 to a
 *     function that takes a two-digit value, so the written year was a truncation of it.
 *   - F4 added 2000 to the register value in both modes, and wrote `year % 100` in both, so a BCD
 *     write put binary digits in a register the HAL reads as BCD.
 *
 * The contract this file implements: the year in xy_hal_rtc_date_t is a full year, always. The format
 * argument decides how the digits are spelled on the wire, not what the number means. The hardware
 * register holds two digits and the century is the board's convention, which is 2000 on these boards.
 *
 * Header-only for the same reason as xy_time_civil.h: a few pure functions, and no build directory to
 * keep re-configured.
 */

#ifndef XY_BCD_H
#define XY_BCD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The century the RTC year registers are read against on these boards. */
#define XY_BCD_CENTURY_BASE 2000U

/** Encode 0-99 as packed BCD. Values above 99 are a caller error and are truncated to two digits. */
static inline uint8_t xy_bcd_encode8(uint8_t value)
{
    return (uint8_t)((((value / 10U) % 10U) << 4) | (value % 10U));
}

/**
 * @brief Decode packed BCD into 0-99.
 * @return 0 if either nibble is not a digit, and the decoded value in *out otherwise.
 *
 * A nibble above nine is not a number, and an RTC that reads back 0x1A has not stored a time. Saying
 * so is better than returning 26 or 16 and letting a caller print it.
 */
static inline int xy_bcd_decode8(uint8_t value, uint8_t *out)
{
    uint8_t high = (uint8_t)(value >> 4);
    uint8_t low = (uint8_t)(value & 0x0FU);

    if (high > 9U || low > 9U) {
        return 0;
    }
    if (out != NULL) {
        *out = (uint8_t)(high * 10U + low);
    }
    return 1;
}

/** Encode 0-9999 as four BCD digits, most significant first: 2026 is 0x2026. */
static inline uint16_t xy_bcd_encode16(uint16_t value)
{
    uint16_t thousands = (uint16_t)((value / 1000U) % 10U);
    uint16_t hundreds = (uint16_t)((value / 100U) % 10U);
    uint16_t tens = (uint16_t)((value / 10U) % 10U);
    uint16_t units = (uint16_t)(value % 10U);

    return (uint16_t)((thousands << 12) | (hundreds << 8) | (tens << 4) | units);
}

/** Decode four BCD digits; 0 if any nibble is not a digit. */
static inline int xy_bcd_decode16(uint16_t value, uint16_t *out)
{
    uint16_t total = 0;
    int i;

    for (i = 0; i < 4; ++i) {
        uint16_t nibble = (uint16_t)((value >> (12 - 4 * i)) & 0x0FU);

        if (nibble > 9U) {
            return 0;
        }
        total = (uint16_t)(total * 10U + nibble);
    }
    if (out != NULL) {
        *out = total;
    }
    return 1;
}

/**
 * @brief Full year from the two-digit value an RTC register holds.
 *
 * @param raw   The register value as read, in whichever format it was read with.
 * @param bcd   Non-zero when `raw` is packed BCD.
 * @return The full year under XY_BCD_CENTURY_BASE, or -1 if the digits are not valid BCD.
 *
 * -1 rather than a fallback year: a register holding 0x1A is not a year, and silently reporting 2026
 * would hide a bus problem behind a plausible date.
 */
static inline int xy_rtc_year_decode(uint8_t raw, int bcd)
{
    uint8_t two_digit = raw;

    if (bcd != 0) {
        if (!xy_bcd_decode8(raw, &two_digit)) {
            return -1;
        }
    } else if (two_digit > 99U) {
        return -1;
    }
    return (int)(XY_BCD_CENTURY_BASE + two_digit);
}

/**
 * @brief Two-digit value to write into an RTC year register.
 *
 * @param year Full year (2026, not 26).
 * @param bcd  Non-zero to produce packed BCD.
 * @return The register value, or -1 for a year outside the century the register can represent.
 *
 * Refusing is the point: the register holds two digits, so 2178 becomes 78 if this truncates, and a
 * board that silently reports the wrong century is worse than one that reports an error.
 */
static inline int xy_rtc_year_encode(uint16_t year, int bcd)
{
    uint16_t two_digit;

    if (year < XY_BCD_CENTURY_BASE || year >= XY_BCD_CENTURY_BASE + 100U) {
        return -1;
    }
    two_digit = (uint16_t)(year - XY_BCD_CENTURY_BASE);
    return (int)(bcd != 0 ? xy_bcd_encode8((uint8_t)two_digit) : (uint8_t)two_digit);
}

#ifdef __cplusplus
}
#endif

#endif /* XY_BCD_H */
