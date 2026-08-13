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
#include "xy_chacha20poly1305.h"
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

static int require_buffer_filled(const uint8_t *buffer, size_t len, uint8_t expected)
{
    size_t i;

    for (i = 0; i < len; ++i) {
        if (buffer[i] != expected) {
            return 1;
        }
    }

    return 0;
}

static void fill_u8(uint8_t *buffer, size_t len, uint8_t value)
{
    size_t i;

    for (i = 0; i < len; ++i) {
        buffer[i] = value;
    }
}

static int exercise_root_chacha20poly1305_contract(void)
{
    static const uint8_t key[32] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    };
    static const uint8_t nonce[12] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x4a, 0x00, 0x00, 0x00, 0x00,
    };
    static const uint8_t aad[] = {
        0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7,
    };
    static const uint8_t plaintext[] = {
        'L', 'a', 'd', 'i', 'e', 's', ' ', 'a', 'n', 'd', ' ', 'G', 'e', 'n', 't', 'l',
        'e', 'm', 'e', 'n', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 'c', 'l', 'a', 's',
        's', ' ', 'o', 'f', ' ', '\'', '9', '9', ':', ' ', 'I', 'f', ' ', 'I', ' ', 'c',
        'o', 'u', 'l', 'd', ' ', 'o', 'f', 'f', 'e', 'r', ' ', 'y', 'o', 'u', ' ', 'o',
        'n', 'l', 'y', ' ', 'o', 'n', 'e', ' ', 't', 'i', 'p', ' ', 'f', 'o', 'r', ' ',
        't', 'h', 'e', ' ', 'f', 'u', 't', 'u', 'r', 'e', ',', ' ', 's', 'u', 'n', 's',
        'c', 'r', 'e', 'e', 'n', ' ', 'w', 'o', 'u', 'l', 'd', ' ', 'b', 'e', ' ', 'i',
        't', '.',
    };
    static const uint8_t expected_cipher_with_tag[] = {
        0x6e, 0x2e, 0x35, 0x9a, 0x25, 0x68, 0xf9, 0x80,
        0x41, 0xba, 0x07, 0x28, 0xdd, 0x0d, 0x69, 0x81,
        0xe9, 0x7e, 0x7a, 0xec, 0x1d, 0x43, 0x60, 0xc2,
        0x0a, 0x27, 0xaf, 0xcc, 0xfd, 0x9f, 0xae, 0x0b,
        0xf9, 0x1b, 0x65, 0xc5, 0x52, 0x47, 0x33, 0xab,
        0x8f, 0x59, 0x3d, 0xab, 0xcd, 0x62, 0xb3, 0x57,
        0x16, 0x39, 0xd6, 0x24, 0xe6, 0x51, 0x52, 0xab,
        0x8f, 0x53, 0x0c, 0x35, 0x9f, 0x08, 0x61, 0xd8,
        0x07, 0xca, 0x0d, 0xbf, 0x50, 0x0d, 0x6a, 0x61,
        0x56, 0xa3, 0x8e, 0x08, 0x8a, 0x22, 0xb6, 0x5e,
        0x52, 0xbc, 0x51, 0x4d, 0x16, 0xcc, 0xf8, 0x06,
        0x81, 0x8c, 0xe9, 0x1a, 0xb7, 0x79, 0x37, 0x36,
        0x5a, 0xf9, 0x0b, 0xbf, 0x74, 0xa3, 0x5b, 0xe6,
        0xb4, 0x0b, 0x8e, 0xed, 0xf2, 0x78, 0x5e, 0x42,
        0x87, 0x4d, 0x31, 0x79, 0x26, 0x7b, 0x0b, 0xa7,
        0x1e, 0x40, 0xa2, 0xad, 0x86, 0x6f, 0xce, 0x5f,
        0x80, 0x52,
    };
    uint8_t ciphertext[sizeof(expected_cipher_with_tag)];
    uint8_t plaintext_out[sizeof(plaintext)];
    size_t ciphertext_len;
    size_t plaintext_len;

    fill_u8(ciphertext, sizeof(ciphertext), 0xa5U);
    ciphertext_len = 0U;
    if (require_int_equal(-1, xy_chacha20poly1305_encrypt(NULL, nonce, aad, sizeof(aad),
                                                          plaintext, sizeof(plaintext),
                                                          ciphertext, &ciphertext_len))) {
        return 1;
    }
    if (require_int_equal(0U, (int)ciphertext_len)) {
        return 2;
    }
    if (require_buffer_filled(ciphertext, sizeof(ciphertext), 0xa5U)) {
        return 3;
    }

    ciphertext_len = sizeof(ciphertext);
    if (require_int_equal(0, xy_chacha20poly1305_encrypt(key, nonce, aad, sizeof(aad),
                                                         plaintext, sizeof(plaintext),
                                                         ciphertext, &ciphertext_len))) {
        return 4;
    }
    if (require_int_equal((int)sizeof(expected_cipher_with_tag), (int)ciphertext_len)) {
        return 5;
    }
    if (require_memory_equal(expected_cipher_with_tag, ciphertext,
                             sizeof(expected_cipher_with_tag))) {
        return 6;
    }

    fill_u8(plaintext_out, sizeof(plaintext_out), 0x5aU);
    plaintext_len = 0U;
    if (require_int_equal(0, xy_chacha20poly1305_decrypt(key, nonce, aad, sizeof(aad),
                                                         ciphertext, ciphertext_len,
                                                         plaintext_out, &plaintext_len))) {
        return 7;
    }
    if (require_int_equal((int)sizeof(plaintext), (int)plaintext_len)) {
        return 8;
    }
    if (require_memory_equal(plaintext, plaintext_out, sizeof(plaintext))) {
        return 9;
    }

    ciphertext[sizeof(ciphertext) - 1U] ^= 0x01U;
    fill_u8(plaintext_out, sizeof(plaintext_out), 0x5aU);
    plaintext_len = sizeof(plaintext_out);
    if (require_int_equal(-1, xy_chacha20poly1305_decrypt(key, nonce, aad, sizeof(aad),
                                                          ciphertext, ciphertext_len,
                                                          plaintext_out, &plaintext_len))) {
        return 10;
    }
    if (require_int_equal((int)sizeof(plaintext_out), (int)plaintext_len)) {
        return 11;
    }
    if (require_buffer_filled(plaintext_out, sizeof(plaintext_out), 0x5aU)) {
        return 12;
    }

    return 0;
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

    if (exercise_root_chacha20poly1305_contract()) {
        return 15;
    }

    if (exercise_ecdsa_format_only_contract()) {
        return 16;
    }

    return 0;
}
