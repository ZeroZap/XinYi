#include "unity.h"

#include "xy_ascon.h"
#include "xy_photon_beetle.h"
#include "xy_tinyjambu.h"

#include <stdint.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_ascon_encrypt_variants_and_hash(void)
{
    uint8_t key128[XY_ASCON_128_KEY_SIZE] = {0};
    uint8_t key80pq[XY_ASCON_80PQ_KEY_SIZE] = {0};
    uint8_t nonce[XY_ASCON_128_NONCE_SIZE] = {0};
    uint8_t plaintext[64];
    uint8_t ciphertext[sizeof(plaintext)];
    uint8_t tag[XY_ASCON_128_TAG_SIZE];
    uint8_t hash[XY_ASCON_HASH_SIZE];

    memset(plaintext, 0x42, sizeof(plaintext));

    TEST_ASSERT_EQUAL_INT(XY_ASCON_SUCCESS,
                          xy_ascon_128_encrypt(key128, nonce, NULL, 0, plaintext, 32,
                                                ciphertext, tag));
    TEST_ASSERT_EQUAL_INT(XY_ASCON_SUCCESS,
                          xy_ascon_128a_encrypt(key128, nonce, NULL, 0, plaintext,
                                                 sizeof(plaintext), ciphertext, tag));
    TEST_ASSERT_EQUAL_INT(XY_ASCON_SUCCESS,
                          xy_ascon_80pq_encrypt(key80pq, nonce, NULL, 0, plaintext, 16,
                                                 ciphertext, tag));
    TEST_ASSERT_EQUAL_INT(XY_ASCON_SUCCESS,
                          xy_ascon_hash((const uint8_t *)"Test message for hashing", 24, hash));
    TEST_ASSERT_NOT_EQUAL_UINT8(0U, hash[0] | hash[1]);
}

static void test_ascon_128_roundtrips_and_rejects_bad_tag(void)
{
    static const size_t lengths[] = {1U, 7U, 8U, 9U, 16U};
    uint8_t key[XY_ASCON_128_KEY_SIZE] = {0};
    uint8_t nonce[XY_ASCON_128_NONCE_SIZE] = {0};
    uint8_t ad[5] = {1U, 2U, 3U, 4U, 5U};
    uint8_t plaintext[16];
    uint8_t ciphertext[17];
    uint8_t decrypted[17];
    uint8_t tag[XY_ASCON_128_TAG_SIZE];
    uint8_t bad_tag[XY_ASCON_128_TAG_SIZE];
    size_t case_index;
    size_t index;

    for (index = 0; index < sizeof(plaintext); ++index) {
        plaintext[index] = (uint8_t)(index + 1U);
    }

    for (case_index = 0; case_index < sizeof(lengths) / sizeof(lengths[0]); ++case_index) {
        size_t length = lengths[case_index];

        memset(ciphertext, 0xA5, sizeof(ciphertext));
        TEST_ASSERT_EQUAL_INT(XY_ASCON_SUCCESS,
                              xy_ascon_128_encrypt(key, nonce, ad, sizeof(ad), plaintext, length,
                                                   ciphertext, tag));
        TEST_ASSERT_EQUAL_HEX8(0xA5U, ciphertext[length]);

        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_ASCON_SUCCESS,
                              xy_ascon_128_decrypt(key, nonce, ad, sizeof(ad), ciphertext, length,
                                                   decrypted, tag));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, decrypted, length);
        TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[length]);

        memcpy(bad_tag, tag, sizeof(bad_tag));
        bad_tag[0] ^= 0x01U;
        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_ASCON_AUTH_FAILED,
                              xy_ascon_128_decrypt(key, nonce, ad, sizeof(ad), ciphertext, length,
                                                   decrypted, bad_tag));
        TEST_ASSERT_EACH_EQUAL_UINT8(0U, decrypted, length);
        TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[length]);
    }
}

static void test_ascon_128a_roundtrips_and_rejects_bad_tag(void)
{
    static const size_t lengths[] = {1U, 15U, 16U, 17U, 32U};
    uint8_t key[XY_ASCON_128A_KEY_SIZE] = {0};
    uint8_t nonce[XY_ASCON_128A_NONCE_SIZE] = {0};
    uint8_t ad[5] = {1U, 2U, 3U, 4U, 5U};
    uint8_t plaintext[32];
    uint8_t ciphertext[33];
    uint8_t decrypted[33];
    uint8_t tag[XY_ASCON_128A_TAG_SIZE];
    uint8_t bad_tag[XY_ASCON_128A_TAG_SIZE];
    size_t case_index;
    size_t index;

    for (index = 0; index < sizeof(plaintext); ++index) {
        plaintext[index] = (uint8_t)(index + 1U);
    }
    for (case_index = 0; case_index < sizeof(lengths) / sizeof(lengths[0]); ++case_index) {
        size_t length = lengths[case_index];

        memset(ciphertext, 0xA5, sizeof(ciphertext));
        TEST_ASSERT_EQUAL_INT(XY_ASCON_SUCCESS,
                              xy_ascon_128a_encrypt(key, nonce, ad, sizeof(ad), plaintext, length,
                                                    ciphertext, tag));
        TEST_ASSERT_EQUAL_HEX8(0xA5U, ciphertext[length]);

        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_ASCON_SUCCESS,
                              xy_ascon_128a_decrypt(key, nonce, ad, sizeof(ad), ciphertext, length,
                                                    decrypted, tag));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, decrypted, length);
        TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[length]);

        memcpy(bad_tag, tag, sizeof(bad_tag));
        bad_tag[0] ^= 0x01U;
        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_ASCON_AUTH_FAILED,
                              xy_ascon_128a_decrypt(key, nonce, ad, sizeof(ad), ciphertext, length,
                                                    decrypted, bad_tag));
        TEST_ASSERT_EACH_EQUAL_UINT8(0U, decrypted, length);
        TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[length]);
    }
}

static void test_ascon_80pq_roundtrips_and_rejects_bad_tag(void)
{
    static const size_t lengths[] = {1U, 7U, 8U, 9U, 16U};
    uint8_t key[XY_ASCON_80PQ_KEY_SIZE] = {0};
    uint8_t nonce[XY_ASCON_80PQ_NONCE_SIZE] = {0};
    uint8_t ad[5] = {1U, 2U, 3U, 4U, 5U};
    uint8_t plaintext[16];
    uint8_t ciphertext[17];
    uint8_t decrypted[17];
    uint8_t tag[XY_ASCON_80PQ_TAG_SIZE];
    uint8_t bad_tag[XY_ASCON_80PQ_TAG_SIZE];
    size_t case_index;
    size_t index;

    for (index = 0; index < sizeof(plaintext); ++index) {
        plaintext[index] = (uint8_t)(index + 1U);
    }
    for (case_index = 0; case_index < sizeof(lengths) / sizeof(lengths[0]); ++case_index) {
        size_t length = lengths[case_index];

        memset(ciphertext, 0xA5, sizeof(ciphertext));
        TEST_ASSERT_EQUAL_INT(XY_ASCON_SUCCESS,
                              xy_ascon_80pq_encrypt(key, nonce, ad, sizeof(ad), plaintext, length,
                                                    ciphertext, tag));
        TEST_ASSERT_EQUAL_HEX8(0xA5U, ciphertext[length]);

        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_ASCON_SUCCESS,
                              xy_ascon_80pq_decrypt(key, nonce, ad, sizeof(ad), ciphertext, length,
                                                    decrypted, tag));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, decrypted, length);
        TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[length]);

        memcpy(bad_tag, tag, sizeof(bad_tag));
        bad_tag[0] ^= 0x01U;
        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_ASCON_AUTH_FAILED,
                              xy_ascon_80pq_decrypt(key, nonce, ad, sizeof(ad), ciphertext, length,
                                                    decrypted, bad_tag));
        TEST_ASSERT_EACH_EQUAL_UINT8(0U, decrypted, length);
        TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[length]);
    }
}

static void test_tinyjambu_encrypt_variants(void)
{
    uint8_t nonce[XY_TINYJAMBU_128_NONCE_SIZE] = {0};
    uint8_t key128[XY_TINYJAMBU_128_KEY_SIZE] = {0};
    uint8_t key192[XY_TINYJAMBU_192_KEY_SIZE] = {0};
    uint8_t key256[XY_TINYJAMBU_256_KEY_SIZE] = {0};
    uint8_t plaintext[32] = "Test data for TinyJambu-128!";
    uint8_t ciphertext[sizeof(plaintext)];
    uint8_t tag[XY_TINYJAMBU_128_TAG_SIZE];

    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_encrypt(key128, nonce, NULL, 0, plaintext,
                                                    sizeof(plaintext), ciphertext, tag));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_192_encrypt(key192, nonce, NULL, 0, plaintext, 16,
                                                    ciphertext, tag));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_256_encrypt(key256, nonce, NULL, 0, plaintext, 16,
                                                    ciphertext, tag));
}

static void test_tinyjambu_128_roundtrips_and_rejects_bad_tag(void)
{
    static const size_t lengths[] = {1U, 15U, 16U, 17U, 32U};
    uint8_t key[XY_TINYJAMBU_128_KEY_SIZE] = {0};
    uint8_t nonce[XY_TINYJAMBU_128_NONCE_SIZE] = {0};
    uint8_t ad[5] = {1U, 2U, 3U, 4U, 5U};
    uint8_t plaintext[32];
    uint8_t ciphertext[33];
    uint8_t decrypted[33];
    uint8_t tag[XY_TINYJAMBU_128_TAG_SIZE];
    uint8_t bad_tag[XY_TINYJAMBU_128_TAG_SIZE];
    size_t case_index;
    size_t index;

    for (index = 0; index < sizeof(plaintext); ++index) {
        plaintext[index] = (uint8_t)(index + 1U);
    }
    for (case_index = 0; case_index < sizeof(lengths) / sizeof(lengths[0]); ++case_index) {
        size_t length = lengths[case_index];

        memset(ciphertext, 0xA5, sizeof(ciphertext));
        TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                              xy_tinyjambu_128_encrypt(key, nonce, ad, sizeof(ad), plaintext,
                                                       length, ciphertext, tag));
        TEST_ASSERT_EQUAL_HEX8(0xA5U, ciphertext[length]);

        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                              xy_tinyjambu_128_decrypt(key, nonce, ad, sizeof(ad), ciphertext,
                                                       length, decrypted, tag));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, decrypted, length);
        TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[length]);

        memcpy(bad_tag, tag, sizeof(bad_tag));
        bad_tag[0] ^= 0x01U;
        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_AUTH_FAILED,
                              xy_tinyjambu_128_decrypt(key, nonce, ad, sizeof(ad), ciphertext,
                                                       length, decrypted, bad_tag));
        TEST_ASSERT_EACH_EQUAL_UINT8(0U, decrypted, length);
        TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[length]);
    }
}

static void test_photon_beetle_roundtrips_tag_sizes_and_hashes(void)
{
    uint8_t key[XY_PHOTON_BEETLE_KEY_SIZE] = {0};
    uint8_t nonce[XY_PHOTON_BEETLE_NONCE_SIZE] = {0};
    uint8_t ad[] = "Associated Data Block";
    uint8_t plaintext[32] = "Test data for Photon Beetle!";
    uint8_t ciphertext[sizeof(plaintext)];
    uint8_t decrypted[sizeof(plaintext)];
    uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE];
    uint8_t tag64[XY_PHOTON_BEETLE_TAG_64_SIZE];
    uint8_t bad_tag64[XY_PHOTON_BEETLE_TAG_64_SIZE];
    uint8_t hash[XY_PHOTON_HASH_SIZE];

    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt(key, nonce, ad, strlen((char *)ad), plaintext,
                                                    sizeof(plaintext), ciphertext, tag));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt(key, nonce, ad, strlen((char *)ad), ciphertext,
                                                    sizeof(ciphertext), decrypted, tag));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, decrypted, sizeof(plaintext));

    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_tag64(key, nonce, NULL, 0, plaintext, 16,
                                                          ciphertext, tag64));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_tag64(key, nonce, NULL, 0, ciphertext, 16,
                                                          decrypted, tag64));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, decrypted, 16);

    memcpy(bad_tag64, tag64, sizeof(bad_tag64));
    bad_tag64[0] ^= 0x01U;
    memset(decrypted, 0xA5, sizeof(decrypted));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_AUTH_FAILED,
                          xy_photon_beetle_decrypt_tag64(key, nonce, NULL, 0, ciphertext, 16,
                                                          decrypted, bad_tag64));
    TEST_ASSERT_EACH_EQUAL_UINT8(0U, decrypted, 16U);
    TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[16]);

    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_hash((const uint8_t *)"Test message for Photon hash", 28,
                                         hash));
    TEST_ASSERT_NOT_EQUAL_UINT8(0U, hash[0] | hash[1]);
}

static void test_photon_beetle_rejects_wrong_tag(void)
{
    uint8_t key[XY_PHOTON_BEETLE_KEY_SIZE] = {0};
    uint8_t nonce[XY_PHOTON_BEETLE_NONCE_SIZE] = {0};
    uint8_t plaintext[16] = {0};
    uint8_t ciphertext[sizeof(plaintext)];
    uint8_t decrypted[sizeof(plaintext)];
    uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE];
    uint8_t bad_tag[XY_PHOTON_BEETLE_TAG_SIZE] = {0xFF};

    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_INVALID_PARAM,
                          xy_photon_beetle_encrypt_tag64(key, nonce, NULL, 0, plaintext,
                                                          sizeof(plaintext), ciphertext, NULL));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_INVALID_PARAM,
                          xy_photon_beetle_decrypt_tag64(key, nonce, NULL, 0, ciphertext,
                                                          sizeof(ciphertext), decrypted, NULL));

    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt(key, nonce, NULL, 0, plaintext,
                                                    sizeof(plaintext), ciphertext, tag));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_AUTH_FAILED,
                          xy_photon_beetle_decrypt(key, nonce, NULL, 0, ciphertext,
                                                    sizeof(ciphertext), decrypted, bad_tag));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ascon_encrypt_variants_and_hash);
    RUN_TEST(test_ascon_128_roundtrips_and_rejects_bad_tag);
    RUN_TEST(test_ascon_128a_roundtrips_and_rejects_bad_tag);
    RUN_TEST(test_ascon_80pq_roundtrips_and_rejects_bad_tag);
    RUN_TEST(test_tinyjambu_encrypt_variants);
    RUN_TEST(test_tinyjambu_128_roundtrips_and_rejects_bad_tag);
    RUN_TEST(test_photon_beetle_roundtrips_tag_sizes_and_hashes);
    RUN_TEST(test_photon_beetle_rejects_wrong_tag);
    return UNITY_END();
}
