/**
 * @file test_blake2.c
 * @brief BLAKE2 focused public-contract tests.
 *
 * These are host contract guards only. They do not claim security/provenance
 * review, side-channel review, or hardware acceleration validation.
 */

#include "unity.h"
#include "xy_blake2.h"

#include <stdint.h>
#include <string.h>

static const uint8_t blake2s_empty[XY_BLAKE2S_OUTBYTES] = {
    0x69, 0x21, 0x7a, 0x30, 0x79, 0x90, 0x80, 0x94,
    0xe1, 0x11, 0x21, 0xd0, 0x42, 0x35, 0x4a, 0x7c,
    0x1f, 0x55, 0xb6, 0x48, 0x2c, 0xa1, 0xa5, 0x1e,
    0x1b, 0x25, 0x0d, 0xfd, 0x1e, 0xd0, 0xee, 0xf9,
};

static const uint8_t blake2s_abc[XY_BLAKE2S_OUTBYTES] = {
    0x50, 0x8c, 0x5e, 0x8c, 0x32, 0x7c, 0x14, 0xe2,
    0xe1, 0xa7, 0x2b, 0xa3, 0x4e, 0xeb, 0x45, 0x2f,
    0x37, 0x45, 0x8b, 0x20, 0x9e, 0xd6, 0x3a, 0x29,
    0x4d, 0x99, 0x9b, 0x4c, 0x86, 0x67, 0x59, 0x82,
};

static const uint8_t blake2s_abc_keyed[XY_BLAKE2S_OUTBYTES] = {
    0x3f, 0x97, 0x23, 0x43, 0x7b, 0x03, 0x3b, 0xf0,
    0xc1, 0xf4, 0xdf, 0x43, 0xca, 0xfd, 0x07, 0x76,
    0x06, 0x8c, 0xb0, 0xa9, 0x59, 0x12, 0xde, 0x13,
    0xf3, 0xb2, 0x95, 0x2a, 0x3a, 0xba, 0x76, 0x4d,
};

void setUp(void)
{
}

void tearDown(void)
{
}

static void assert_blake2s_digest_equal(const uint8_t expected[XY_BLAKE2S_OUTBYTES],
                                        const uint8_t actual[XY_BLAKE2S_OUTBYTES])
{
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, actual, XY_BLAKE2S_OUTBYTES);
}

void test_blake2s_one_shot_matches_known_vectors(void)
{
    uint8_t digest[XY_BLAKE2S_OUTBYTES];

    memset(digest, 0xa5, sizeof(digest));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_SUCCESS,
                          xy_blake2s(digest, sizeof(digest), NULL, 0U, NULL, 0U));
    assert_blake2s_digest_equal(blake2s_empty, digest);

    memset(digest, 0xa5, sizeof(digest));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_SUCCESS,
                          xy_blake2s(digest, sizeof(digest), (const uint8_t *)"abc", 3U,
                                      NULL, 0U));
    assert_blake2s_digest_equal(blake2s_abc, digest);
}

void test_blake2s_incremental_matches_one_shot(void)
{
    xy_blake2s_ctx_t ctx;
    uint8_t incremental[XY_BLAKE2S_OUTBYTES];
    uint8_t one_shot[XY_BLAKE2S_OUTBYTES];
    const uint8_t part0[] = {'a'};
    const uint8_t part1[] = {'b', 'c'};

    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_SUCCESS, xy_blake2s_init(&ctx, sizeof(incremental)));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_SUCCESS, xy_blake2s_update(&ctx, part0, sizeof(part0)));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_SUCCESS, xy_blake2s_update(&ctx, part1, sizeof(part1)));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_SUCCESS,
                          xy_blake2s_final(&ctx, incremental, sizeof(incremental)));

    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_SUCCESS,
                          xy_blake2s(one_shot, sizeof(one_shot), (const uint8_t *)"abc", 3U,
                                      NULL, 0U));
    assert_blake2s_digest_equal(one_shot, incremental);
}

void test_blake2s_keyed_hash_matches_known_vector(void)
{
    uint8_t digest[XY_BLAKE2S_OUTBYTES];
    const uint8_t key[] = {'k', 'e', 'y'};

    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_SUCCESS,
                          xy_blake2s(digest, sizeof(digest), (const uint8_t *)"abc", 3U,
                                      key, sizeof(key)));
    assert_blake2s_digest_equal(blake2s_abc_keyed, digest);
}

void test_blake2s_rejects_invalid_params_and_preserves_output(void)
{
    xy_blake2s_ctx_t ctx;
    uint8_t digest[XY_BLAKE2S_OUTBYTES];
    uint8_t before[XY_BLAKE2S_OUTBYTES];
    const uint8_t payload[] = {'a', 'b', 'c'};

    memset(digest, 0x5a, sizeof(digest));
    memcpy(before, digest, sizeof(before));

    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_ERROR_INVALID_PARAM,
                          xy_blake2s(NULL, sizeof(digest), payload, sizeof(payload), NULL, 0U));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(before, digest, sizeof(digest));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_ERROR_INVALID_PARAM,
                          xy_blake2s(digest, 0U, payload, sizeof(payload), NULL, 0U));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(before, digest, sizeof(digest));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_ERROR_INVALID_PARAM,
                          xy_blake2s(digest, sizeof(digest) + 1U, payload, sizeof(payload),
                                      NULL, 0U));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(before, digest, sizeof(digest));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_ERROR_INVALID_PARAM,
                          xy_blake2s(digest, sizeof(digest), NULL, sizeof(payload), NULL, 0U));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(before, digest, sizeof(digest));

    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_ERROR_INVALID_PARAM, xy_blake2s_init(NULL, sizeof(digest)));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_ERROR_INVALID_PARAM, xy_blake2s_init(&ctx, 0U));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_ERROR_INVALID_PARAM,
                          xy_blake2s_init(&ctx, sizeof(digest) + 1U));

    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_SUCCESS, xy_blake2s_init(&ctx, sizeof(digest)));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_ERROR_INVALID_PARAM,
                          xy_blake2s_update(&ctx, NULL, sizeof(payload)));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_ERROR_INVALID_PARAM,
                          xy_blake2s_final(NULL, digest, sizeof(digest)));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_ERROR_INVALID_PARAM,
                          xy_blake2s_final(&ctx, NULL, sizeof(digest)));
    TEST_ASSERT_EQUAL_INT(XY_BLAKE2_ERROR_INVALID_PARAM,
                          xy_blake2s_final(&ctx, digest, sizeof(digest) - 1U));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_blake2s_one_shot_matches_known_vectors);
    RUN_TEST(test_blake2s_incremental_matches_one_shot);
    RUN_TEST(test_blake2s_keyed_hash_matches_known_vector);
    RUN_TEST(test_blake2s_rejects_invalid_params_and_preserves_output);
    return UNITY_END();
}
