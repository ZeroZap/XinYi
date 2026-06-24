/**
 * @file test_encode.c
 * @brief Unit tests for Crypto Base64 and Hex helpers.
 */

#include "xy_tiny_crypto.h"
#include "unity.h"

#include <stdint.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_hex_lengths_and_validation(void)
{
    uint8_t decoded[8] = {0};
    char encoded[16] = {0};
    const uint8_t input[] = {0x00, 0x01, 0x2a, 0xab, 0xff};

    TEST_ASSERT_TRUE(xy_hex_encode_len(0) == 1);
    TEST_ASSERT_TRUE(xy_hex_encode_len(sizeof(input)) == 11);
    TEST_ASSERT_TRUE(xy_hex_decode_len(10) == 5);

    TEST_ASSERT_TRUE(xy_hex_encode(NULL, sizeof(input), encoded, sizeof(encoded)) ==
                     XY_CRYPTO_INVALID_PARAM);
    TEST_ASSERT_TRUE(xy_hex_encode(input, sizeof(input), NULL, sizeof(encoded)) ==
                     XY_CRYPTO_INVALID_PARAM);
    TEST_ASSERT_TRUE(xy_hex_encode(input, sizeof(input), encoded, 10) ==
                     XY_CRYPTO_BUFFER_TOO_SMALL);

    TEST_ASSERT_TRUE(xy_hex_decode(NULL, 2, decoded, sizeof(decoded)) ==
                     XY_CRYPTO_INVALID_PARAM);
    TEST_ASSERT_TRUE(xy_hex_decode("0", 1, decoded, sizeof(decoded)) ==
                     XY_CRYPTO_INVALID_PARAM);
    TEST_ASSERT_TRUE(xy_hex_decode("xx", 2, decoded, sizeof(decoded)) ==
                     XY_CRYPTO_INVALID_PARAM);
    TEST_ASSERT_TRUE(xy_hex_decode("0001", 4, decoded, 1) ==
                     XY_CRYPTO_BUFFER_TOO_SMALL);
}

static void test_hex_roundtrip(void)
{
    const uint8_t input[] = {0x00, 0x01, 0x2a, 0xab, 0xff};
    char encoded[16] = {0};
    uint8_t decoded[8] = {0};

    TEST_ASSERT_TRUE(xy_hex_encode(input, sizeof(input), encoded, sizeof(encoded)) ==
                     XY_CRYPTO_SUCCESS);
    TEST_ASSERT_TRUE(strcmp(encoded, "00012aabff") == 0);

    TEST_ASSERT_TRUE(xy_hex_decode(encoded, strlen(encoded), decoded, sizeof(decoded)) ==
                     XY_CRYPTO_SUCCESS);
    TEST_ASSERT_TRUE(memcmp(decoded, input, sizeof(input)) == 0);

    memset(decoded, 0, sizeof(decoded));
    TEST_ASSERT_TRUE(xy_hex_decode("00012AABFF", 10, decoded, sizeof(decoded)) ==
                     XY_CRYPTO_SUCCESS);
    TEST_ASSERT_TRUE(memcmp(decoded, input, sizeof(input)) == 0);
}

static void test_base64_lengths_and_validation(void)
{
    uint8_t decoded[8] = {0};
    char encoded[16] = {0};
    const uint8_t input[] = {'a', 'b', 'c'};

    TEST_ASSERT_TRUE(xy_base64_encode_len(0) == 1);
    TEST_ASSERT_TRUE(xy_base64_encode_len(1) == 5);
    TEST_ASSERT_TRUE(xy_base64_encode_len(2) == 5);
    TEST_ASSERT_TRUE(xy_base64_encode_len(3) == 5);
    TEST_ASSERT_TRUE(xy_base64_decode_len(4) == 3);

    TEST_ASSERT_TRUE(xy_base64_encode(NULL, sizeof(input), encoded, sizeof(encoded)) ==
                     XY_CRYPTO_INVALID_PARAM);
    TEST_ASSERT_TRUE(xy_base64_encode(input, sizeof(input), NULL, sizeof(encoded)) ==
                     XY_CRYPTO_INVALID_PARAM);
    TEST_ASSERT_TRUE(xy_base64_encode(input, sizeof(input), encoded, 4) ==
                     XY_CRYPTO_BUFFER_TOO_SMALL);

    TEST_ASSERT_TRUE(xy_base64_decode(NULL, 4, decoded, sizeof(decoded)) ==
                     XY_CRYPTO_INVALID_PARAM);
    TEST_ASSERT_TRUE(xy_base64_decode("abc", 3, decoded, sizeof(decoded)) ==
                     XY_CRYPTO_INVALID_PARAM);
    TEST_ASSERT_TRUE(xy_base64_decode("!!!!", 4, decoded, sizeof(decoded)) ==
                     XY_CRYPTO_INVALID_PARAM);
    TEST_ASSERT_TRUE(xy_base64_decode("QUJD", 4, decoded, 2) ==
                     XY_CRYPTO_BUFFER_TOO_SMALL);
}

static void test_base64_roundtrip_and_padding(void)
{
    char encoded[16] = {0};
    uint8_t decoded[8] = {0};
    size_t decoded_len;

    TEST_ASSERT_TRUE(xy_base64_encode((const uint8_t *)"", 0, encoded, sizeof(encoded)) ==
                     XY_CRYPTO_SUCCESS);
    TEST_ASSERT_TRUE(strcmp(encoded, "") == 0);

    TEST_ASSERT_TRUE(xy_base64_encode((const uint8_t *)"f", 1, encoded, sizeof(encoded)) ==
                     XY_CRYPTO_SUCCESS);
    TEST_ASSERT_TRUE(strcmp(encoded, "Zg==") == 0);
    TEST_ASSERT_TRUE(xy_base64_decode(encoded, strlen(encoded), decoded, sizeof(decoded)) ==
                     XY_CRYPTO_SUCCESS);
    TEST_ASSERT_TRUE(decoded[0] == 'f');

    memset(decoded, 0, sizeof(decoded));
    TEST_ASSERT_TRUE(xy_base64_encode((const uint8_t *)"fo", 2, encoded, sizeof(encoded)) ==
                     XY_CRYPTO_SUCCESS);
    TEST_ASSERT_TRUE(strcmp(encoded, "Zm8=") == 0);
    TEST_ASSERT_TRUE(xy_base64_decode(encoded, strlen(encoded), decoded, sizeof(decoded)) ==
                     XY_CRYPTO_SUCCESS);
    TEST_ASSERT_TRUE(memcmp(decoded, "fo", 2) == 0);

    memset(decoded, 0, sizeof(decoded));
    TEST_ASSERT_TRUE(xy_base64_encode((const uint8_t *)"foo", 3, encoded, sizeof(encoded)) ==
                     XY_CRYPTO_SUCCESS);
    TEST_ASSERT_TRUE(strcmp(encoded, "Zm9v") == 0);
    TEST_ASSERT_TRUE(xy_base64_decode(encoded, strlen(encoded), decoded, sizeof(decoded)) ==
                     XY_CRYPTO_SUCCESS);
    TEST_ASSERT_TRUE(memcmp(decoded, "foo", 3) == 0);

    decoded_len = xy_base64_decode_len(strlen("Zm9v"));
    TEST_ASSERT_TRUE(decoded_len == 3);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_hex_lengths_and_validation);
    RUN_TEST(test_hex_roundtrip);
    RUN_TEST(test_base64_lengths_and_validation);
    RUN_TEST(test_base64_roundtrip_and_padding);
    return UNITY_END();
}
