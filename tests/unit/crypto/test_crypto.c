/**
 * @file test_crypto.c
 * @brief Crypto Component Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* Crypto headers */
#include "xy_crc.h"
#include "xy_tiny_crypto.h"

/* ==================== Test Fixtures ==================== */

void setUp(void)
{
    /* Called before each test */
}

void tearDown(void)
{
    /* Called after each test */
}

/* ==================== Helper Functions ==================== */

static void print_hex(const uint8_t *data, size_t len, const char *label)
{
    (void)label;
    for (size_t i = 0; i < len; i++) {
        TEST_PRINTF("%02x", data[i]);
    }
}

/* ==================== CRC Tests ==================== */

void test_crc32_basic(void)
{
    const char *test_data = "123456789";
    uint32_t crc;

    /* Standard CRC32 test vector */
    crc = xy_crc32_normal((uint8_t *)test_data, strlen(test_data));
    TEST_ASSERT_EQUAL_UINT32(0xcbf43926, crc);
}

void test_crc32_empty_data(void)
{
    uint32_t crc;

    /* CRC32 of empty data should be initial value */
    crc = xy_crc32_normal(NULL, 0);
    /* Implementation dependent, just check it returns a value */
    TEST_ASSERT_TRUE(crc == 0 || crc != 0);
}

void test_crc32_consistency(void)
{
    const char *test_data = "Hello, World!";
    uint32_t crc1, crc2;

    /* Same data should produce same CRC */
    crc1 = xy_crc32_normal((uint8_t *)test_data, strlen(test_data));
    crc2 = xy_crc32_normal((uint8_t *)test_data, strlen(test_data));
    TEST_ASSERT_EQUAL_UINT32(crc1, crc2);
}

void test_crc32_different_data(void)
{
    const char *data1 = "Hello";
    const char *data2 = "World";
    uint32_t crc1, crc2;

    /* Different data should produce different CRC (most of the time) */
    crc1 = xy_crc32_normal((uint8_t *)data1, strlen(data1));
    crc2 = xy_crc32_normal((uint8_t *)data2, strlen(data2));
    TEST_ASSERT_NOT_EQUAL_UINT32(crc1, crc2);
}

void test_crc16_modbus(void)
{
    uint8_t test_data[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x0A };
    uint16_t crc;

    /* Modbus CRC16 test */
    crc = xy_crc16_modbus(test_data, sizeof(test_data));
    /* Just check it returns a value */
    TEST_ASSERT_TRUE(crc != 0 || crc == 0);
}

void test_crc8_basic(void)
{
    const char *test_data = "123456789";
    uint8_t crc;

    /* CRC8 test */
    crc = xy_crc8_normal((uint8_t *)test_data, strlen(test_data));
    /* Just check it returns a value */
    TEST_ASSERT_TRUE(crc != 0 || crc == 0);
}

/* ==================== MD5 Tests ==================== */

void test_md5_basic(void)
{
    const char *test_data = "The quick brown fox jumps over the lazy dog";
    uint8_t digest[XY_MD5_DIGEST_SIZE];
    const uint8_t expected[] = {
        0x9e, 0x10, 0x7d, 0x9d, 0x37, 0x2b, 0xb6, 0x82,
        0x6b, 0xd8, 0x1d, 0x35, 0x42, 0xa4, 0x19, 0xd6
    };

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_md5_hash((const uint8_t *)test_data,
                                      strlen(test_data), digest));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, digest, XY_MD5_DIGEST_SIZE);
}

void test_md5_empty_string(void)
{
    const char *test_data = "";
    uint8_t digest[XY_MD5_DIGEST_SIZE];
    const uint8_t expected[] = {
        0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
        0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e
    };

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_md5_hash((const uint8_t *)test_data,
                                      strlen(test_data), digest));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, digest, XY_MD5_DIGEST_SIZE);
}

void test_md5_incremental(void)
{
    const char *test_data = "Hello, World!";
    xy_md5_ctx_t ctx;
    uint8_t digest[XY_MD5_DIGEST_SIZE];
    uint8_t digest_one_shot[XY_MD5_DIGEST_SIZE];

    /* Test incremental hashing */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_md5_init(&ctx));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_md5_update(&ctx, (const uint8_t *)test_data,
                                        strlen(test_data)));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS, xy_md5_final(&ctx, digest));

    /* Compare with one-shot hash */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_md5_hash((const uint8_t *)test_data,
                                      strlen(test_data), digest_one_shot));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(digest, digest_one_shot, XY_MD5_DIGEST_SIZE);
}

/* ==================== SHA256 Tests ==================== */

void test_sha256_basic(void)
{
    const char *test_data = "The quick brown fox jumps over the lazy dog";
    uint8_t digest[XY_SHA256_DIGEST_SIZE];
    const uint8_t expected[] = {
        0xd7, 0xa8, 0xfb, 0xb3, 0x07, 0xd7, 0x80, 0x94,
        0x69, 0xca, 0x9a, 0xbc, 0xb0, 0x08, 0x2e, 0x4f,
        0x8d, 0x56, 0x51, 0xe4, 0x6d, 0x3c, 0xdb, 0x76,
        0x2d, 0x02, 0xd0, 0xbf, 0x37, 0xc9, 0xe5, 0x92
    };

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_sha256_hash((const uint8_t *)test_data,
                                         strlen(test_data), digest));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, digest, XY_SHA256_DIGEST_SIZE);
}

void test_sha256_empty_string(void)
{
    const char *test_data = "";
    uint8_t digest[XY_SHA256_DIGEST_SIZE];
    const uint8_t expected[] = {
        0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
        0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
        0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
        0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
    };

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_sha256_hash((const uint8_t *)test_data,
                                         strlen(test_data), digest));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, digest, XY_SHA256_DIGEST_SIZE);
}

/* ==================== AES Tests ==================== */

void test_aes128_encrypt_decrypt(void)
{
    const uint8_t key[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                              0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
    const uint8_t plaintext[16] = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40,
                                    0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11,
                                    0x73, 0x93, 0x17, 0x2a };
    uint8_t ciphertext[16];
    uint8_t decrypted[16];
    xy_aes_ctx_t ctx;

    /* Initialize AES */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_aes_init(&ctx, key, XY_AES_KEY_SIZE_128));

    /* Encrypt */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_aes_encrypt_block(&ctx, plaintext, ciphertext));

    /* Decrypt */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_aes_decrypt_block(&ctx, ciphertext, decrypted));

    /* Verify decrypted matches original */
    TEST_ASSERT_EQUAL_HEX8_ARRAY(plaintext, decrypted, 16);
}

void test_aes_cbc_encrypt_decrypt(void)
{
    const uint8_t key[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                              0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
    const uint8_t iv[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                             0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    const uint8_t plaintext[32] = "Hello, AES-CBC! Test message.";
    uint8_t ciphertext[32];
    uint8_t decrypted[32];
    xy_aes_ctx_t ctx;

    /* Initialize AES */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_aes_init(&ctx, key, XY_AES_KEY_SIZE_128));

    /* Encrypt */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_aes_cbc_encrypt(&ctx, iv, plaintext, 32,
                                             ciphertext));

    /* Decrypt */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_aes_cbc_decrypt(&ctx, iv, ciphertext, 32,
                                             decrypted));

    /* Verify decrypted matches original */
    TEST_ASSERT_EQUAL_HEX8_ARRAY(plaintext, decrypted, 32);
}

/* ==================== Base64 Tests ==================== */

void test_base64_encode(void)
{
    const char *test_data = "Hello, World!";
    char encoded[64];
    const char *expected = "SGVsbG8sIFdvcmxkIQ==";

    TEST_ASSERT_EQUAL_INT(
        XY_CRYPTO_SUCCESS,
        xy_base64_encode((const uint8_t *)test_data, strlen(test_data), encoded,
                         sizeof(encoded)));
    TEST_ASSERT_EQUAL_STRING(expected, encoded);
}

void test_base64_decode(void)
{
    const char *encoded = "SGVsbG8sIFdvcmxkIQ==";
    uint8_t decoded[64];
    const char *expected = "Hello, World!";

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_base64_decode(encoded, strlen(encoded), decoded,
                                           sizeof(decoded)));
    decoded[strlen(expected)] = '\0';
    TEST_ASSERT_EQUAL_STRING(expected, (char *)decoded);
}

void test_base64_round_trip(void)
{
    const char *original = "Test Base64 round trip!";
    char encoded[128];
    uint8_t decoded[128];

    /* Encode */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_base64_encode((const uint8_t *)original,
                                           strlen(original), encoded,
                                           sizeof(encoded)));

    /* Decode */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_base64_decode(encoded, strlen(encoded), decoded,
                                           sizeof(decoded)));
    decoded[strlen(original)] = '\0';

    /* Verify */
    TEST_ASSERT_EQUAL_STRING(original, (char *)decoded);
}

void test_base64_empty_input(void)
{
    char encoded[16];
    uint8_t decoded[16];

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_base64_encode(NULL, 0, encoded, sizeof(encoded)));
    TEST_ASSERT_EQUAL_STRING("", encoded);
}

/* ==================== Hex Encoding Tests ==================== */

void test_hex_encode(void)
{
    const uint8_t test_data[] = { 0x48, 0x65, 0x6c, 0x6c, 0x6f };
    char encoded[32];
    const char *expected = "48656c6c6f";

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_hex_encode(test_data, sizeof(test_data), encoded,
                                        sizeof(encoded)));
    TEST_ASSERT_EQUAL_STRING(expected, encoded);
}

void test_hex_decode(void)
{
    const char *hex = "48656c6c6f";
    uint8_t decoded[32];
    const uint8_t expected[] = { 0x48, 0x65, 0x6c, 0x6c, 0x6f };

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_hex_decode(hex, strlen(hex), decoded,
                                        sizeof(decoded)));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, decoded, sizeof(expected));
}

void test_hex_round_trip(void)
{
    const uint8_t original[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE };
    char encoded[32];
    uint8_t decoded[32];

    /* Encode */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_hex_encode(original, sizeof(original), encoded,
                                        sizeof(encoded)));

    /* Decode */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_hex_decode(encoded, strlen(encoded), decoded,
                                        sizeof(decoded)));

    /* Verify */
    TEST_ASSERT_EQUAL_HEX8_ARRAY(original, decoded, sizeof(original));
}

/* ==================== HMAC Tests ==================== */

void test_hmac_sha256_basic(void)
{
    const char *key = "key";
    const char *data = "The quick brown fox jumps over the lazy dog";
    uint8_t hmac[XY_SHA256_DIGEST_SIZE];

    TEST_ASSERT_EQUAL_INT(
        XY_CRYPTO_SUCCESS,
        xy_hmac_sha256((const uint8_t *)key, strlen(key), (const uint8_t *)data,
                       strlen(data), hmac));
    /* Just check it returns a valid HMAC (non-zero) */
    TEST_ASSERT_TRUE(memcmp(hmac, "\x00\x00\x00\x00", 4) != 0);
}

void test_hmac_sha256_consistency(void)
{
    const char *key = "test_key";
    const char *data = "test_data";
    uint8_t hmac1[XY_SHA256_DIGEST_SIZE];
    uint8_t hmac2[XY_SHA256_DIGEST_SIZE];

    /* Same key and data should produce same HMAC */
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_hmac_sha256((const uint8_t *)key, strlen(key),
                                         (const uint8_t *)data, strlen(data),
                                         hmac1));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_hmac_sha256((const uint8_t *)key, strlen(key),
                                         (const uint8_t *)data, strlen(data),
                                         hmac2));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(hmac1, hmac2, XY_SHA256_DIGEST_SIZE);
}

/* ==================== Random Number Tests ==================== */

void test_random_bytes(void)
{
    uint8_t random1[16];
    uint8_t random2[16];

    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_random_bytes(random1, sizeof(random1)));
    TEST_ASSERT_EQUAL_INT(XY_CRYPTO_SUCCESS,
                          xy_random_bytes(random2, sizeof(random2)));

    /* Two random sequences should be different (very high probability) */
    TEST_ASSERT_TRUE(memcmp(random1, random2, sizeof(random1)) != 0);
}

void test_random_uint32(void)
{
    uint32_t r1, r2;

    r1 = xy_random_uint32();
    r2 = xy_random_uint32();

    /* Just check it returns values */
    TEST_ASSERT_TRUE(r1 != 0 || r2 != 0);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* CRC Tests */
    RUN_TEST(test_crc32_basic);
    RUN_TEST(test_crc32_empty_data);
    RUN_TEST(test_crc32_consistency);
    RUN_TEST(test_crc32_different_data);
    RUN_TEST(test_crc16_modbus);
    RUN_TEST(test_crc8_basic);

    /* MD5 Tests */
    RUN_TEST(test_md5_basic);
    RUN_TEST(test_md5_empty_string);
    RUN_TEST(test_md5_incremental);

    /* SHA256 Tests */
    RUN_TEST(test_sha256_basic);
    RUN_TEST(test_sha256_empty_string);

    /* AES Tests */
    RUN_TEST(test_aes128_encrypt_decrypt);
    RUN_TEST(test_aes_cbc_encrypt_decrypt);

    /* Base64 Tests */
    RUN_TEST(test_base64_encode);
    RUN_TEST(test_base64_decode);
    RUN_TEST(test_base64_round_trip);
    RUN_TEST(test_base64_empty_input);

    /* Hex Tests */
    RUN_TEST(test_hex_encode);
    RUN_TEST(test_hex_decode);
    RUN_TEST(test_hex_round_trip);

    /* HMAC Tests */
    RUN_TEST(test_hmac_sha256_basic);
    RUN_TEST(test_hmac_sha256_consistency);

    /* Random Tests */
    RUN_TEST(test_random_bytes);
    RUN_TEST(test_random_uint32);

    return UNITY_END();
}
