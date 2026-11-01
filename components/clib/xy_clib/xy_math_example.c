/**
 * @file xy_math_example.c
 * @brief Sample of the public xy_math.h API, compiled and run by CTest `clib_math_example`.
 *
 * This file is a build artifact, not prose. Nothing in the tree compiles
 * `*_example.c` (the clib library glob excludes it), so before this target
 * existed the sample could call sixteen functions that no header declares and
 * no build ever complained: `xy_udiv32`, `xy_isqrt32`, `xy_ipow`,
 * `xy_fixed_sqrt`, `xy_sin_deg`, `xy_stdio_printf` and friends. Every call
 * below is against a declaration that `xy_math.h` actually publishes, and
 * every value is checked against the header's documented contract.
 *
 * `main` returns non-zero when a check fails, which is what makes this a CTest
 * rather than a demonstration. There is deliberately no `assert()`: the
 * default build type is Release, where `NDEBUG` turns `assert()` into nothing
 * and the sample would silently check nothing at all.
 *
 * This file includes `xy_math.h` only — no `xy_stdlib.h`, no `xy_common.h` —
 * so it also proves the math header is self-contained for its own API.
 */

#include "xy_math.h"

#include <stdint.h>
#include <stdio.h>

static int g_checks;
static int g_failures;

static void check_int(const char *what, long expected, long actual)
{
    g_checks++;
    if (expected != actual) {
        g_failures++;
        printf("  FAIL %-40s expected %ld, got %ld\n", what, expected, actual);
        return;
    }
    printf("  ok   %-40s %ld\n", what, actual);
}

static void check_true(const char *what, int condition)
{
    check_int(what, 1, condition ? 1 : 0);
}

static void check_close(const char *what, float expected, float actual, float tolerance)
{
    float delta = actual - expected;

    g_checks++;
    if (delta < 0.0f) {
        delta = -delta;
    }
    if (delta > tolerance) {
        g_failures++;
        printf("  FAIL %-40s expected %.5f +/- %.5f, got %.5f\n", what, (double)expected,
               (double)tolerance, (double)actual);
        return;
    }
    printf("  ok   %-40s %.5f\n", what, (double)actual);
}

/* Q16.16 has no integer<->fixed helper in xy_math.h, so the sample shows the
 * shift the format is defined by: one whole unit is 1 << 16. */
#define FIXED_ONE ((int32_t)1 << 16)

static int32_t fixed_from_int(int value)
{
    return (int32_t)value * FIXED_ONE;
}

static int fixed_whole(int32_t value)
{
    return (int)(value / FIXED_ONE);
}

/* --- Basic arithmetic and the div() family --- */

static void example_division(void)
{
    xy_div_t d = xy_div(1000, 7);
    xy_ldiv_t ld = xy_ldiv(1000000L, 7L);
    xy_lldiv_t lld = xy_lldiv(1000000000LL, 999LL);

    printf("\n-- division --\n");
    check_int("xy_abs(-42)", 42, xy_abs(-42));
    check_int("xy_labs(-1234)", 1234L, xy_labs(-1234L));
    check_int("xy_llabs(-1234567890123)", 1234567890123LL, xy_llabs(-1234567890123LL));

    check_int("xy_div(1000,7).quot", 142, d.quot);
    check_int("xy_div(1000,7).rem", 6, d.rem);
    check_int("xy_ldiv(1000000,7).quot", 142857L, ld.quot);
    check_int("xy_ldiv(1000000,7).rem", 1L, ld.rem);
    check_int("xy_lldiv(1e9,999).quot", 1001001LL, lld.quot);
    check_int("xy_lldiv(1e9,999).rem", 1LL, lld.rem);
}

/* --- Min/max/clamp macros and the integer mapping helpers --- */

static void example_limits_and_mapping(void)
{
    printf("\n-- limits and mapping --\n");
    check_int("xy_min(3,9)", 3, xy_min(3, 9));
    check_int("xy_max(3,9)", 9, xy_max(3, 9));
    check_int("xy_clamp(15,0,10)", 10, xy_clamp(15, 0, 10));
    check_int("xy_clamp(-1,0,10)", 0, xy_clamp(-1, 0, 10));
    check_int("xy_clamp(5,0,10)", 5, xy_clamp(5, 0, 10));

    check_int("xy_constrain(15,0,10)", 10, xy_constrain(15, 0, 10));
    check_int("xy_constrain(-3,0,10)", 0, xy_constrain(-3, 0, 10));

    check_int("xy_map(50,0,100,0,1000)", 500, xy_map(50, 0, 100, 0, 1000));
    check_int("xy_map(0,0,100,0,1000)", 0, xy_map(0, 0, 100, 0, 1000));
    check_int("xy_map(1,0,0,7,99) degenerate", 7, xy_map(1, 0, 0, 7, 99));
}

/* --- Integer number theory helpers --- */

static void example_number_theory(void)
{
    printf("\n-- number theory --\n");
    check_int("xy_gcd(48,18)", 6, xy_gcd(48, 18));
    check_int("xy_gcd(17,19) coprime", 1, xy_gcd(17, 19));
    check_int("xy_lcm(48,18)", 144, xy_lcm(48, 18));
    check_int("xy_lcm(0,5)", 0, xy_lcm(0, 5));
    check_int("xy_factorial(0)", 1, xy_factorial(0));
    check_int("xy_factorial(5)", 120, xy_factorial(5));
    check_int("xy_factorial(-1)", 0, xy_factorial(-1));
}

/* --- Power and root functions --- */

static void example_power_and_roots(void)
{
    printf("\n-- power and roots --\n");
    check_int("xy_pow(2,10)", 1024, xy_pow(2, 10));
    check_int("xy_pow(3,8)", 6561, xy_pow(3, 8));
    check_int("xy_pow(7,0)", 1, xy_pow(7, 0));
    check_int("xy_sqrt(1024)", 32, xy_sqrt(1024));
    check_int("xy_sqrt(10000)", 100, xy_sqrt(10000));
    /* non-perfect squares floor: 31*31 = 961, 32*32 = 1024 */
    check_int("xy_sqrt(1000) floor", 31, xy_sqrt(1000));
    check_int("xy_cbrt(27)", 3, xy_cbrt(27));
    check_int("xy_cbrt(-27)", -3, xy_cbrt(-27));
}

/* --- Fixed-point trigonometry: degrees*100 in, value*10000 out --- */

static void example_trigonometry(void)
{
    printf("\n-- trigonometry (degrees x100, value x10000) --\n");
    check_int("xy_sin_fixed(0)", 0, xy_sin_fixed(0));
    check_int("xy_sin_fixed(3000) = sin 30", 5000, xy_sin_fixed(3000));
    check_int("xy_sin_fixed(9000) = sin 90", 10000, xy_sin_fixed(9000));
    check_int("xy_sin_fixed(18000) = sin 180", 0, xy_sin_fixed(18000));
    check_int("xy_sin_fixed(27000) = sin 270", -10000, xy_sin_fixed(27000));
    check_int("xy_sin_fixed(-9000) wraps", -10000, xy_sin_fixed(-9000));
    check_int("xy_sin_fixed(45000) wraps", 10000, xy_sin_fixed(45000));

    check_int("xy_cos_fixed(0)", 10000, xy_cos_fixed(0));
    check_int("xy_cos_fixed(9000) = cos 90", 0, xy_cos_fixed(9000));
    check_int("xy_cos_fixed(18000) = cos 180", -10000, xy_cos_fixed(18000));

    check_int("xy_tan_fixed(4500) = tan 45", 10000, xy_tan_fixed(4500));
    check_int("xy_tan_fixed(0)", 0, xy_tan_fixed(0));
    /* 90 degrees has no tangent; the implementation saturates on its sentinel
     * instead of dividing by zero. That sentinel is part of the contract. */
    check_int("xy_tan_fixed(9000) saturates", 1000000, xy_tan_fixed(9000));
}

/* --- Bit manipulation --- */

static void example_bit_ops(void)
{
    printf("\n-- bit manipulation --\n");
    check_int("xy_popcount(0xFF)", 8, xy_popcount(0xFFU));
    check_int("xy_popcount(0)", 0, xy_popcount(0U));
    check_int("xy_clz(0x00001234)", 19, xy_clz(0x00001234U));
    check_int("xy_clz(0) documented full width", 32, xy_clz(0U));
    check_int("xy_ctz(0x00001234)", 2, xy_ctz(0x00001234U));
    check_int("xy_ctz(0x80000000)", 31, xy_ctz(0x80000000U));

    check_int("xy_round_up_pow2(100)", 128, (long)xy_round_up_pow2(100U));
    check_int("xy_round_up_pow2(64) already pow2", 64, (long)xy_round_up_pow2(64U));
    check_int("xy_round_up_pow2(0)", 1, (long)xy_round_up_pow2(0U));
    check_int("xy_is_pow2(64)", 1, xy_is_pow2(64U));
    check_int("xy_is_pow2(100)", 0, xy_is_pow2(100U));
    check_int("xy_is_pow2(0)", 0, xy_is_pow2(0U));
}

/* --- Q16.16 fixed point --- */

static void example_fixed_point(void)
{
    int32_t five = fixed_from_int(5);
    int32_t three = fixed_from_int(3);
    int32_t ten = fixed_from_int(10);
    int32_t four = fixed_from_int(4);
    int32_t product = xy_fixed_mul(five, three);
    int32_t quotient = xy_fixed_div(ten, four);

    printf("\n-- Q16.16 fixed point --\n");
    check_int("fixed_from_int(5) raw", 327680, five);
    check_int("5.0 * 3.0 whole", 15, fixed_whole(product));
    check_int("5.0 * 3.0 raw", 15 * FIXED_ONE, product);
    check_int("10.0 / 4.0 whole", 2, fixed_whole(quotient));
    check_int("10.0 / 4.0 half (0.5 unit)", FIXED_ONE / 2, quotient - 2 * FIXED_ONE);
    /* Division by zero is refused with 0 rather than trapping. */
    check_int("xy_fixed_div(x,0)", 0, xy_fixed_div(ten, 0));
}

/* --- Pseudo random numbers --- */

static void example_random(void)
{
    xy_rand_state_t state;
    int32_t first;
    int32_t second;
    int32_t replay_first;
    int32_t ranged;

    printf("\n-- pseudo random --\n");
    xy_rand_init(&state, 12345U);
    first = xy_rand(&state);
    second = xy_rand(&state);
    check_true("xy_rand() non-negative", first >= 0 && second >= 0);

    /* Same seed, same stream: that is the whole contract of xy_rand_init. */
    xy_rand_init(&state, 12345U);
    replay_first = xy_rand(&state);
    check_int("xy_rand() replays the same seed", (long)first, replay_first);

    ranged = xy_rand_range(&state, 10, 20);
    check_true("xy_rand_range() inside [10,20]", ranged >= 10 && ranged <= 20);
    check_int("xy_rand_range(m,m)", 7, xy_rand_range(&state, 7, 7));
}

/* --- Float approximations (no libm, no FPU required) --- */

static void example_float_approximations(void)
{
    printf("\n-- float approximations --\n");
    /* Quake's fast inverse square root is a first-order Newton step: it is an
     * approximation, so the sample pins the tolerance as well as the value. */
    check_close("xy_fast_inv_sqrt(4.0)", 0.5f, xy_fast_inv_sqrt(4.0f), 0.002f);
    check_close("xy_fast_sin(0.0)", 0.0f, xy_fast_sin(0.0f), 0.0001f);
    /* xy_fast_sin is a fifth-order Taylor series with no range reduction, so
     * it overshoots 1.0 by the omitted x^7/5040 term (~0.0045 at pi/2), and
     * xy_fast_cos(0) is defined as xy_fast_sin(pi/2) and carries the same
     * error. Pinning 1e-4 here would teach an accuracy the code never had;
     * the bound below is the measured truncation error. */
    check_close("xy_fast_cos(0.0) = fast_sin(pi/2)", 1.0f, xy_fast_cos(0.0f), 0.005f);
    check_close("xy_fast_sin(pi/2)", 1.0f, xy_fast_sin(1.57079632f), 0.005f);
}

int main(void)
{
    printf("XY_MATH example: samples of the public xy_math.h API\n");

    example_division();
    example_limits_and_mapping();
    example_number_theory();
    example_power_and_roots();
    example_trigonometry();
    example_bit_ops();
    example_fixed_point();
    example_random();
    example_float_approximations();

    printf("\n%d checks, %d failure(s)\n", g_checks, g_failures);
    return (g_failures == 0) ? 0 : 1;
}
