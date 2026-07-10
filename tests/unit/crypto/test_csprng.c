/**
 * @file test_csprng.c
 * @brief Unit tests for ChaCha20-based CSPRNG lifecycle and buffering.
 */

#include "xy_rng.h"
#include "unity.h"

#include <stdint.h>
#include <string.h>

static uint8_t seed[48];
static uint8_t entropy[16];

void setUp(void)
{
}

void tearDown(void)
{
}

static void fill_fixtures(void)
{
    for (uint8_t i = 0; i < sizeof(seed); i++) {
        seed[i] = (uint8_t)(i * 3U + 1U);
    }
    for (uint8_t i = 0; i < sizeof(entropy); i++) {
        entropy[i] = (uint8_t)(0xA5U ^ i);
    }
}

static int all_zero(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (buf[i] != 0U) {
            return 0;
        }
    }
    return 1;
}

static void test_lifecycle_and_params(void)
{
    uint8_t out[16];

    xy_csprng_cleanup();
    TEST_ASSERT_EQUAL_INT(XY_RNG_NOT_INITIALIZED, xy_csprng_generate(out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(XY_RNG_NOT_INITIALIZED, xy_csprng_reseed(entropy, sizeof(entropy)));
    TEST_ASSERT_EQUAL_INT(XY_RNG_INVALID_PARAM, xy_csprng_init(NULL, sizeof(seed)));
    TEST_ASSERT_EQUAL_INT(XY_RNG_INVALID_PARAM, xy_csprng_init(seed, 31));

    TEST_ASSERT_EQUAL_INT(XY_RNG_SUCCESS, xy_csprng_init(seed, sizeof(seed)));
    TEST_ASSERT_EQUAL_INT(XY_RNG_INVALID_PARAM, xy_csprng_generate(NULL, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(XY_RNG_INVALID_PARAM, xy_csprng_generate(out, 0));
    TEST_ASSERT_EQUAL_INT(XY_RNG_INVALID_PARAM, xy_csprng_reseed(NULL, sizeof(entropy)));
    TEST_ASSERT_EQUAL_INT(XY_RNG_INVALID_PARAM, xy_csprng_reseed(entropy, 0));
    xy_csprng_cleanup();
}

static void test_deterministic_output_and_buffering(void)
{
    uint8_t split[80];
    uint8_t full[80];

    TEST_ASSERT_EQUAL_INT(XY_RNG_SUCCESS, xy_csprng_init(seed, sizeof(seed)));
    TEST_ASSERT_EQUAL_INT(XY_RNG_SUCCESS, xy_csprng_generate(split, 13));
    TEST_ASSERT_EQUAL_INT(XY_RNG_SUCCESS, xy_csprng_generate(split + 13, sizeof(split) - 13));
    TEST_ASSERT_FALSE(all_zero(split, sizeof(split)));
    xy_csprng_cleanup();

    TEST_ASSERT_EQUAL_INT(XY_RNG_SUCCESS, xy_csprng_init(seed, sizeof(seed)));
    TEST_ASSERT_EQUAL_INT(XY_RNG_SUCCESS, xy_csprng_generate(full, sizeof(full)));
    TEST_ASSERT_EQUAL_MEMORY(split, full, sizeof(full));
    xy_csprng_cleanup();
}

static void test_reseed_and_integer_helpers(void)
{
    uint8_t before[32];
    uint8_t after[32];
    uint32_t value32;
    uint64_t value64;

    TEST_ASSERT_EQUAL_INT(XY_RNG_SUCCESS, xy_csprng_init(seed, sizeof(seed)));
    TEST_ASSERT_EQUAL_INT(XY_RNG_SUCCESS, xy_csprng_generate(before, sizeof(before)));
    TEST_ASSERT_EQUAL_INT(XY_RNG_SUCCESS, xy_csprng_reseed(entropy, sizeof(entropy)));
    TEST_ASSERT_EQUAL_INT(XY_RNG_SUCCESS, xy_csprng_generate(after, sizeof(after)));
    TEST_ASSERT_NOT_EQUAL(0, memcmp(before, after, sizeof(before)));

    value32 = xy_csprng_uint32();
    value64 = xy_csprng_uint64();
    TEST_ASSERT_NOT_EQUAL(0ULL, ((uint64_t)value32 | value64));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_csprng_uniform(0));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_csprng_uniform(1));
    for (int i = 0; i < 32; i++) {
        TEST_ASSERT_LESS_THAN_UINT32(7U, xy_csprng_uniform(7));
    }
    xy_csprng_cleanup();
}

int main(void)
{
    UNITY_BEGIN();
    fill_fixtures();
    RUN_TEST(test_lifecycle_and_params);
    RUN_TEST(test_deterministic_output_and_buffering);
    RUN_TEST(test_reseed_and_integer_helpers);
    return UNITY_END();
}
