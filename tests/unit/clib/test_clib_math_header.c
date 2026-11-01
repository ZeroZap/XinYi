/**
 * @file test_clib_math_header.c
 * @brief `xy_math.c`'s own header must declare what its translation unit defines.
 *
 * Why this target exists next to `clib_math_stdlib`
 * -------------------------------------------------
 * That target includes `xy_stdlib.h` as well, and `xy_stdlib.h` still carries
 * the declarations the math library shared with it before the duplicate
 * definitions were removed (`xy_gcd`, `xy_lcm`, `xy_map`, `xy_constrain`,
 * `xy_factorial`). The definitions moved to `xy_math.c`; the declarations
 * stayed behind. So a consumer that includes the header owning the
 * implementation - `xy_math.h` - and calls `xy_gcd()` got an implicit
 * declaration: a C99 warning under the default flags, and a hard error under
 * `-Werror=implicit-function-declaration`. Nothing failed, because every test
 * that exercised the function also happened to include `xy_stdlib.h`.
 *
 * This file includes `xy_math.h` and nothing else, and the target is built with
 * `-Werror=implicit-function-declaration`. Both directions of the disagreement
 * are therefore errors rather than prose: a function the TU defines without the
 * header declaring it fails to compile, and a function the header declares
 * without any TU defining it fails to link (this target links `xy_math.c` and
 * nothing else).
 *
 * One thing this target cannot see by itself: it runs on the 64-bit host, where
 * `int32_t` *is* `int`, so a definition spelled `int32_t` against an `int`
 * declaration agrees there and disagrees only on a 32-bit target (where
 * `int32_t` is `long`). Making the header visible to the TU is what let the
 * `xy_gcd`/`xy_lcm`/`xy_constrain`/`xy_map` definitions surface at all - and the
 * STM32L4/U5 gates, not this test, are what report them (`conflicting types`).
 */

#include "unity.h"
#include "xy_math.h" /* the only product header - see the note above */

void setUp(void)
{
}

void tearDown(void)
{
}

/* Calls every function `xy_math.h` declares. The point is the call, not the
 * value: an undeclared name is a compile error here, and a declared-but-absent
 * definition is a link error. Values are asserted only where the contract is
 * exact and integer. */
static void test_every_declared_function_is_defined_by_this_translation_unit(void)
{
    xy_rand_state_t state;
    int32_t quotient;

    TEST_ASSERT_EQUAL_INT(7, xy_abs(-7));
    TEST_ASSERT_EQUAL_INT(7, (int)xy_labs(-7L));
    TEST_ASSERT_EQUAL_INT(7, (int)xy_llabs(-7LL));

    xy_div_t division = xy_div(7, 2);
    TEST_ASSERT_EQUAL_INT(3, division.quot);
    TEST_ASSERT_EQUAL_INT(1, division.rem);
    xy_ldiv_t long_division = xy_ldiv(7L, 2L);
    TEST_ASSERT_EQUAL_INT(3, (int)long_division.quot);
    xy_lldiv_t long_long_division = xy_lldiv(7LL, 2LL);
    TEST_ASSERT_EQUAL_INT(3, (int)long_long_division.quot);

    /* Integer utilities: the five the header used to omit. */
    TEST_ASSERT_EQUAL_INT(3, xy_gcd(12, 9));
    TEST_ASSERT_EQUAL_INT(36, xy_lcm(12, 18));
    TEST_ASSERT_EQUAL_INT(120, xy_factorial(5));
    TEST_ASSERT_EQUAL_INT(100, xy_constrain(150, 0, 100));
    TEST_ASSERT_EQUAL_INT(50, xy_map(5, 0, 10, 0, 100));

    /* Min/Max/Clamp are macros in the same header; calling them proves the
     * macro spelling a consumer would copy. */
    TEST_ASSERT_EQUAL_INT(3, xy_min(3, 9));
    TEST_ASSERT_EQUAL_INT(9, xy_max(3, 9));
    TEST_ASSERT_EQUAL_INT(9, xy_clamp(20, 0, 9));

    TEST_ASSERT_EQUAL_INT(1024, xy_pow(2, 10));
    TEST_ASSERT_EQUAL_INT(12, xy_sqrt(144));
    TEST_ASSERT_EQUAL_INT(3, xy_cbrt(27));

    TEST_ASSERT_EQUAL_INT32(10000, xy_sin_fixed(9000));
    TEST_ASSERT_EQUAL_INT32(10000, xy_cos_fixed(0));
    TEST_ASSERT_EQUAL_INT32(10000, xy_tan_fixed(4500));

    TEST_ASSERT_EQUAL_INT(4, xy_popcount(0xF0U));
    TEST_ASSERT_EQUAL_INT(24, xy_clz(0x000000FFU));
    TEST_ASSERT_EQUAL_INT(8, xy_ctz(0x00000100U));
    TEST_ASSERT_EQUAL_UINT(128, xy_round_up_pow2(100));
    TEST_ASSERT_EQUAL_INT(1, xy_is_pow2(64));

    /* Q16.16: 1.5 * 2.0 = 3.0 */
    TEST_ASSERT_EQUAL_INT32(3 * 65536, xy_fixed_mul(1.5 * 65536, 2.0 * 65536));
    quotient = xy_fixed_div(3 * 65536, 2 * 65536);
    TEST_ASSERT_EQUAL_INT32(1 * 65536 + 32768, quotient);

    xy_rand_init(&state, 12345U);
    TEST_ASSERT_TRUE(xy_rand(&state) >= 0);
    int32_t ranged = xy_rand_range(&state, 10, 20);
    TEST_ASSERT_TRUE(ranged >= 10 && ranged <= 20);

    /* Floating-point approximations: compared with a tolerance, because they
     * are approximations by design. */
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, xy_fast_inv_sqrt(4.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, xy_fast_sin(1.5707963f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, xy_fast_cos(0.0f));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_every_declared_function_is_defined_by_this_translation_unit);
    return UNITY_END();
}
