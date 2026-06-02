/**
 * @file test_encode.c
 * @brief Unit tests for Crypto Base64 and Hex helpers.
 */

#include "xy_tiny_crypto.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static void test_hex_lengths_and_validation(void)
{
    uint8_t decoded[8] = {0};
    char encoded[16] = {0};
    const uint8_t input[] = {0x00, 0x01, 0x2a, 0xab, 0xff};

    assert(xy_hex_encode_len(0) == 1);
    assert(xy_hex_encode_len(sizeof(input)) == 11);
    assert(xy_hex_decode_len(10) == 5);

    assert(xy_hex_encode(NULL, sizeof(input), encoded, sizeof(encoded)) ==
           XY_CRYPTO_INVALID_PARAM);
    assert(xy_hex_encode(input, sizeof(input), NULL, sizeof(encoded)) ==
           XY_CRYPTO_INVALID_PARAM);
    assert(xy_hex_encode(input, sizeof(input), encoded, 10) ==
           XY_CRYPTO_BUFFER_TOO_SMALL);

    assert(xy_hex_decode(NULL, 2, decoded, sizeof(decoded)) ==
           XY_CRYPTO_INVALID_PARAM);
    assert(xy_hex_decode("0", 1, decoded, sizeof(decoded)) ==
           XY_CRYPTO_INVALID_PARAM);
    assert(xy_hex_decode("xx", 2, decoded, sizeof(decoded)) ==
           XY_CRYPTO_INVALID_PARAM);
    assert(xy_hex_decode("0001", 4, decoded, 1) ==
           XY_CRYPTO_BUFFER_TOO_SMALL);
}

static void test_hex_roundtrip(void)
{
    const uint8_t input[] = {0x00, 0x01, 0x2a, 0xab, 0xff};
    char encoded[16] = {0};
    uint8_t decoded[8] = {0};

    assert(xy_hex_encode(input, sizeof(input), encoded, sizeof(encoded)) ==
           XY_CRYPTO_SUCCESS);
    assert(strcmp(encoded, "00012aabff") == 0);

    assert(xy_hex_decode(encoded, strlen(encoded), decoded, sizeof(decoded)) ==
           XY_CRYPTO_SUCCESS);
    assert(memcmp(decoded, input, sizeof(input)) == 0);

    memset(decoded, 0, sizeof(decoded));
    assert(xy_hex_decode("00012AABFF", 10, decoded, sizeof(decoded)) ==
           XY_CRYPTO_SUCCESS);
    assert(memcmp(decoded, input, sizeof(input)) == 0);
}

static void test_base64_lengths_and_validation(void)
{
    uint8_t decoded[8] = {0};
    char encoded[16] = {0};
    const uint8_t input[] = {'a', 'b', 'c'};

    assert(xy_base64_encode_len(0) == 1);
    assert(xy_base64_encode_len(1) == 5);
    assert(xy_base64_encode_len(2) == 5);
    assert(xy_base64_encode_len(3) == 5);
    assert(xy_base64_decode_len(4) == 3);

    assert(xy_base64_encode(NULL, sizeof(input), encoded, sizeof(encoded)) ==
           XY_CRYPTO_INVALID_PARAM);
    assert(xy_base64_encode(input, sizeof(input), NULL, sizeof(encoded)) ==
           XY_CRYPTO_INVALID_PARAM);
    assert(xy_base64_encode(input, sizeof(input), encoded, 4) ==
           XY_CRYPTO_BUFFER_TOO_SMALL);

    assert(xy_base64_decode(NULL, 4, decoded, sizeof(decoded)) ==
           XY_CRYPTO_INVALID_PARAM);
    assert(xy_base64_decode("abc", 3, decoded, sizeof(decoded)) ==
           XY_CRYPTO_INVALID_PARAM);
    assert(xy_base64_decode("!!!!", 4, decoded, sizeof(decoded)) ==
           XY_CRYPTO_INVALID_PARAM);
    assert(xy_base64_decode("QUJD", 4, decoded, 2) ==
           XY_CRYPTO_BUFFER_TOO_SMALL);
}

static void test_base64_roundtrip_and_padding(void)
{
    char encoded[16] = {0};
    uint8_t decoded[8] = {0};
    size_t decoded_len;

    assert(xy_base64_encode((const uint8_t *)"", 0, encoded, sizeof(encoded)) ==
           XY_CRYPTO_SUCCESS);
    assert(strcmp(encoded, "") == 0);

    assert(xy_base64_encode((const uint8_t *)"f", 1, encoded, sizeof(encoded)) ==
           XY_CRYPTO_SUCCESS);
    assert(strcmp(encoded, "Zg==") == 0);
    assert(xy_base64_decode(encoded, strlen(encoded), decoded, sizeof(decoded)) ==
           XY_CRYPTO_SUCCESS);
    assert(decoded[0] == 'f');

    memset(decoded, 0, sizeof(decoded));
    assert(xy_base64_encode((const uint8_t *)"fo", 2, encoded, sizeof(encoded)) ==
           XY_CRYPTO_SUCCESS);
    assert(strcmp(encoded, "Zm8=") == 0);
    assert(xy_base64_decode(encoded, strlen(encoded), decoded, sizeof(decoded)) ==
           XY_CRYPTO_SUCCESS);
    assert(memcmp(decoded, "fo", 2) == 0);

    memset(decoded, 0, sizeof(decoded));
    assert(xy_base64_encode((const uint8_t *)"foo", 3, encoded, sizeof(encoded)) ==
           XY_CRYPTO_SUCCESS);
    assert(strcmp(encoded, "Zm9v") == 0);
    assert(xy_base64_decode(encoded, strlen(encoded), decoded, sizeof(decoded)) ==
           XY_CRYPTO_SUCCESS);
    assert(memcmp(decoded, "foo", 3) == 0);

    decoded_len = xy_base64_decode_len(strlen("Zm9v"));
    assert(decoded_len == 3);
}

int main(void)
{
    test_hex_lengths_and_validation();
    test_hex_roundtrip();
    test_base64_lengths_and_validation();
    test_base64_roundtrip_and_padding();
    return 0;
}
