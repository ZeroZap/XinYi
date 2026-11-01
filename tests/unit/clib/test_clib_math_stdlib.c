/**
 * @file test_clib_math_stdlib.c
 * @brief One implementation per CLIB symbol: conversions, integer and fixed-point math.
 *
 * Why this target links three translation units into one executable
 * -----------------------------------------------------------------
 * `libxy_xy_clib.a` shipped two implementations of 28 symbols: xy_math.c's whole
 * API plus xy_atof/xy_atoi/xy_atol/xy_atoll appeared a second time in
 * xy_stdlib.c (conversions a third time in xy_stdio.c). Measured on the PC
 * archive before the fix, that had two different symptoms depending on which
 * symbols the consumer referenced:
 *
 *   1. a consumer using only stdio/math symbols never pulled xy_stdlib.o, so
 *      nothing complained — and got xy_stdio.c's "unsupported" xy_atof, i.e.
 *      0.0 for every input, while a real parser sat unread in the same archive;
 *   2. a consumer that also needed one stdlib-only symbol (xy_itoa and friends)
 *      pulled a second member defining the same names, and the link failed:
 *      "multiple definition of `xy_atof' ... xy_stdio.c.o: first defined here".
 *
 * Linking the three files here makes case 2 permanent: reintroducing a duplicate
 * definition breaks this target's link instead of silently changing behaviour.
 * The assertions pin the surviving contract so a later "fix" has to be deliberate.
 */

#include <math.h>

#include "unity.h"
#include "xy_math.h"
#include "xy_stdlib.h"
#include "xy_stdio.h"

void setUp(void)
{
}

void tearDown(void)
{
}

/* --- string to integer conversions (owner: xy_stdio.c) --- */

static void test_atoi_parses_sign_and_stops_at_first_non_digit(void)
{
    TEST_ASSERT_EQUAL_INT(-123, xy_atoi("-123"));
    TEST_ASSERT_EQUAL_INT(123, xy_atoi("+123"));
    TEST_ASSERT_EQUAL_INT(42, xy_atoi("42abc"));
    TEST_ASSERT_EQUAL_INT(0, xy_atoi(""));
    TEST_ASSERT_EQUAL_INT(0, xy_atoi(NULL));
}

static void test_atol_and_atoll_share_the_conversion_contract(void)
{
    TEST_ASSERT_EQUAL_INT64(456L, xy_atol("456"));
    TEST_ASSERT_EQUAL_INT64(-456L, xy_atol("-456"));
    TEST_ASSERT_EQUAL_INT64(-789LL, xy_atoll("-789"));
    TEST_ASSERT_EQUAL_INT64(0LL, xy_atoll(NULL));
}

/* --- float parsing: currently unsupported, and that is the surviving contract ---
 *
 * xy_stdio.c carries an explicit design note (CLIB-005~007): float parsing is
 * considered out of scope for embedded builds, so xy_strtod/xy_strtof/xy_atof
 * return 0. The removed xy_stdlib.c copy implemented a real parser, which is why
 * the same archive answered both ways. Keeping the stub is the behaviour-neutral
 * choice (nothing that linked before changes answer); `xy_clib_apis.md` marks
 * xy_atof as "已实现", so the documentation is the thing that is wrong here.
 * Deciding to actually implement double parsing is a product call, not a
 * side effect of de-duplication.
 */

static void test_float_parsing_is_unsupported_today(void)
{
    /* Unity in this tree is built without UNITY_INCLUDE_DOUBLE, so compare a
     * scaled integer: "3.5" would read 35 if a parser ever takes over. */
    TEST_ASSERT_EQUAL_INT(0, (int)(xy_atof("3.5") * 10.0));
    TEST_ASSERT_EQUAL_INT(0, (int)(xy_atof("-2.25") * 10.0));
    TEST_ASSERT_EQUAL_INT(0, (int)(xy_strtod("3.5", NULL) * 10.0));
    TEST_ASSERT_EQUAL_INT(0, (int)((double)xy_strtof("3.5", NULL) * 10.0));
}

/* --- integer math (owner: xy_math.c) --- */

static void test_basic_integer_helpers(void)
{
    TEST_ASSERT_EQUAL_INT(7, xy_abs(-7));
    TEST_ASSERT_EQUAL_INT(7, xy_abs(7));
    TEST_ASSERT_EQUAL_INT(120, xy_factorial(5));
    TEST_ASSERT_EQUAL_INT(1, xy_factorial(0));
}

static void test_integer_roots_and_powers(void)
{
    TEST_ASSERT_EQUAL_INT(12, xy_sqrt(144));
    TEST_ASSERT_EQUAL_INT(1, xy_sqrt(2));
    TEST_ASSERT_EQUAL_INT(3, xy_cbrt(27));
    TEST_ASSERT_EQUAL_INT(1024, xy_pow(2, 10));
    TEST_ASSERT_EQUAL_INT(1, xy_pow(7, 0));
}

static void test_gcd_lcm_are_positive_and_ordered(void)
{
    TEST_ASSERT_EQUAL_INT(6, xy_gcd(12, 18));
    TEST_ASSERT_EQUAL_INT(36, xy_lcm(12, 18));
}

static void test_power_of_two_range_and_mapping_helpers(void)
{
    TEST_ASSERT_EQUAL_INT(1, xy_is_pow2(64));
    TEST_ASSERT_EQUAL_INT(0, xy_is_pow2(100));
    TEST_ASSERT_EQUAL_UINT(128, xy_round_up_pow2(100));
    TEST_ASSERT_EQUAL_UINT(64, xy_round_up_pow2(64));
    TEST_ASSERT_EQUAL_INT(100, xy_constrain(150, 0, 100));
    TEST_ASSERT_EQUAL_INT(0, xy_constrain(-5, 0, 100));
    TEST_ASSERT_EQUAL_INT(50, xy_map(5, 0, 10, 0, 100));
}

/* --- fixed-point trigonometry (owner: xy_math.c) ---
 *
 * Angle in degrees x100, result is sin x10000, and `sin_table[d]` is
 * floor(sin(d degrees) * 10000) for every whole degree d in 0..90. Fractional
 * hundredths are truncated because the table is integer-indexed.
 *
 * The mirror index in quadrants 1 and 3 is a contract, not an implementation
 * detail. Those branches used to read `sin_table[89 - x]`, i.e. the entry for
 * the degree *before* the mirror, so an exact right angle read 9998 instead of
 * 10000 and sin(91) read 9993 where sin(91) is 9998 - five LSBs off. Spot
 * checks did not show it because the pinned angles whose mirrored entry
 * happened to land within a LSB looked right. The two loops below compare every
 * whole degree against libm, so a mirrored-quadrant regression fails on the
 * degree it breaks instead of waiting for someone to try 91.
 */

static double degrees_to_radians(int32_t degrees)
{
    return (double)degrees * 3.14159265358979323846 / 180.0;
}

static void test_sin_fixed_tracks_libm_at_every_whole_degree(void)
{
    for (int32_t degrees = 0; degrees < 360; degrees++) {
        const double expected = sin(degrees_to_radians(degrees)) * 10000.0;
        const int32_t actual = xy_sin_fixed(degrees * 100);
        /* One LSB: the table truncates toward zero, so a negative half sits one
         * LSB above floor(sin * 10000). */
        TEST_ASSERT_FLOAT_WITHIN(1.0f, (float)expected, (float)actual);
    }
}

static void test_cos_fixed_tracks_libm_at_every_whole_degree(void)
{
    for (int32_t degrees = 0; degrees < 360; degrees++) {
        const double expected = cos(degrees_to_radians(degrees)) * 10000.0;
        const int32_t actual = xy_cos_fixed(degrees * 100);
        TEST_ASSERT_FLOAT_WITHIN(1.0f, (float)expected, (float)actual);
    }
}

static void test_sin_fixed_quadrants(void)
{
    TEST_ASSERT_EQUAL_INT32(0, xy_sin_fixed(0));
    TEST_ASSERT_EQUAL_INT32(5000, xy_sin_fixed(3000));
    TEST_ASSERT_EQUAL_INT32(7071, xy_sin_fixed(4500));
    TEST_ASSERT_EQUAL_INT32(8660, xy_sin_fixed(6000));
    TEST_ASSERT_EQUAL_INT32(10000, xy_sin_fixed(9000));
    TEST_ASSERT_EQUAL_INT32(9998, xy_sin_fixed(9100));
    TEST_ASSERT_EQUAL_INT32(0, xy_sin_fixed(18000));
    TEST_ASSERT_EQUAL_INT32(-9998, xy_sin_fixed(26900));
    TEST_ASSERT_EQUAL_INT32(-10000, xy_sin_fixed(27000));
}

static void test_sin_fixed_normalises_out_of_range_angles(void)
{
    TEST_ASSERT_EQUAL_INT32(-10000, xy_sin_fixed(-9000));
    /* 369.00 degrees normalises to 9 degrees. */
    TEST_ASSERT_EQUAL_INT32(1564, xy_sin_fixed(36900));
    /* 364.50 degrees normalises to 4 degrees: the fractional hundredth is
     * truncated, not rounded, because the table is integer-indexed. */
    TEST_ASSERT_EQUAL_INT32(697, xy_sin_fixed(36450));
    TEST_ASSERT_EQUAL_INT32(-9998, xy_sin_fixed(-9100));
}

static void test_cos_and_tan_fixed_are_derived_from_sin(void)
{
    TEST_ASSERT_EQUAL_INT32(10000, xy_cos_fixed(0));
    TEST_ASSERT_EQUAL_INT32(0, xy_cos_fixed(9000));
    TEST_ASSERT_EQUAL_INT32(-10000, xy_cos_fixed(18000));
    TEST_ASSERT_EQUAL_INT32(10000, xy_tan_fixed(4500));
    TEST_ASSERT_EQUAL_INT32(-10000, xy_tan_fixed(13500));
    TEST_ASSERT_EQUAL_INT32(0, xy_tan_fixed(0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_atoi_parses_sign_and_stops_at_first_non_digit);
    RUN_TEST(test_atol_and_atoll_share_the_conversion_contract);
    RUN_TEST(test_float_parsing_is_unsupported_today);
    RUN_TEST(test_basic_integer_helpers);
    RUN_TEST(test_integer_roots_and_powers);
    RUN_TEST(test_gcd_lcm_are_positive_and_ordered);
    RUN_TEST(test_power_of_two_range_and_mapping_helpers);
    RUN_TEST(test_sin_fixed_tracks_libm_at_every_whole_degree);
    RUN_TEST(test_cos_fixed_tracks_libm_at_every_whole_degree);
    RUN_TEST(test_sin_fixed_quadrants);
    RUN_TEST(test_sin_fixed_normalises_out_of_range_angles);
    RUN_TEST(test_cos_and_tan_fixed_are_derived_from_sin);
    return UNITY_END();
}
