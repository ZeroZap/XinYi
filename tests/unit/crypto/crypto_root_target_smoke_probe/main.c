/**
 * @file main.c
 * @brief Root xy_tiny_crypto target smoke consumer.
 *
 * This probe links the public umbrella header against the real root/runtime
 * xy_tiny_crypto static library. It is an API/build ownership guard only; it
 * does not claim cryptographic security review, entropy-source quality,
 * hardware acceleration, or compliance validation.
 */

#include "xy_tiny_crypto.h"

#include <stdint.h>
#include <string.h>

static const uint8_t abc_sha256[XY_SHA256_DIGEST_SIZE] = {
    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
    0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
    0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
};

static int require_int_equal(int expected, int actual)
{
    return expected == actual ? 0 : 1;
}

static int require_memory_equal(const void *expected, const void *actual, size_t len)
{
    return memcmp(expected, actual, len) == 0 ? 0 : 1;
}

static int require_string_equal(const char *expected, const char *actual)
{
    return strcmp(expected, actual) == 0 ? 0 : 1;
}

int main(void)
{
    const uint8_t payload[] = {'X', 'i', 'n', 'Y', 'i'};
    char b64[16] = {0};
    char hex[16] = {0};
    uint8_t decoded[8] = {0};
    uint8_t digest[XY_SHA256_DIGEST_SIZE] = {0};

    if (require_int_equal(XY_CRYPTO_SUCCESS,
                          xy_base64_encode(payload, sizeof(payload), b64, sizeof(b64)))) {
        return 1;
    }
    if (require_string_equal("WGluWWk=", b64)) {
        return 2;
    }
    if (require_int_equal(XY_CRYPTO_SUCCESS,
                          xy_base64_decode(b64, strlen(b64), decoded, sizeof(decoded)))) {
        return 3;
    }
    if (require_memory_equal(payload, decoded, sizeof(payload))) {
        return 4;
    }

    if (require_int_equal(XY_CRYPTO_SUCCESS,
                          xy_hex_encode(payload, sizeof(payload), hex, sizeof(hex)))) {
        return 5;
    }
    if (require_string_equal("58696e5969", hex)) {
        return 6;
    }
    memset(decoded, 0, sizeof(decoded));
    if (require_int_equal(XY_CRYPTO_SUCCESS,
                          xy_hex_decode(hex, strlen(hex), decoded, sizeof(decoded)))) {
        return 7;
    }
    if (require_memory_equal(payload, decoded, sizeof(payload))) {
        return 8;
    }

    if (require_int_equal(XY_CRYPTO_SUCCESS, xy_sha256_hash((const uint8_t *)"abc", 3U, digest))) {
        return 9;
    }
    if (require_memory_equal(abc_sha256, digest, sizeof(abc_sha256))) {
        return 10;
    }

    return 0;
}
