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

static void test_tinyjambu_192_and_256_roundtrip_boundaries(void)
{
    static const size_t lengths[] = {1U, 15U, 16U, 17U, 32U};
    uint8_t key192[XY_TINYJAMBU_192_KEY_SIZE] = {0};
    uint8_t key256[XY_TINYJAMBU_256_KEY_SIZE] = {0};
    uint8_t nonce[XY_TINYJAMBU_192_NONCE_SIZE] = {0};
    uint8_t ad[5] = {1U, 2U, 3U, 4U, 5U};
    uint8_t plaintext[32];
    uint8_t ciphertext[33];
    uint8_t decrypted[33];
    uint8_t tag[XY_TINYJAMBU_192_TAG_SIZE];
    uint8_t bad_tag[XY_TINYJAMBU_192_TAG_SIZE];
    size_t case_index;
    size_t index;

    for (index = 0; index < sizeof(plaintext); ++index) {
        plaintext[index] = (uint8_t)(index + 1U);
    }
    for (case_index = 0; case_index < sizeof(lengths) / sizeof(lengths[0]); ++case_index) {
        size_t length = lengths[case_index];

        memset(ciphertext, 0xA5, sizeof(ciphertext));
        TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                              xy_tinyjambu_192_encrypt(key192, nonce, ad, sizeof(ad), plaintext,
                                                       length, ciphertext, tag));
        TEST_ASSERT_EQUAL_HEX8(0xA5U, ciphertext[length]);
        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                              xy_tinyjambu_192_decrypt(key192, nonce, ad, sizeof(ad), ciphertext,
                                                       length, decrypted, tag));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, decrypted, length);
        TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[length]);

        memcpy(bad_tag, tag, sizeof(bad_tag));
        bad_tag[0] ^= 0x01U;
        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_AUTH_FAILED,
                              xy_tinyjambu_192_decrypt(key192, nonce, ad, sizeof(ad), ciphertext,
                                                       length, decrypted, bad_tag));
        TEST_ASSERT_EACH_EQUAL_UINT8(0U, decrypted, length);
        TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[length]);

        memset(ciphertext, 0xA5, sizeof(ciphertext));
        TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                              xy_tinyjambu_256_encrypt(key256, nonce, ad, sizeof(ad), plaintext,
                                                       length, ciphertext, tag));
        TEST_ASSERT_EQUAL_HEX8(0xA5U, ciphertext[length]);
        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                              xy_tinyjambu_256_decrypt(key256, nonce, ad, sizeof(ad), ciphertext,
                                                       length, decrypted, tag));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, decrypted, length);
        TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[length]);

        memcpy(bad_tag, tag, sizeof(bad_tag));
        bad_tag[0] ^= 0x01U;
        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_AUTH_FAILED,
                              xy_tinyjambu_256_decrypt(key256, nonce, ad, sizeof(ad), ciphertext,
                                                       length, decrypted, bad_tag));
        TEST_ASSERT_EACH_EQUAL_UINT8(0U, decrypted, length);
        TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[length]);
    }
}

static void test_ascon_and_tinyjambu_reject_inconsistent_buffers(void)
{
    uint8_t key128[XY_ASCON_128_KEY_SIZE] = {0};
    uint8_t key80pq[XY_ASCON_80PQ_KEY_SIZE] = {0};
    uint8_t tiny_key192[XY_TINYJAMBU_192_KEY_SIZE] = {0};
    uint8_t tiny_key256[XY_TINYJAMBU_256_KEY_SIZE] = {0};
    uint8_t nonce[XY_ASCON_128_NONCE_SIZE] = {0};
    uint8_t input[16] = {0};
    uint8_t output[16] = {0};
    uint8_t tag[XY_ASCON_128_TAG_SIZE] = {0};

    TEST_ASSERT_EQUAL_INT(XY_ASCON_INVALID_PARAM,
                          xy_ascon_128_encrypt(key128, nonce, NULL, 1U, input, 1U, output, tag));
    TEST_ASSERT_EQUAL_INT(XY_ASCON_INVALID_PARAM,
                          xy_ascon_128a_encrypt(key128, nonce, NULL, 0U, NULL, 1U, output, tag));
    TEST_ASSERT_EQUAL_INT(XY_ASCON_INVALID_PARAM,
                          xy_ascon_80pq_decrypt(key80pq, nonce, NULL, 1U, input, 1U, output, tag));

    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_INVALID_PARAM,
                          xy_tinyjambu_128_encrypt(key128, nonce, NULL, 1U, input, 1U, output, tag));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_INVALID_PARAM,
                          xy_tinyjambu_128_encrypt_tag128(key128, nonce, NULL, 0U, NULL, 1U,
                                                         output, tag));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_INVALID_PARAM,
                          xy_tinyjambu_192_decrypt(tiny_key192, nonce, NULL, 1U, input, 1U,
                                                   output, tag));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_INVALID_PARAM,
                          xy_tinyjambu_256_encrypt(tiny_key256, nonce, NULL, 0U, NULL, 1U,
                                                   output, tag));
}

static void assert_tinyjambu_context_scrubbed(const xy_tinyjambu_128_ctx_t *ctx)
{
    xy_tinyjambu_128_ctx_t expected;

    memset(&expected, 0, sizeof(expected));
    expected.mode = 3;
    TEST_ASSERT_EQUAL_MEMORY(&expected, ctx, sizeof(expected));
}

static void test_tinyjambu_incremental_scrubs_context_on_final_paths(void)
{
    uint8_t key[XY_TINYJAMBU_128_KEY_SIZE];
    uint8_t nonce[XY_TINYJAMBU_128_NONCE_SIZE];
    uint8_t plaintext[16];
    uint8_t ciphertext[sizeof(plaintext)];
    uint8_t decrypted[sizeof(plaintext)];
    uint8_t tag[XY_TINYJAMBU_128_TAG_SIZE];
    xy_tinyjambu_128_ctx_t ctx;

    memset(key, 0x11, sizeof(key));
    memset(nonce, 0x22, sizeof(nonce));
    memset(plaintext, 0x33, sizeof(plaintext));

    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_encrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_encrypt_ad(&ctx, plaintext, 1U));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_encrypt_update(&ctx, plaintext, sizeof(plaintext),
                                                         ciphertext));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_encrypt_final(&ctx, tag));
    assert_tinyjambu_context_scrubbed(&ctx);

    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_decrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_decrypt_ad(&ctx, plaintext, 1U));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_decrypt_update(&ctx, ciphertext, sizeof(ciphertext),
                                                         decrypted));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_decrypt_final(&ctx, tag));
    assert_tinyjambu_context_scrubbed(&ctx);

    tag[0] ^= 0x01U;
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_decrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_decrypt_ad(&ctx, plaintext, 1U));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_decrypt_update(&ctx, ciphertext, sizeof(ciphertext),
                                                         decrypted));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_AUTH_FAILED,
                          xy_tinyjambu_128_decrypt_final(&ctx, tag));
    assert_tinyjambu_context_scrubbed(&ctx);

    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_decrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_decrypt_ad(&ctx, plaintext, 1U));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_decrypt_update(&ctx, ciphertext, sizeof(ciphertext),
                                                         decrypted));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_INVALID_PARAM,
                          xy_tinyjambu_128_decrypt_final(&ctx, NULL));
    assert_tinyjambu_context_scrubbed(&ctx);

    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_encrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_encrypt_ad(&ctx, plaintext, 1U));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_SUCCESS,
                          xy_tinyjambu_128_encrypt_update(&ctx, plaintext, sizeof(plaintext),
                                                         ciphertext));
    TEST_ASSERT_EQUAL_INT(XY_TINYJAMBU_INVALID_PARAM,
                          xy_tinyjambu_128_encrypt_final(&ctx, NULL));
    assert_tinyjambu_context_scrubbed(&ctx);
}

static void test_photon_beetle_incremental_matches_one_shot(void)
{
    uint8_t key[XY_PHOTON_BEETLE_KEY_SIZE] = {0};
    uint8_t nonce[XY_PHOTON_BEETLE_NONCE_SIZE] = {0};
    uint8_t plaintext[17];
    uint8_t expected_ciphertext[sizeof(plaintext)];
    uint8_t incremental_ciphertext[sizeof(plaintext)];
    uint8_t expected_tag[XY_PHOTON_BEETLE_TAG_SIZE];
    uint8_t incremental_tag[XY_PHOTON_BEETLE_TAG_SIZE];
    xy_photon_beetle_ctx_t ctx;
    size_t index;

    for (index = 0U; index < sizeof(plaintext); ++index) {
        plaintext[index] = (uint8_t)(index + 1U);
    }
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt(key, nonce, NULL, 0U, plaintext,
                                                    sizeof(plaintext), expected_ciphertext,
                                                    expected_tag));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_ad(&ctx, NULL, 0U));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_update(&ctx, plaintext, sizeof(plaintext),
                                                          incremental_ciphertext));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_final(&ctx, incremental_tag));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_ciphertext, incremental_ciphertext,
                                  sizeof(expected_ciphertext));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_tag, incremental_tag, sizeof(expected_tag));
}

static void test_photon_beetle_incremental_accepts_chunked_data(void)
{
    static const size_t chunks[] = {1U, 7U, 9U, 16U};
    uint8_t key[XY_PHOTON_BEETLE_KEY_SIZE] = {0};
    uint8_t nonce[XY_PHOTON_BEETLE_NONCE_SIZE] = {0};
    uint8_t ad[3] = {0xA1U, 0xB2U, 0xC3U};
    uint8_t plaintext[33];
    uint8_t expected_ciphertext[sizeof(plaintext)];
    uint8_t incremental_ciphertext[sizeof(plaintext)];
    uint8_t decrypted[sizeof(plaintext)];
    uint8_t expected_tag[XY_PHOTON_BEETLE_TAG_SIZE];
    uint8_t incremental_tag[XY_PHOTON_BEETLE_TAG_SIZE];
    xy_photon_beetle_ctx_t ctx;
    size_t chunk_index;
    size_t index;
    size_t offset;

    for (index = 0U; index < sizeof(plaintext); ++index) {
        plaintext[index] = (uint8_t)(0x30U + index);
    }
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt(key, nonce, ad, sizeof(ad), plaintext,
                                                    sizeof(plaintext), expected_ciphertext,
                                                    expected_tag));

    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_ad(&ctx, ad, sizeof(ad)));
    offset = 0U;
    for (chunk_index = 0U; chunk_index < sizeof(chunks) / sizeof(chunks[0]); ++chunk_index) {
        TEST_ASSERT_EQUAL_INT(
            XY_PHOTON_BEETLE_SUCCESS,
            xy_photon_beetle_encrypt_update(&ctx, plaintext + offset, chunks[chunk_index],
                                            incremental_ciphertext + offset));
        offset += chunks[chunk_index];
    }
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_final(&ctx, incremental_tag));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_ciphertext, incremental_ciphertext,
                                  sizeof(expected_ciphertext));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_tag, incremental_tag, sizeof(expected_tag));

    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_ad(&ctx, ad, sizeof(ad)));
    offset = 0U;
    for (chunk_index = 0U; chunk_index < sizeof(chunks) / sizeof(chunks[0]); ++chunk_index) {
        TEST_ASSERT_EQUAL_INT(
            XY_PHOTON_BEETLE_SUCCESS,
            xy_photon_beetle_decrypt_update(&ctx, incremental_ciphertext + offset,
                                            chunks[chunk_index], decrypted + offset));
        offset += chunks[chunk_index];
    }
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_final(&ctx, incremental_tag));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, decrypted, sizeof(plaintext));

    incremental_tag[0] ^= 0x01U;
    memset(decrypted, 0xA5, sizeof(decrypted));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_ad(&ctx, ad, sizeof(ad)));
    offset = 0U;
    for (chunk_index = 0U; chunk_index < sizeof(chunks) / sizeof(chunks[0]); ++chunk_index) {
        TEST_ASSERT_EQUAL_INT(
            XY_PHOTON_BEETLE_SUCCESS,
            xy_photon_beetle_decrypt_update(&ctx, incremental_ciphertext + offset,
                                            chunks[chunk_index], decrypted + offset));
        offset += chunks[chunk_index];
    }
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_AUTH_FAILED,
                          xy_photon_beetle_decrypt_final(&ctx, incremental_tag));
    TEST_ASSERT_EACH_EQUAL_UINT8(0U, decrypted, sizeof(plaintext));
}

static void test_photon_beetle_incremental_guards_state_and_inputs(void)
{
    uint8_t key[XY_PHOTON_BEETLE_KEY_SIZE] = {0};
    uint8_t nonce[XY_PHOTON_BEETLE_NONCE_SIZE] = {0};
    uint8_t data[1] = {0};
    uint8_t output[2] = {0};
    uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE];
    xy_photon_beetle_ctx_t ctx;

    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_INVALID_PARAM,
                          xy_photon_beetle_encrypt_ad(&ctx, NULL, 1U));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_INVALID_PARAM,
                          xy_photon_beetle_encrypt_update(&ctx, NULL, 1U, output));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_ad(&ctx, NULL, 0U));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_update(&ctx, data, sizeof(data), output));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_INVALID_PARAM,
                          xy_photon_beetle_encrypt_ad(&ctx, NULL, 0U));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_final(&ctx, tag));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_INVALID_PARAM,
                          xy_photon_beetle_encrypt_final(&ctx, tag));

    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_ad(&ctx, NULL, 0U));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_update(&ctx, data, sizeof(data), output));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_INVALID_PARAM,
                          xy_photon_beetle_decrypt_update(&ctx, data, sizeof(data), output));
}

static void assert_photon_context_scrubbed(const xy_photon_beetle_ctx_t *ctx)
{
    TEST_ASSERT_EACH_EQUAL_UINT8(0U, ctx->S, sizeof(ctx->S));
    TEST_ASSERT_EACH_EQUAL_UINT8(0U, ctx->K, sizeof(ctx->K));
    TEST_ASSERT_EACH_EQUAL_UINT8(0U, ctx->N, sizeof(ctx->N));
    TEST_ASSERT_EACH_EQUAL_UINT8(0U, ctx->data_block, sizeof(ctx->data_block));
    TEST_ASSERT_EQUAL_UINT(0U, ctx->ad_len);
    TEST_ASSERT_EQUAL_UINT(0U, ctx->plaintext_len);
    TEST_ASSERT_EQUAL_UINT(0U, ctx->data_block_len);
    TEST_ASSERT_NULL(ctx->plaintext);
    TEST_ASSERT_EQUAL_INT(3, ctx->mode);
}

static void test_photon_beetle_incremental_scrubs_context_after_final(void)
{
    uint8_t key[XY_PHOTON_BEETLE_KEY_SIZE];
    uint8_t nonce[XY_PHOTON_BEETLE_NONCE_SIZE];
    uint8_t ad[3] = {0xA1U, 0xB2U, 0xC3U};
    uint8_t plaintext[17];
    uint8_t ciphertext[sizeof(plaintext)];
    uint8_t decrypted[sizeof(plaintext)];
    uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE];
    xy_photon_beetle_ctx_t ctx;

    memset(key, 0x11, sizeof(key));
    memset(nonce, 0x22, sizeof(nonce));
    memset(plaintext, 0x33, sizeof(plaintext));

    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_ad(&ctx, ad, sizeof(ad)));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_update(&ctx, plaintext, sizeof(plaintext),
                                                          ciphertext));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_final(&ctx, tag));
    assert_photon_context_scrubbed(&ctx);

    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_ad(&ctx, ad, sizeof(ad)));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_update(&ctx, ciphertext, sizeof(ciphertext),
                                                          decrypted));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_final(&ctx, tag));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, decrypted, sizeof(plaintext));
    assert_photon_context_scrubbed(&ctx);

    tag[0] ^= 0x01U;
    memset(decrypted, 0xA5, sizeof(decrypted));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_ad(&ctx, ad, sizeof(ad)));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_update(&ctx, ciphertext, sizeof(ciphertext),
                                                          decrypted));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_AUTH_FAILED,
                          xy_photon_beetle_decrypt_final(&ctx, tag));
    TEST_ASSERT_EACH_EQUAL_UINT8(0U, decrypted, sizeof(decrypted));
    assert_photon_context_scrubbed(&ctx);
}

static void test_photon_beetle_incremental_scrubs_context_when_tag_is_missing(void)
{
    uint8_t key[XY_PHOTON_BEETLE_KEY_SIZE];
    uint8_t nonce[XY_PHOTON_BEETLE_NONCE_SIZE];
    uint8_t plaintext[17];
    uint8_t ciphertext[sizeof(plaintext)];
    uint8_t decrypted[sizeof(plaintext)];
    uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE];
    xy_photon_beetle_ctx_t ctx;

    memset(key, 0x11, sizeof(key));
    memset(nonce, 0x22, sizeof(nonce));
    memset(plaintext, 0x33, sizeof(plaintext));

    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_ad(&ctx, NULL, 0U));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt_update(&ctx, plaintext, sizeof(plaintext),
                                                          ciphertext));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_INVALID_PARAM,
                          xy_photon_beetle_encrypt_final(&ctx, NULL));
    assert_photon_context_scrubbed(&ctx);

    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_encrypt(key, nonce, NULL, 0U, plaintext,
                                                    sizeof(plaintext), ciphertext, tag));
    memset(decrypted, 0xA5, sizeof(decrypted));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_init(&ctx, key, nonce));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_ad(&ctx, NULL, 0U));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                          xy_photon_beetle_decrypt_update(&ctx, ciphertext, sizeof(ciphertext),
                                                          decrypted));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_INVALID_PARAM,
                          xy_photon_beetle_decrypt_final(&ctx, NULL));
    TEST_ASSERT_EACH_EQUAL_UINT8(0U, decrypted, sizeof(decrypted));
    assert_photon_context_scrubbed(&ctx);
}

static void test_photon_beetle_roundtrip_boundaries(void)
{
    static const size_t lengths[] = {1U, 15U, 16U, 17U, 32U};
    uint8_t key[XY_PHOTON_BEETLE_KEY_SIZE] = {0};
    uint8_t nonce[XY_PHOTON_BEETLE_NONCE_SIZE] = {0};
    uint8_t ad[5] = {1U, 2U, 3U, 4U, 5U};
    uint8_t plaintext[32];
    uint8_t ciphertext[33];
    uint8_t decrypted[33];
    uint8_t tag[XY_PHOTON_BEETLE_TAG_SIZE];
    uint8_t bad_tag[XY_PHOTON_BEETLE_TAG_SIZE];
    size_t case_index;
    size_t index;

    for (index = 0; index < sizeof(plaintext); ++index) {
        plaintext[index] = (uint8_t)(index + 1U);
    }
    for (case_index = 0; case_index < sizeof(lengths) / sizeof(lengths[0]); ++case_index) {
        size_t length = lengths[case_index];

        memset(ciphertext, 0xA5, sizeof(ciphertext));
        TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                              xy_photon_beetle_encrypt(key, nonce, ad, sizeof(ad), plaintext,
                                                        length, ciphertext, tag));
        TEST_ASSERT_EQUAL_HEX8(0xA5U, ciphertext[length]);

        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_SUCCESS,
                              xy_photon_beetle_decrypt(key, nonce, ad, sizeof(ad), ciphertext,
                                                        length, decrypted, tag));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(plaintext, decrypted, length);
        TEST_ASSERT_EQUAL_HEX8(0xA5U, decrypted[length]);

        memcpy(bad_tag, tag, sizeof(bad_tag));
        bad_tag[0] ^= 0x01U;
        memset(decrypted, 0xA5, sizeof(decrypted));
        TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_AUTH_FAILED,
                              xy_photon_beetle_decrypt(key, nonce, ad, sizeof(ad), ciphertext,
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
                          xy_photon_beetle_encrypt(key, nonce, NULL, 1U, plaintext,
                                                   sizeof(plaintext), ciphertext, tag));
    TEST_ASSERT_EQUAL_INT(XY_PHOTON_BEETLE_INVALID_PARAM,
                          xy_photon_beetle_encrypt(key, nonce, NULL, 0, NULL,
                                                   sizeof(plaintext), ciphertext, tag));
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
    RUN_TEST(test_tinyjambu_192_and_256_roundtrip_boundaries);
    RUN_TEST(test_ascon_and_tinyjambu_reject_inconsistent_buffers);
    RUN_TEST(test_tinyjambu_incremental_scrubs_context_on_final_paths);
    RUN_TEST(test_photon_beetle_incremental_matches_one_shot);
    RUN_TEST(test_photon_beetle_incremental_accepts_chunked_data);
    RUN_TEST(test_photon_beetle_incremental_guards_state_and_inputs);
    RUN_TEST(test_photon_beetle_incremental_scrubs_context_after_final);
    RUN_TEST(test_photon_beetle_incremental_scrubs_context_when_tag_is_missing);
    RUN_TEST(test_photon_beetle_roundtrip_boundaries);
    RUN_TEST(test_photon_beetle_roundtrips_tag_sizes_and_hashes);
    RUN_TEST(test_photon_beetle_rejects_wrong_tag);
    return UNITY_END();
}
