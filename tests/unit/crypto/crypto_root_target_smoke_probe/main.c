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
#include "xy_blake2.h"
#include "xy_ecdsa.h"

#include <stdint.h>
#include <string.h>

static const uint8_t abc_sha256[XY_SHA256_DIGEST_SIZE] = {
    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
    0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
    0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
};

static const uint8_t abc_blake2s[XY_BLAKE2S_OUTBYTES] = {
    0x50, 0x8c, 0x5e, 0x8c, 0x32, 0x7c, 0x14, 0xe2,
    0xe1, 0xa7, 0x2b, 0xa3, 0x4e, 0xeb, 0x45, 0x2f,
    0x37, 0x45, 0x8b, 0x20, 0x9e, 0xd6, 0x3a, 0x29,
    0x4d, 0x99, 0x9b, 0x4c, 0x86, 0x67, 0x59, 0x82,
};

static const uint8_t quick_brown_hmac_sha256[XY_SHA256_DIGEST_SIZE] = {
    0xf7, 0xbc, 0x83, 0xf4, 0x30, 0x53, 0x84, 0x24,
    0xb1, 0x32, 0x98, 0xe6, 0xaa, 0x6f, 0xb1, 0x43,
    0xef, 0x4d, 0x59, 0xa1, 0x49, 0x46, 0x17, 0x59,
    0x97, 0x47, 0x9d, 0xbc, 0x2d, 0x1a, 0x3c, 0xd8,
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

static void fill_u8(uint8_t *buffer, size_t len, uint8_t value)
{
    size_t i;

    for (i = 0; i < len; ++i) {
        buffer[i] = value;
    }
}

static int exercise_ecdsa_format_only_contract(void)
{
    const uint8_t message[] = {'r', 'o', 'o', 't', '-', 's', 'm', 'o', 'k', 'e'};
    const uint8_t empty_message[] = {0};
    xy_ecdsa_pub_key_t pub_key;
    xy_ecdsa_sig_t sig;
    uint8_t pub_key_bytes[XY_ECDSA_P256_PUB_KEY_SIZE];
    uint8_t sig_bytes[XY_ECDSA_P256_SIG_SIZE];

    memset(&pub_key, 0, sizeof(pub_key));
    memset(&sig, 0, sizeof(sig));
    fill_u8(sig.r, sizeof(sig.r), 0x01U);
    fill_u8(sig.s, sizeof(sig.s), 0x02U);

    if (require_int_equal(
            -1,
            xy_ecdsa_p256_verify((const xy_ecdsa_pub_key_t *)0, message, sizeof(message),
                                  &sig))) {
        return 1;
    }
    if (require_int_equal(
            -1,
            xy_ecdsa_p256_verify(&pub_key, (const uint8_t *)0, sizeof(message), &sig))) {
        return 2;
    }
    if (require_int_equal(
            -1,
            xy_ecdsa_p256_verify(&pub_key, message, sizeof(message),
                                  (const xy_ecdsa_sig_t *)0))) {
        return 3;
    }
    if (require_int_equal(-1, xy_ecdsa_p256_verify(&pub_key, message, sizeof(message), &sig))) {
        return 4;
    }

    fill_u8(pub_key.x, sizeof(pub_key.x), 0x01U);
    fill_u8(pub_key.y, sizeof(pub_key.y), 0x02U);
    memset(&sig, 0, sizeof(sig));

    if (require_int_equal(-1, xy_ecdsa_p256_verify(&pub_key, message, sizeof(message), &sig))) {
        return 5;
    }

    fill_u8(sig.r, sizeof(sig.r), 0x01U);
    if (require_int_equal(-1, xy_ecdsa_p256_verify(&pub_key, message, sizeof(message), &sig))) {
        return 6;
    }

    fill_u8(sig.s, sizeof(sig.s), 0x02U);

    /* Current root ECDSA is format-only: valid-looking values return success without
     * real elliptic-curve signature verification. This is a build/API guard only. */
    if (require_int_equal(
            0,
            xy_ecdsa_p256_verify(&pub_key, empty_message, 0U, &sig))) {
        return 7;
    }

    memset(pub_key_bytes, 0, sizeof(pub_key_bytes));
    memset(sig_bytes, 0, sizeof(sig_bytes));
    fill_u8(pub_key_bytes, sizeof(pub_key_bytes), 0x03U);
    fill_u8(sig_bytes, sizeof(sig_bytes), 0x04U);
    if (require_int_equal(0, xy_ecdsa_verify_simple(pub_key_bytes, empty_message, 0U,
                                                    sig_bytes))) {
        return 8;
    }

    return 0;
}

int main(void)
{
    const uint8_t payload[] = {'X', 'i', 'n', 'Y', 'i'};
    const uint8_t hmac_key[] = {'k', 'e', 'y'};
    const uint8_t hmac_msg[] = {
        'T', 'h', 'e', ' ', 'q', 'u', 'i', 'c', 'k', ' ', 'b', 'r', 'o', 'w', 'n', ' ',
        'f', 'o', 'x', ' ', 'j', 'u', 'm', 'p', 's', ' ', 'o', 'v', 'e', 'r', ' ', 't',
        'h', 'e', ' ', 'l', 'a', 'z', 'y', ' ', 'd', 'o', 'g',
    };
    char b64[16] = {0};
    char hex[16] = {0};
    uint8_t decoded[8] = {0};
    uint8_t digest[XY_SHA256_DIGEST_SIZE] = {0};
    uint8_t blake2s_digest[XY_BLAKE2S_OUTBYTES] = {0};

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

    memset(digest, 0, sizeof(digest));
    if (require_int_equal(XY_CRYPTO_SUCCESS,
                          xy_hmac_sha256(hmac_key, sizeof(hmac_key), hmac_msg,
                                          sizeof(hmac_msg), digest))) {
        return 11;
    }
    if (require_memory_equal(quick_brown_hmac_sha256, digest,
                             sizeof(quick_brown_hmac_sha256))) {
        return 12;
    }

    if (require_int_equal(XY_BLAKE2_SUCCESS,
                          xy_blake2s(blake2s_digest, sizeof(blake2s_digest),
                                      (const uint8_t *)"abc", 3U, NULL, 0U))) {
        return 13;
    }
    if (require_memory_equal(abc_blake2s, blake2s_digest, sizeof(abc_blake2s))) {
        return 14;
    }

    if (exercise_ecdsa_format_only_contract()) {
        return 15;
    }

    return 0;
}
