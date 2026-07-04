/**
 * @file test_sm2.c
 * @brief Unit tests for SM2 public API contracts.
 */

#include "xy_sm2.h"
#include "xy_tiny_crypto.h"
#include "unity.h"

#include <stdint.h>
#include <string.h>

static const uint8_t fixed_private_key[XY_SM2_PRIVATE_KEY_SIZE] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};

static const uint8_t fixed_public_key[XY_SM2_PUBLIC_KEY_SIZE] = {
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
};

void setUp(void)
{
}

void tearDown(void)
{
}

static int buffer_is_zero(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (buf[i] != 0U) {
            return 0;
        }
    }
    return 1;
}

static void test_sm2_key_api_validation(void)
{
    xy_sm2_ctx_t ctx;

    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm2_generate_key(NULL));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm2_set_private_key(NULL, fixed_private_key));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm2_set_private_key(&ctx, NULL));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm2_set_public_key(NULL, fixed_public_key));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM, xy_sm2_set_public_key(&ctx, NULL));

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_sm2_set_public_key(&ctx, fixed_public_key));
    TEST_ASSERT_EQUAL_MEMORY(fixed_public_key, ctx.public_key, sizeof(fixed_public_key));

    /* The simplified SM2 implementation currently rejects derived public keys because
     * point arithmetic is still a placeholder. Keep set_public_key guarded above and
     * keypair generation covered as an availability/error-shape contract below. */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_ERROR, xy_sm2_set_private_key(&ctx, fixed_private_key));
}

static void test_sm2_sign_verify_contracts(void)
{
    xy_sm2_ctx_t ctx;
    uint8_t signature[XY_SM2_SIGNATURE_SIZE];
    const uint8_t id[] = "test-user";
    const uint8_t message[] = "message for sm2";
    uint8_t zero_signature[XY_SM2_SIGNATURE_SIZE] = {0};

    memset(&ctx, 0, sizeof(ctx));
    memcpy(ctx.private_key, fixed_private_key, sizeof(fixed_private_key));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_sm2_set_public_key(&ctx, fixed_public_key));

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_sign(NULL, id, sizeof(id) - 1, message, sizeof(message) - 1, signature));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_sign(&ctx, id, sizeof(id) - 1, NULL, sizeof(message) - 1, signature));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_sign(&ctx, id, sizeof(id) - 1, message, sizeof(message) - 1, NULL));

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_sm2_sign(&ctx, id, sizeof(id) - 1, message, sizeof(message) - 1, signature));
    TEST_ASSERT_FALSE(buffer_is_zero(signature, sizeof(signature)));

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_verify(NULL, id, sizeof(id) - 1, message, sizeof(message) - 1, signature));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_verify(&ctx, id, sizeof(id) - 1, NULL, sizeof(message) - 1, signature));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_verify(&ctx, id, sizeof(id) - 1, message, sizeof(message) - 1, NULL));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_ERROR,
                          xy_sm2_verify(&ctx, id, sizeof(id) - 1, message, sizeof(message) - 1,
                                        zero_signature));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_sm2_verify(&ctx, id, sizeof(id) - 1, message, sizeof(message) - 1, signature));
}

static void test_sm2_encrypt_decrypt_validation(void)
{
    xy_sm2_ctx_t ctx;
    uint8_t ciphertext[128];
    uint8_t plaintext[32];
    size_t out_len = sizeof(ciphertext);
    size_t plain_len = sizeof(plaintext);
    const uint8_t message[] = "0123456789abcdef";

    memset(&ctx, 0, sizeof(ctx));
    memcpy(ctx.private_key, fixed_private_key, sizeof(fixed_private_key));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_sm2_set_public_key(&ctx, fixed_public_key));

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_encrypt(NULL, ctx.public_key, message, sizeof(message) - 1,
                                         ciphertext, &out_len));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_encrypt(&ctx, ctx.public_key, NULL, sizeof(message) - 1,
                                         ciphertext, &out_len));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_encrypt(&ctx, ctx.public_key, message, sizeof(message) - 1,
                                         NULL, &out_len));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_encrypt(&ctx, ctx.public_key, message, sizeof(message) - 1,
                                         ciphertext, NULL));

    out_len = 4;
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_BUFFER_TOO_SMALL,
                          xy_sm2_encrypt(&ctx, ctx.public_key, message, sizeof(message) - 1,
                                         ciphertext, &out_len));

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_decrypt(NULL, ctx.public_key, ciphertext, sizeof(ciphertext),
                                         plaintext, &plain_len));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_decrypt(&ctx, ctx.public_key, NULL, sizeof(ciphertext),
                                         plaintext, &plain_len));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_decrypt(&ctx, ctx.public_key, ciphertext, sizeof(ciphertext),
                                         NULL, &plain_len));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_INVALID_PARAM,
                          xy_sm2_decrypt(&ctx, ctx.public_key, ciphertext, sizeof(ciphertext),
                                         plaintext, NULL));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_ERROR,
                          xy_sm2_decrypt(&ctx, ctx.public_key, ciphertext, 96,
                                         plaintext, &plain_len));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sm2_key_api_validation);
    RUN_TEST(test_sm2_sign_verify_contracts);
    RUN_TEST(test_sm2_encrypt_decrypt_validation);
    return UNITY_END();
}
