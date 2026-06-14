/**
 * @file test_csprng.c
 * @brief Unit tests for ChaCha20-based CSPRNG lifecycle and buffering.
 */

#include "xy_rng.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static uint8_t seed[48];
static uint8_t entropy[16];

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
    assert(xy_csprng_generate(out, sizeof(out)) == XY_RNG_NOT_INITIALIZED);
    assert(xy_csprng_reseed(entropy, sizeof(entropy)) == XY_RNG_NOT_INITIALIZED);
    assert(xy_csprng_init(NULL, sizeof(seed)) == XY_RNG_INVALID_PARAM);
    assert(xy_csprng_init(seed, 31) == XY_RNG_INVALID_PARAM);

    assert(xy_csprng_init(seed, sizeof(seed)) == XY_RNG_SUCCESS);
    assert(xy_csprng_generate(NULL, sizeof(out)) == XY_RNG_INVALID_PARAM);
    assert(xy_csprng_generate(out, 0) == XY_RNG_INVALID_PARAM);
    assert(xy_csprng_reseed(NULL, sizeof(entropy)) == XY_RNG_INVALID_PARAM);
    assert(xy_csprng_reseed(entropy, 0) == XY_RNG_INVALID_PARAM);
    xy_csprng_cleanup();
}

static void test_deterministic_output_and_buffering(void)
{
    uint8_t split[80];
    uint8_t full[80];

    assert(xy_csprng_init(seed, sizeof(seed)) == XY_RNG_SUCCESS);
    assert(xy_csprng_generate(split, 13) == XY_RNG_SUCCESS);
    assert(xy_csprng_generate(split + 13, sizeof(split) - 13) == XY_RNG_SUCCESS);
    assert(!all_zero(split, sizeof(split)));
    xy_csprng_cleanup();

    assert(xy_csprng_init(seed, sizeof(seed)) == XY_RNG_SUCCESS);
    assert(xy_csprng_generate(full, sizeof(full)) == XY_RNG_SUCCESS);
    assert(memcmp(split, full, sizeof(full)) == 0);
    xy_csprng_cleanup();
}

static void test_reseed_and_integer_helpers(void)
{
    uint8_t before[32];
    uint8_t after[32];
    uint32_t value32;
    uint64_t value64;

    assert(xy_csprng_init(seed, sizeof(seed)) == XY_RNG_SUCCESS);
    assert(xy_csprng_generate(before, sizeof(before)) == XY_RNG_SUCCESS);
    assert(xy_csprng_reseed(entropy, sizeof(entropy)) == XY_RNG_SUCCESS);
    assert(xy_csprng_generate(after, sizeof(after)) == XY_RNG_SUCCESS);
    assert(memcmp(before, after, sizeof(before)) != 0);

    value32 = xy_csprng_uint32();
    value64 = xy_csprng_uint64();
    assert(value32 != 0U || value64 != 0ULL);
    assert(xy_csprng_uniform(0) == 0U);
    assert(xy_csprng_uniform(1) == 0U);
    for (int i = 0; i < 32; i++) {
        assert(xy_csprng_uniform(7) < 7U);
    }
    xy_csprng_cleanup();
}

int main(void)
{
    fill_fixtures();
    test_lifecycle_and_params();
    test_deterministic_output_and_buffering();
    test_reseed_and_integer_helpers();
    return 0;
}
