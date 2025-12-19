/**
 * @file xy_math.h
 * @brief XinYi Math Library - Optimized for embedded systems
 *
 * Provides software implementations of math operations for MCUs
 * without hardware division/multiplication support (e.g., Cortex-M0).
 *
 * Features:
 * - Software division (signed/unsigned, 32-bit/64-bit)
 * - Software multiplication
 * - Basic math functions (sqrt, pow, etc.)
 * - Optimized for code size and speed
 */

#ifndef _XY_MATH_H_
#define _XY_MATH_H_

#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Software Division Functions (for M0 MCU without hardware divider)
 * ======================================================================== */

/**
 * @defgroup soft_div Software Division
 * @brief Software division implementations for MCUs without hardware divider
 * @{
 */

/**
 * @brief 32-bit unsigned division
 * @param dividend Dividend (numerator)
 * @param divisor Divisor (denominator)
 * @return Quotient
 * @note Optimized for Cortex-M0, returns 0 if divisor is 0
 */
uint32_t xy_udiv32(uint32_t dividend, uint32_t divisor);

/**
 * @brief 32-bit signed division
 * @param dividend Dividend (numerator)
 * @param divisor Divisor (denominator)
 * @return Quotient
 * @note Returns 0 if divisor is 0
 */
int32_t xy_sdiv32(int32_t dividend, int32_t divisor);

/**
 * @brief 32-bit unsigned division with remainder
 * @param dividend Dividend (numerator)
 * @param divisor Divisor (denominator)
 * @param remainder Pointer to store remainder (can be NULL)
 * @return Quotient
 */
uint32_t xy_udivmod32(uint32_t dividend, uint32_t divisor, uint32_t *remainder);

/**
 * @brief 32-bit signed division with remainder
 * @param dividend Dividend (numerator)
 * @param divisor Divisor (denominator)
 * @param remainder Pointer to store remainder (can be NULL)
 * @return Quotient
 */
int32_t xy_sdivmod32(int32_t dividend, int32_t divisor, int32_t *remainder);

/**
 * @brief 64-bit unsigned division
 * @param dividend Dividend (numerator)
 * @param divisor Divisor (denominator)
 * @return Quotient
 */
uint64_t xy_udiv64(uint64_t dividend, uint64_t divisor);

/**
 * @brief 64-bit unsigned division with remainder
 * @param dividend Dividend (numerator)
 * @param divisor Divisor (denominator)
 * @param remainder Pointer to store remainder (can be NULL)
 * @return Quotient
 */
uint64_t xy_udivmod64(uint64_t dividend, uint64_t divisor, uint64_t *remainder);

/** @} */ /* end of soft_div */

/* ========================================================================
 * Software Multiplication Functions
 * ======================================================================== */

/**
 * @defgroup soft_mul Software Multiplication
 * @{
 */

/**
 * @brief 32-bit unsigned multiplication
 * @param a First operand
 * @param b Second operand
 * @return Product (lower 32 bits)
 */
uint32_t xy_umul32(uint32_t a, uint32_t b);

/**
 * @brief 32x32 -> 64-bit unsigned multiplication
 * @param a First operand
 * @param b Second operand
 * @return 64-bit product
 */
uint64_t xy_umul32x32(uint32_t a, uint32_t b);

/** @} */ /* end of soft_mul */

/* ========================================================================
 * Basic Math Functions
 * ======================================================================== */

/**
 * @defgroup basic_math Basic Math Operations
 * @{
 */

/**
 * @brief Integer square root (32-bit)
 * @param x Input value
 * @return Square root (floor value)
 * @note Uses binary search algorithm
 */
uint32_t xy_isqrt32(uint32_t x);

/**
 * @brief Integer square root (64-bit)
 * @param x Input value
 * @return Square root (floor value)
 */
uint32_t xy_isqrt64(uint64_t x);

/**
 * @brief Integer power (x^n)
 * @param base Base value
 * @param exp Exponent (must be >= 0)
 * @return base raised to the power of exp
 */
uint32_t xy_ipow(uint32_t base, uint32_t exp);

/**
 * @brief Greatest Common Divisor (GCD)
 * @param a First number
 * @param b Second number
 * @return GCD of a and b
 * @note Uses Euclidean algorithm
 */
uint32_t xy_gcd(uint32_t a, uint32_t b);

/**
 * @brief Least Common Multiple (LCM)
 * @param a First number
 * @param b Second number
 * @return LCM of a and b
 */
uint32_t xy_lcm(uint32_t a, uint32_t b);

/**
 * @brief Compute average without overflow
 * @param a First value
 * @param b Second value
 * @return Average of a and b
 */
uint32_t xy_avg(uint32_t a, uint32_t b);

/**
 * @brief Check if number is power of 2
 * @param x Number to check
 * @return 1 if power of 2, 0 otherwise
 */
int xy_is_power_of_2(uint32_t x);

/**
 * @brief Round up to next power of 2
 * @param x Input value
 * @return Next power of 2
 */
uint32_t xy_next_power_of_2(uint32_t x);

/**
 * @brief Count leading zeros (32-bit)
 * @param x Input value
 * @return Number of leading zero bits
 */
int xy_clz32(uint32_t x);

/**
 * @brief Count trailing zeros (32-bit)
 * @param x Input value
 * @return Number of trailing zero bits
 */
int xy_ctz32(uint32_t x);

/**
 * @brief Count set bits (population count)
 * @param x Input value
 * @return Number of 1 bits
 */
int xy_popcount32(uint32_t x);

/** @} */ /* end of basic_math */

/* ========================================================================
 * Fixed-Point Math (Q16.16 format)
 * ======================================================================== */

/**
 * @defgroup fixed_point Fixed-Point Math
 * @brief Q16.16 fixed-point arithmetic (16-bit integer, 16-bit fractional)
 * @{
 */

typedef int32_t xy_fixed_t;  /**< Q16.16 fixed-point type */

#define XY_FIXED_SHIFT 16
#define XY_FIXED_ONE (1 << XY_FIXED_SHIFT)

/**
 * @brief Convert integer to fixed-point
 * @param x Integer value
 * @return Fixed-point value
 */
#define xy_int_to_fixed(x) ((xy_fixed_t)((x) << XY_FIXED_SHIFT))

/**
 * @brief Convert fixed-point to integer (truncate)
 * @param x Fixed-point value
 * @return Integer value
 */
#define xy_fixed_to_int(x) ((int32_t)((x) >> XY_FIXED_SHIFT))

/**
 * @brief Fixed-point multiplication
 * @param a First operand
 * @param b Second operand
 * @return Product
 */
xy_fixed_t xy_fixed_mul(xy_fixed_t a, xy_fixed_t b);

/**
 * @brief Fixed-point division
 * @param a Dividend
 * @param b Divisor
 * @return Quotient
 */
xy_fixed_t xy_fixed_div(xy_fixed_t a, xy_fixed_t b);

/**
 * @brief Fixed-point square root
 * @param x Input value
 * @return Square root
 */
xy_fixed_t xy_fixed_sqrt(xy_fixed_t x);

/** @} */ /* end of fixed_point */

/* ========================================================================
 * Trigonometric Functions (using lookup tables)
 * ======================================================================== */

/**
 * @defgroup trig Trigonometric Functions
 * @brief Fast trigonometric functions using lookup tables
 * @{
 */

/**
 * @brief Fast sine (integer degrees, 0-359)
 * @param degrees Angle in degrees (0-359)
 * @return Sine value scaled by 32767 (Q0.15 format)
 */
int16_t xy_sin_deg(int16_t degrees);

/**
 * @brief Fast cosine (integer degrees, 0-359)
 * @param degrees Angle in degrees (0-359)
 * @return Cosine value scaled by 32767 (Q0.15 format)
 */
int16_t xy_cos_deg(int16_t degrees);

/**
 * @brief Fast tangent (integer degrees, 0-359)
 * @param degrees Angle in degrees (0-359)
 * @return Tangent value scaled by 32767, or INT16_MAX/MIN for overflow
 */
int16_t xy_tan_deg(int16_t degrees);

/** @} */ /* end of trig */

/* ========================================================================
 * Utility Macros
 * ======================================================================== */

/**
 * @defgroup math_macros Math Utility Macros
 * @{
 */

#ifndef XY_MIN
#define XY_MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef XY_MAX
#define XY_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef XY_CLAMP
#define XY_CLAMP(x, min, max) (XY_MIN(XY_MAX((x), (min)), (max)))
#endif

#ifndef XY_ABS
#define XY_ABS(x) ((x) < 0 ? -(x) : (x))
#endif

#ifndef XY_SIGN
#define XY_SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#endif

/** @} */ /* end of math_macros */

#ifdef __cplusplus
}
#endif

#endif /* _XY_MATH_H_ */