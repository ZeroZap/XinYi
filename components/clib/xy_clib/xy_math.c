/**
 * @file xy_math.c
 * @brief XinYi Math Library Implementation
 *
 * Optimized math functions for embedded systems, especially
 * for Cortex-M0 MCUs without hardware division/multiplication.
 */

#include "xy_math.h"

/* ========================================================================
 * Software Division Functions
 * ======================================================================== */

/**
 * @brief 32-bit unsigned division (optimized for M0)
 * @note Uses binary long division algorithm
 */
uint32_t xy_udiv32(uint32_t dividend, uint32_t divisor)
{
    uint32_t quotient, remainder;
    return xy_udivmod32(dividend, divisor, &remainder);
}

/**
 * @brief 32-bit unsigned division with remainder
 * @note Binary long division - O(32) iterations
 */
uint32_t xy_udivmod32(uint32_t dividend, uint32_t divisor, uint32_t *remainder)
{
    uint32_t quotient = 0;
    uint32_t rem = 0;
    int i;

    /* Handle division by zero */
    if (divisor == 0) {
        if (remainder) *remainder = 0;
        return 0;
    }

    /* Optimization: if divisor is power of 2 */
    if ((divisor & (divisor - 1)) == 0) {
        /* Count trailing zeros to get shift amount */
        int shift = 0;
        uint32_t temp = divisor;
        while ((temp & 1) == 0) {
            temp >>= 1;
            shift++;
        }
        quotient = dividend >> shift;
        if (remainder) {
            *remainder = dividend & (divisor - 1);
        }
        return quotient;
    }

    /* Binary long division */
    for (i = 31; i >= 0; i--) {
        rem = (rem << 1) | ((dividend >> i) & 1);
        if (rem >= divisor) {
            rem -= divisor;
            quotient |= (1U << i);
        }
    }

    if (remainder) {
        *remainder = rem;
    }

    return quotient;
}

/**
 * @brief 32-bit signed division
 */
int32_t xy_sdiv32(int32_t dividend, int32_t divisor)
{
    int32_t quotient, remainder;
    return xy_sdivmod32(dividend, divisor, &remainder);
}

/**
 * @brief 32-bit signed division with remainder
 */
int32_t xy_sdivmod32(int32_t dividend, int32_t divisor, int32_t *remainder)
{
    int sign = 1;
    uint32_t udividend, udivisor;
    uint32_t uquotient, uremainder;

    if (divisor == 0) {
        if (remainder) *remainder = 0;
        return 0;
    }

    /* Calculate sign of result */
    if (dividend < 0) {
        sign = -sign;
        udividend = (uint32_t)(-dividend);
    } else {
        udividend = (uint32_t)dividend;
    }

    if (divisor < 0) {
        sign = -sign;
        udivisor = (uint32_t)(-divisor);
    } else {
        udivisor = (uint32_t)divisor;
    }

    /* Perform unsigned division */
    uquotient = xy_udivmod32(udividend, udivisor, &uremainder);

    /* Apply sign */
    if (remainder) {
        *remainder = (dividend < 0) ? -(int32_t)uremainder : (int32_t)uremainder;
    }

    return (sign < 0) ? -(int32_t)uquotient : (int32_t)uquotient;
}

/**
 * @brief 64-bit unsigned division
 */
uint64_t xy_udiv64(uint64_t dividend, uint64_t divisor)
{
    uint64_t remainder;
    return xy_udivmod64(dividend, divisor, &remainder);
}

/**
 * @brief 64-bit unsigned division with remainder
 */
uint64_t xy_udivmod64(uint64_t dividend, uint64_t divisor, uint64_t *remainder)
{
    uint64_t quotient = 0;
    uint64_t rem = 0;
    int i;

    if (divisor == 0) {
        if (remainder) *remainder = 0;
        return 0;
    }

    /* Binary long division */
    for (i = 63; i >= 0; i--) {
        rem = (rem << 1) | ((dividend >> i) & 1);
        if (rem >= divisor) {
            rem -= divisor;
            quotient |= (1ULL << i);
        }
    }

    if (remainder) {
        *remainder = rem;
    }

    return quotient;
}

/* ========================================================================
 * Software Multiplication Functions
 * ======================================================================== */

/**
 * @brief 32-bit unsigned multiplication
 */
uint32_t xy_umul32(uint32_t a, uint32_t b)
{
    return (uint32_t)(xy_umul32x32(a, b) & 0xFFFFFFFFULL);
}

/**
 * @brief 32x32 -> 64-bit unsigned multiplication
 */
uint64_t xy_umul32x32(uint32_t a, uint32_t b)
{
    /* Break into 16-bit parts for better portability */
    uint32_t a_lo = a & 0xFFFF;
    uint32_t a_hi = a >> 16;
    uint32_t b_lo = b & 0xFFFF;
    uint32_t b_hi = b >> 16;

    uint64_t p0 = (uint64_t)a_lo * b_lo;
    uint64_t p1 = (uint64_t)a_lo * b_hi;
    uint64_t p2 = (uint64_t)a_hi * b_lo;
    uint64_t p3 = (uint64_t)a_hi * b_hi;

    uint64_t result = p0;
    result += (p1 << 16);
    result += (p2 << 16);
    result += (p3 << 32);

    return result;
}

/* ========================================================================
 * Basic Math Functions
 * ======================================================================== */

/**
 * @brief Integer square root (32-bit)
 * @note Uses Newton's method
 */
uint32_t xy_isqrt32(uint32_t x)
{
    uint32_t result = 0;
    uint32_t bit = 1U << 30; /* Second-to-top bit set */

    /* "bit" starts at the highest power of four <= x */
    while (bit > x) {
        bit >>= 2;
    }

    while (bit != 0) {
        if (x >= result + bit) {
            x -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }

    return result;
}

/**
 * @brief Integer square root (64-bit)
 */
uint32_t xy_isqrt64(uint64_t x)
{
    uint64_t result = 0;
    uint64_t bit = 1ULL << 62;

    while (bit > x) {
        bit >>= 2;
    }

    while (bit != 0) {
        if (x >= result + bit) {
            x -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }

    return (uint32_t)result;
}

/**
 * @brief Integer power (x^n)
 * @note Uses exponentiation by squaring
 */
uint32_t xy_ipow(uint32_t base, uint32_t exp)
{
    uint32_t result = 1;

    while (exp > 0) {
        if (exp & 1) {
            result *= base;
        }
        base *= base;
        exp >>= 1;
    }

    return result;
}

/**
 * @brief Greatest Common Divisor
 * @note Uses Euclidean algorithm
 */
uint32_t xy_gcd(uint32_t a, uint32_t b)
{
    uint32_t temp;

    while (b != 0) {
        temp = b;
        b = a % b;
        a = temp;
    }

    return a;
}

/**
 * @brief Least Common Multiple
 */
uint32_t xy_lcm(uint32_t a, uint32_t b)
{
    if (a == 0 || b == 0) {
        return 0;
    }
    return (a / xy_gcd(a, b)) * b;
}

/**
 * @brief Compute average without overflow
 */
uint32_t xy_avg(uint32_t a, uint32_t b)
{
    return (a & b) + ((a ^ b) >> 1);
}

/**
 * @brief Check if number is power of 2
 */
int xy_is_power_of_2(uint32_t x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

/**
 * @brief Round up to next power of 2
 */
uint32_t xy_next_power_of_2(uint32_t x)
{
    if (x == 0) return 1;

    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;

    return x;
}

/**
 * @brief Count leading zeros (32-bit)
 */
int xy_clz32(uint32_t x)
{
    int n = 0;

    if (x == 0) return 32;

    if ((x & 0xFFFF0000) == 0) { n += 16; x <<= 16; }
    if ((x & 0xFF000000) == 0) { n += 8;  x <<= 8;  }
    if ((x & 0xF0000000) == 0) { n += 4;  x <<= 4;  }
    if ((x & 0xC0000000) == 0) { n += 2;  x <<= 2;  }
    if ((x & 0x80000000) == 0) { n += 1; }

    return n;
}

/**
 * @brief Count trailing zeros (32-bit)
 */
int xy_ctz32(uint32_t x)
{
    int n = 0;

    if (x == 0) return 32;

    if ((x & 0x0000FFFF) == 0) { n += 16; x >>= 16; }
    if ((x & 0x000000FF) == 0) { n += 8;  x >>= 8;  }
    if ((x & 0x0000000F) == 0) { n += 4;  x >>= 4;  }
    if ((x & 0x00000003) == 0) { n += 2;  x >>= 2;  }
    if ((x & 0x00000001) == 0) { n += 1; }

    return n;
}

/**
 * @brief Count set bits (population count)
 */
int xy_popcount32(uint32_t x)
{
    /* Brian Kernighan's algorithm */
    int count = 0;
    while (x) {
        x &= x - 1;
        count++;
    }
    return count;
}

/* ========================================================================
 * Fixed-Point Math
 * ======================================================================== */

/**
 * @brief Fixed-point multiplication (Q16.16)
 */
xy_fixed_t xy_fixed_mul(xy_fixed_t a, xy_fixed_t b)
{
    int64_t temp = (int64_t)a * (int64_t)b;
    return (xy_fixed_t)(temp >> XY_FIXED_SHIFT);
}

/**
 * @brief Fixed-point division (Q16.16)
 */
xy_fixed_t xy_fixed_div(xy_fixed_t a, xy_fixed_t b)
{
    if (b == 0) return 0;

    int64_t temp = ((int64_t)a << XY_FIXED_SHIFT) / b;
    return (xy_fixed_t)temp;
}

/**
 * @brief Fixed-point square root (Q16.16)
 */
xy_fixed_t xy_fixed_sqrt(xy_fixed_t x)
{
    if (x <= 0) return 0;

    /* Convert to integer sqrt and adjust */
    uint32_t n = (uint32_t)x;
    uint32_t root = xy_isqrt32(n);

    return (xy_fixed_t)(root << (XY_FIXED_SHIFT / 2));
}

/* ========================================================================
 * Trigonometric Functions (using lookup tables)
 * ======================================================================== */

/* Sine lookup table for 0-90 degrees (Q0.15 format) */
static const int16_t sine_table_90[91] = {
    0,     572,   1144,  1715,  2286,  2856,  3425,  3993,  4560,  5126,  /* 0-9 */
    5690,  6252,  6813,  7371,  7927,  8481,  9032,  9580,  10126, 10668, /* 10-19 */
    11207, 11743, 12275, 12803, 13328, 13848, 14365, 14876, 15384, 15886, /* 20-29 */
    16384, 16877, 17364, 17847, 18324, 18795, 19261, 19720, 20174, 20622, /* 30-39 */
    21063, 21498, 21926, 22348, 22763, 23170, 23571, 23965, 24351, 24730, /* 40-49 */
    25102, 25466, 25822, 26170, 26510, 26843, 27168, 27485, 27795, 28096, /* 50-59 */
    28378, 28659, 28932, 29197, 29452, 29699, 29938, 30168, 30390, 30603, /* 60-69 */
    30807, 31003, 31190, 31368, 31538, 31699, 31851, 31995, 32130, 32256, /* 70-79 */
    32374, 32483, 32583, 32675, 32758, 32833, 32899, 32957, 33006, 33047, /* 80-89 */
    33079                                                                   /* 90 */
};

/**
 * @brief Fast sine (integer degrees)
 */
int16_t xy_sin_deg(int16_t degrees)
{
    int quadrant;
    int16_t angle;

    /* Normalize to 0-359 */
    while (degrees < 0) degrees += 360;
    while (degrees >= 360) degrees -= 360;

    /* Determine quadrant */
    quadrant = degrees / 90;
    angle = degrees % 90;

    switch (quadrant) {
        case 0: return sine_table_90[angle];
        case 1: return sine_table_90[90 - angle];
        case 2: return -sine_table_90[angle];
        case 3: return -sine_table_90[90 - angle];
        default: return 0;
    }
}

/**
 * @brief Fast cosine (integer degrees)
 */
int16_t xy_cos_deg(int16_t degrees)
{
    return xy_sin_deg(degrees + 90);
}

/**
 * @brief Fast tangent (integer degrees)
 */
int16_t xy_tan_deg(int16_t degrees)
{
    int16_t sin_val = xy_sin_deg(degrees);
    int16_t cos_val = xy_cos_deg(degrees);

    if (cos_val == 0) {
        return (sin_val >= 0) ? INT16_MAX : INT16_MIN;
    }

    /* tan = sin/cos, scale result */
    return (int16_t)(((int32_t)sin_val << 15) / cos_val);
}
