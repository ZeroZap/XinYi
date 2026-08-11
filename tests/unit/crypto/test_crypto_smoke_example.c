/**
 * @file test_crypto_smoke_example.c
 * @brief Build-guarded host-safe smoke example for active Crypto public APIs.
 *
 * This smoke intentionally uses deterministic public vectors plus non-exact
 * simple RNG shape checks. It is an API-drift guard only; it does not claim
 * cryptographic security review, side-channel safety, hardware acceleration,
 * or compliance certification.
 */

#include "unity.h"
#include "xy_tiny_crypto.h"

#include <stdint.h>
#include <string.h>

static const uint8_t abc_sha256[XY_SHA256_DIGEST_SIZE] = {
    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
    0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
    0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
};

void setUp(void)
{
}

void tearDown(void)
{
}

static int any_nonzero(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (buf[i] != 0U) {
            return 1;
        }
    }
    return 0;
}

static void test_encode_hash_rng_public_flow(void)
{
    const uint8_t payload[] = {'X', 'i', 'n', 'Y', 'i'};
    char b64[16] = {0};
    char hex[16] = {0};
    uint8_t digest[XY_SHA256_DIGEST_SIZE] = {0};
    uint8_t random_bytes[8] = {0};

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_base64_encode(payload, sizeof(payload), b64, sizeof(b64)));
    TEST_ASSERT_EQUAL_STRING("WGluWWk=", b64);

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_hex_encode(payload, sizeof(payload), hex, sizeof(hex)));
    TEST_ASSERT_EQUAL_STRING("58696e5969", hex);

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_sha256_hash((const uint8_t *)"abc", 3U, digest));
    TEST_ASSERT_EQUAL_MEMORY(abc_sha256, digest, sizeof(abc_sha256));

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_random_bytes(random_bytes, sizeof(random_bytes)));
    TEST_ASSERT_TRUE(any_nonzero(random_bytes, sizeof(random_bytes)));
}

static void test_decode_roundtrip_and_buffer_guards(void)
{
    uint8_t decoded[8] = {0};
    char tiny_output[4] = {0};

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_BUFFER_TOO_SMALL,
                          xy_base64_encode((const uint8_t *)"XinYi", 5U, tiny_output,
                                           sizeof(tiny_output)));
    TEST_ASSERT_EQUAL_STRING("", tiny_output);

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_base64_decode("WGluWWk=", strlen("WGluWWk="), decoded,
                                           sizeof(decoded)));
    TEST_ASSERT_EQUAL_MEMORY("XinYi", decoded, 5U);

    memset(decoded, 0, sizeof(decoded));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_hex_decode("58696E5969", strlen("58696E5969"), decoded,
                                        sizeof(decoded)));
    TEST_ASSERT_EQUAL_MEMORY("XinYi", decoded, 5U);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_encode_hash_rng_public_flow);
    RUN_TEST(test_decode_roundtrip_and_buffer_guards);
    return UNITY_END();
}
