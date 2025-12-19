/**
 * @file test_xy_math.c
 * @brief Unit tests for xy_math library
 */

#include <stdint.h>
#include "xy_math.h"
#include "xy_stdio.h"

/* Test result counter */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            tests_passed++; \
        } else { \
            tests_failed++; \
            xy_stdio_printf("FAIL: %s\n", msg); \
        } \
    } while(0)

/* ========================================================================
 * Software Division Tests
 * ======================================================================== */

void test_udiv32(void)
{
    uint32_t result;

    xy_stdio_printf("\n=== Testing xy_udiv32 ===\n");

    result = xy_udiv32(100, 10);
    TEST_ASSERT(result == 10, "100/10 = 10");

    result = xy_udiv32(1000, 3);
    TEST_ASSERT(result == 333, "1000/3 = 333");

    result = xy_udiv32(0xFFFFFFFF, 1);
    TEST_ASSERT(result == 0xFFFFFFFF, "max/1 = max");

    result = xy_udiv32(128, 16);
    TEST_ASSERT(result == 8, "128/16 = 8 (power of 2 optimization)");

    result = xy_udiv32(100, 0);
    TEST_ASSERT(result == 0, "100/0 = 0 (div by zero)");
}

void test_sdiv32(void)
{
    int32_t result;

    xy_stdio_printf("\n=== Testing xy_sdiv32 ===\n");

    result = xy_sdiv32(100, 10);
    TEST_ASSERT(result == 10, "100/10 = 10");

    result = xy_sdiv32(-100, 10);
    TEST_ASSERT(result == -10, "-100/10 = -10");

    result = xy_sdiv32(100, -10);
    TEST_ASSERT(result == -10, "100/-10 = -10");

    result = xy_sdiv32(-100, -10);
    TEST_ASSERT(result == 10, "-100/-10 = 10");
}

void test_udivmod32(void)
{
    uint32_t quotient, remainder;

    xy_stdio_printf("\n=== Testing xy_udivmod32 ===\n");

    quotient = xy_udivmod32(100, 10, &remainder);
    TEST_ASSERT(quotient == 10 && remainder == 0, "100/10 = 10 r0");

    quotient = xy_udivmod32(100, 7, &remainder);
    TEST_ASSERT(quotient == 14 && remainder == 2, "100/7 = 14 r2");

    quotient = xy_udivmod32(1000, 33, &remainder);
    TEST_ASSERT(quotient == 30 && remainder == 10, "1000/33 = 30 r10");
}

void test_udiv64(void)
{
    uint64_t result;

    xy_stdio_printf("\n=== Testing xy_udiv64 ===\n");

    result = xy_udiv64(1000000000ULL, 1000);
    TEST_ASSERT(result == 1000000, "1B/1000 = 1M");

    result = xy_udiv64(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF);
    TEST_ASSERT(result == 0x100000001ULL, "max64/max32");
}

/* ========================================================================
 * Basic Math Tests
 * ======================================================================== */

void test_isqrt(void)
{
    uint32_t result;

    xy_stdio_printf("\n=== Testing xy_isqrt32 ===\n");

    result = xy_isqrt32(0);
    TEST_ASSERT(result == 0, "sqrt(0) = 0");

    result = xy_isqrt32(1);
    TEST_ASSERT(result == 1, "sqrt(1) = 1");

    result = xy_isqrt32(4);
    TEST_ASSERT(result == 2, "sqrt(4) = 2");

    result = xy_isqrt32(100);
    TEST_ASSERT(result == 10, "sqrt(100) = 10");

    result = xy_isqrt32(1024);
    TEST_ASSERT(result == 32, "sqrt(1024) = 32");

    result = xy_isqrt32(10000);
    TEST_ASSERT(result == 100, "sqrt(10000) = 100");

    result = xy_isqrt32(999);
    TEST_ASSERT(result == 31, "sqrt(999) = 31 (floor)");
}

void test_ipow(void)
{
    uint32_t result;

    xy_stdio_printf("\n=== Testing xy_ipow ===\n");

    result = xy_ipow(2, 0);
    TEST_ASSERT(result == 1, "2^0 = 1");

    result = xy_ipow(2, 1);
    TEST_ASSERT(result == 2, "2^1 = 2");

    result = xy_ipow(2, 8);
    TEST_ASSERT(result == 256, "2^8 = 256");

    result = xy_ipow(10, 3);
    TEST_ASSERT(result == 1000, "10^3 = 1000");

    result = xy_ipow(5, 5);
    TEST_ASSERT(result == 3125, "5^5 = 3125");
}

void test_gcd_lcm(void)
{
    uint32_t result;

    xy_stdio_printf("\n=== Testing xy_gcd/xy_lcm ===\n");

    result = xy_gcd(12, 8);
    TEST_ASSERT(result == 4, "gcd(12, 8) = 4");

    result = xy_gcd(100, 50);
    TEST_ASSERT(result == 50, "gcd(100, 50) = 50");

    result = xy_gcd(17, 19);
    TEST_ASSERT(result == 1, "gcd(17, 19) = 1 (coprime)");

    result = xy_lcm(12, 8);
    TEST_ASSERT(result == 24, "lcm(12, 8) = 24");

    result = xy_lcm(6, 9);
    TEST_ASSERT(result == 18, "lcm(6, 9) = 18");
}

void test_bit_ops(void)
{
    int result;
    uint32_t val;

    xy_stdio_printf("\n=== Testing bit operations ===\n");

    result = xy_is_power_of_2(16);
    TEST_ASSERT(result == 1, "16 is power of 2");

    result = xy_is_power_of_2(17);
    TEST_ASSERT(result == 0, "17 is not power of 2");

    val = xy_next_power_of_2(17);
    TEST_ASSERT(val == 32, "next_power_of_2(17) = 32");

    val = xy_next_power_of_2(64);
    TEST_ASSERT(val == 128, "next_power_of_2(64) = 128");

    result = xy_clz32(0x00000001);
    TEST_ASSERT(result == 31, "clz(0x00000001) = 31");

    result = xy_clz32(0x80000000);
    TEST_ASSERT(result == 0, "clz(0x80000000) = 0");

    result = xy_ctz32(0x00000001);
    TEST_ASSERT(result == 0, "ctz(0x00000001) = 0");

    result = xy_ctz32(0x80000000);
    TEST_ASSERT(result == 31, "ctz(0x80000000) = 31");

    result = xy_popcount32(0x0000000F);
    TEST_ASSERT(result == 4, "popcount(0x0F) = 4");

    result = xy_popcount32(0xFFFFFFFF);
    TEST_ASSERT(result == 32, "popcount(0xFFFFFFFF) = 32");
}

/* ========================================================================
 * Fixed-Point Math Tests
 * ======================================================================== */

void test_fixed_point(void)
{
    xy_fixed_t a, b, result;

    xy_stdio_printf("\n=== Testing fixed-point math ===\n");

    /* Test conversion */
    a = xy_int_to_fixed(5);
    TEST_ASSERT(xy_fixed_to_int(a) == 5, "int<->fixed conversion");

    /* Test multiplication: 2.5 * 3.0 = 7.5 */
    a = xy_int_to_fixed(5) >> 1;  /* 2.5 */
    b = xy_int_to_fixed(3);        /* 3.0 */
    result = xy_fixed_mul(a, b);
    TEST_ASSERT(xy_fixed_to_int(result) == 7, "2.5 * 3.0 = 7.5 (int part)");

    /* Test division: 10 / 4 = 2.5 */
    a = xy_int_to_fixed(10);
    b = xy_int_to_fixed(4);
    result = xy_fixed_div(a, b);
    TEST_ASSERT(xy_fixed_to_int(result) == 2, "10 / 4 = 2.5 (int part)");
}

/* ========================================================================
 * Trigonometric Tests
 * ======================================================================== */

void test_trig(void)
{
    int16_t result;

    xy_stdio_printf("\n=== Testing trigonometric functions ===\n");

    result = xy_sin_deg(0);
    TEST_ASSERT(result == 0, "sin(0) = 0");

    result = xy_sin_deg(90);
    TEST_ASSERT(result > 33000, "sin(90) ≈ 1.0 (scaled)");

    result = xy_sin_deg(180);
    TEST_ASSERT(result >= -10 && result <= 10, "sin(180) ≈ 0");

    result = xy_sin_deg(270);
    TEST_ASSERT(result < -33000, "sin(270) ≈ -1.0 (scaled)");

    result = xy_cos_deg(0);
    TEST_ASSERT(result > 33000, "cos(0) ≈ 1.0 (scaled)");

    result = xy_cos_deg(90);
    TEST_ASSERT(result >= -10 && result <= 10, "cos(90) ≈ 0");
}

/* ========================================================================
 * Performance Benchmark (optional)
 * ======================================================================== */

void benchmark_division(void)
{
    uint32_t i;
    volatile uint32_t result;

    xy_stdio_printf("\n=== Division benchmark ===\n");
    xy_stdio_printf("Performing 10000 divisions...\n");

    /* Benchmark software division */
    for (i = 0; i < 10000; i++) {
        result = xy_udiv32(0xFFFFFF, 1234);
    }

    xy_stdio_printf("Software division completed\n");
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

int main(void)
{
    xy_stdio_printf("\n");
    xy_stdio_printf("========================================\n");
    xy_stdio_printf("  XY_MATH Library Test Suite\n");
    xy_stdio_printf("========================================\n");

    /* Run all tests */
    test_udiv32();
    test_sdiv32();
    test_udivmod32();
    test_udiv64();
    test_isqrt();
    test_ipow();
    test_gcd_lcm();
    test_bit_ops();
    test_fixed_point();
    test_trig();

    /* Optional benchmark */
    /* benchmark_division(); */

    /* Print summary */
    xy_stdio_printf("\n");
    xy_stdio_printf("========================================\n");
    xy_stdio_printf("  Test Results\n");
    xy_stdio_printf("========================================\n");
    xy_stdio_printf("Passed: %d\n", tests_passed);
    xy_stdio_printf("Failed: %d\n", tests_failed);
    xy_stdio_printf("Total:  %d\n", tests_passed + tests_failed);
    xy_stdio_printf("========================================\n");

    return tests_failed == 0 ? 0 : 1;
}
