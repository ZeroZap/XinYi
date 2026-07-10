/**
 * @file test_cipher_hmac.c
 * @brief Unit tests for remaining Crypto cipher and HMAC helpers.
 */

#include "xy_chacha20_poly1305.h"
#include "xy_sm3.h"
#include "xy_sm4.h"
#include "xy_tiny_crypto.h"
#include "unity.h"

#include <stdint.h>
#include <string.h>

static const uint8_t aes128_key[XY_AES_KEY_SIZE_128] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
};

static const uint8_t aes_plain[XY_AES_BLOCK_SIZE] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
};

static const uint8_t aes_cipher[XY_AES_BLOCK_SIZE] = {
    0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
    0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a,
};

static const uint8_t hmac_md5_expected[XY_MD5_DIGEST_SIZE] = {
    0x80, 0x07, 0x07, 0x13, 0x46, 0x3e, 0x77, 0x49,
    0xb9, 0x0c, 0x2d, 0xc2, 0x49, 0x11, 0xe2, 0x75,
};

static const uint8_t hmac_sha256_expected[XY_SHA256_DIGEST_SIZE] = {
    0xf7, 0xbc, 0x83, 0xf4, 0x30, 0x53, 0x84, 0x24,
    0xb1, 0x32, 0x98, 0xe6, 0xaa, 0x6f, 0xb1, 0x43,
    0xef, 0x4d, 0x59, 0xa1, 0x49, 0x46, 0x17, 0x59,
    0x97, 0x47, 0x9d, 0xbc, 0x2d, 0x1a, 0x3c, 0xd8,
};

static const uint8_t chacha_key[XY_CHACHA20_KEY_SIZE] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};

static const uint8_t chacha_nonce[XY_CHACHA20_NONCE_SIZE] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x4a, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t chacha_plain[] = {
    'L', 'a', 'd', 'i', 'e', 's', ' ', 'a', 'n', 'd', ' ', 'G', 'e', 'n', 't', 'l',
    'e', 'm', 'e', 'n', ' ', 'o', 'f', ' ', 't', 'h', 'e', ' ', 'c', 'l', 'a', 's',
    's', ' ', 'o', 'f', ' ', '\'', '9', '9', ':', ' ', 'I', 'f', ' ', 'I', ' ', 'c',
    'o', 'u', 'l', 'd', ' ', 'o', 'f', 'f', 'e', 'r', ' ', 'y', 'o', 'u', ' ', 'o',
    'n', 'l', 'y', ' ', 'o', 'n', 'e', ' ', 't', 'i', 'p', ' ', 'f', 'o', 'r', ' ',
    't', 'h', 'e', ' ', 'f', 'u', 't', 'u', 'r', 'e', ',', ' ', 's', 'u', 'n', 's',
    'c', 'r', 'e', 'e', 'n', ' ', 'w', 'o', 'u', 'l', 'd', ' ', 'b', 'e', ' ', 'i',
    't', '.',
};

static const uint8_t chacha_cipher[] = {
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
    0x87, 0x4d,
};

static const uint8_t sm3_abc[XY_SM3_DIGEST_SIZE] = {
    0x66, 0xc7, 0xf0, 0xf4, 0x62, 0xee, 0xed, 0xd9,
    0xd1, 0xf2, 0xd4, 0x6b, 0xdc, 0x10, 0xe4, 0xe2,
    0x41, 0x67, 0xc4, 0x87, 0x5c, 0xf2, 0xf7, 0xa2,
    0x29, 0x7d, 0xa0, 0x2b, 0x8f, 0x4b, 0xa8, 0xe0,
};

static void test_aes_vectors(void)
{
    xy_aes_ctx_t ctx;
    uint8_t output[XY_AES_BLOCK_SIZE];
    uint8_t roundtrip[XY_AES_BLOCK_SIZE];
    uint8_t iv[XY_AES_BLOCK_SIZE] = { 0 };
    uint8_t iv_dec[XY_AES_BLOCK_SIZE] = { 0 };
    uint8_t cbc_plain[XY_AES_BLOCK_SIZE * 2];
    uint8_t cbc_cipher[XY_AES_BLOCK_SIZE * 2];
    uint8_t cbc_roundtrip[XY_AES_BLOCK_SIZE * 2];

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_aes_init(NULL, aes128_key, XY_AES_KEY_SIZE_128));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_aes_init(&ctx, NULL, XY_AES_KEY_SIZE_128));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_aes_init(&ctx, aes128_key, 15));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_aes_init(&ctx, aes128_key, XY_AES_KEY_SIZE_128));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_aes_encrypt_block(NULL, aes_plain, output));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_aes_encrypt_block(&ctx, NULL, output));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_aes_encrypt_block(&ctx, aes_plain, NULL));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_aes_decrypt_block(&ctx, NULL, roundtrip));

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_aes_encrypt_block(&ctx, aes_plain, output));
    TEST_ASSERT_EQUAL_MEMORY(aes_cipher, output, sizeof(aes_cipher));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_aes_decrypt_block(&ctx, output, roundtrip));
    TEST_ASSERT_EQUAL_MEMORY(aes_plain, roundtrip, sizeof(aes_plain));

    memcpy(cbc_plain, aes_plain, XY_AES_BLOCK_SIZE);
    memcpy(&cbc_plain[XY_AES_BLOCK_SIZE], aes_cipher, XY_AES_BLOCK_SIZE);
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_aes_cbc_encrypt(&ctx, iv, cbc_plain, sizeof(cbc_plain), cbc_cipher));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_aes_cbc_decrypt(&ctx, iv_dec, cbc_cipher, sizeof(cbc_cipher), cbc_roundtrip));
    TEST_ASSERT_EQUAL_MEMORY(cbc_plain, cbc_roundtrip, sizeof(cbc_plain));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_aes_cbc_encrypt(&ctx, iv, cbc_plain, sizeof(cbc_plain) - 1, cbc_cipher));
}

static void test_hmac_vectors(void)
{
    const uint8_t key[] = "key";
    const uint8_t msg[] = "The quick brown fox jumps over the lazy dog";
    uint8_t digest[XY_SHA256_DIGEST_SIZE];

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_hmac_md5(NULL, sizeof(key) - 1, msg, sizeof(msg) - 1, digest));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_hmac_md5(key, sizeof(key) - 1, NULL, sizeof(msg) - 1, digest));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_hmac_md5(key, sizeof(key) - 1, msg, sizeof(msg) - 1, NULL));

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_hmac_md5(key, sizeof(key) - 1, msg, sizeof(msg) - 1, digest));
    TEST_ASSERT_EQUAL_MEMORY(hmac_md5_expected, digest, XY_MD5_DIGEST_SIZE);
    memset(digest, 0, sizeof(digest));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_hmac_sha256(key, sizeof(key) - 1, msg, sizeof(msg) - 1, digest));
    TEST_ASSERT_EQUAL_MEMORY(hmac_sha256_expected, digest, XY_SHA256_DIGEST_SIZE);
}

static void test_sm3_api_shape(void)
{
    xy_sm3_ctx_t ctx;
    uint8_t digest[XY_SM3_DIGEST_SIZE];
    uint8_t one_shot[XY_SM3_DIGEST_SIZE];
    const uint8_t msg[] = "abc";

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm3_init(NULL));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm3_hash(NULL, 3, digest));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm3_hash(msg, sizeof(msg) - 1, NULL));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_sm3_hash(msg, sizeof(msg) - 1, one_shot));
    TEST_ASSERT_EQUAL_MEMORY(sm3_abc, one_shot, XY_SM3_DIGEST_SIZE);

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_sm3_init(&ctx));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_sm3_update(&ctx, msg, 1));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_sm3_update(&ctx, &msg[1], 2));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_sm3_final(&ctx, digest));
    TEST_ASSERT_EQUAL_MEMORY(one_shot, digest, XY_SM3_DIGEST_SIZE);
}

static void test_sm4_api_shape(void)
{
    xy_sm4_ctx_t ctx;
    uint8_t key[XY_SM4_KEY_SIZE] = { 0 };
    uint8_t plain[XY_SM4_BLOCK_SIZE * 2];
    uint8_t cipher[XY_SM4_BLOCK_SIZE * 2];
    uint8_t iv[XY_SM4_BLOCK_SIZE] = { 0 };

    for (size_t i = 0; i < sizeof(plain); i++) {
        plain[i] = (uint8_t)i;
    }

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm4_init(NULL, key));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm4_init(&ctx, NULL));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_sm4_init(&ctx, key));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm4_encrypt_block(NULL, plain, cipher));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm4_encrypt_block(&ctx, NULL, cipher));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm4_encrypt_block(&ctx, plain, NULL));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm4_decrypt_block(&ctx, NULL, plain));

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_sm4_encrypt_block(&ctx, plain, cipher));
    TEST_ASSERT_NOT_EQUAL(0, memcmp(cipher, plain, XY_SM4_BLOCK_SIZE));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_sm4_cbc_encrypt(&ctx, iv, plain, sizeof(plain), cipher));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm4_cbc_encrypt(&ctx, iv, plain, sizeof(plain) - 1, cipher));
}

static void test_chacha20_vectors(void)
{
    xy_chacha20_ctx_t ctx;
    xy_chacha20_ctx_t dec;
    uint8_t output[sizeof(chacha_plain)];
    uint8_t roundtrip[sizeof(chacha_plain)];

    TEST_ASSERT_EQUAL_INT(XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM, xy_chacha20_init(NULL, chacha_key, chacha_nonce, 1));
    TEST_ASSERT_EQUAL_INT(XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM, xy_chacha20_init(&ctx, NULL, chacha_nonce, 1));
    TEST_ASSERT_EQUAL_INT(XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM, xy_chacha20_init(&ctx, chacha_key, NULL, 1));
    TEST_ASSERT_EQUAL_INT(XY_CHACHA20_POLY1305_SUCCESS, xy_chacha20_init(&ctx, chacha_key, chacha_nonce, 1));
    TEST_ASSERT_EQUAL_INT(XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM, xy_chacha20_crypt(NULL, output, chacha_plain, sizeof(chacha_plain)));
    TEST_ASSERT_EQUAL_INT(XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM, xy_chacha20_crypt(&ctx, NULL, chacha_plain, sizeof(chacha_plain)));
    TEST_ASSERT_EQUAL_INT(XY_CHACHA20_POLY1305_ERROR_INVALID_PARAM, xy_chacha20_crypt(&ctx, output, NULL, sizeof(chacha_plain)));

    TEST_ASSERT_EQUAL_INT(XY_CHACHA20_POLY1305_SUCCESS, xy_chacha20_crypt(&ctx, output, chacha_plain, sizeof(chacha_plain)));
    TEST_ASSERT_EQUAL_MEMORY(chacha_cipher, output, sizeof(chacha_cipher));
    TEST_ASSERT_EQUAL_INT(XY_CHACHA20_POLY1305_SUCCESS, xy_chacha20_init(&dec, chacha_key, chacha_nonce, 1));
    TEST_ASSERT_EQUAL_INT(XY_CHACHA20_POLY1305_SUCCESS, xy_chacha20_crypt(&dec, roundtrip, output, sizeof(output)));
    TEST_ASSERT_EQUAL_MEMORY(chacha_plain, roundtrip, sizeof(chacha_plain));
}

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_aes_vectors);
    RUN_TEST(test_hmac_vectors);
    RUN_TEST(test_sm3_api_shape);
    RUN_TEST(test_sm4_api_shape);
    RUN_TEST(test_chacha20_vectors);
    return UNITY_END();
}
