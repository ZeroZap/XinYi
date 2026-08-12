/**
 * @file test_ecdsa_root_contract.c
 * @brief Root aggregate ECDSA format-only contract guard.
 *
 * This test intentionally exercises the current root/runtime source copy
 * components/crypto/src/xy_ecdsa.c. It is a host API guard for defensive
 * parameter handling and the documented placeholder-grade format-only path;
 * it is not a cryptographic signature validation or provenance review.
 */

#include "unity.h"
#include "xy_ecdsa.h"

#include <string.h>

static const uint8_t kMessage[] = {'X', 'i', 'n', 'Y', 'i'};

void setUp(void) {}
void tearDown(void) {}

static void fill_bytes(uint8_t *buffer, size_t len, uint8_t value)
{
    for (size_t i = 0; i < len; ++i) {
        buffer[i] = value;
    }
}

static void make_valid_looking_inputs(xy_ecdsa_pub_key_t *pub_key, xy_ecdsa_sig_t *sig)
{
    memset(pub_key, 0, sizeof(*pub_key));
    memset(sig, 0, sizeof(*sig));
    fill_bytes(pub_key->x, sizeof(pub_key->x), 0x01U);
    fill_bytes(pub_key->y, sizeof(pub_key->y), 0x02U);
    fill_bytes(sig->r, sizeof(sig->r), 0x03U);
    fill_bytes(sig->s, sizeof(sig->s), 0x04U);
}

static void test_ecdsa_root_rejects_null_inputs(void)
{
    xy_ecdsa_pub_key_t pub_key;
    xy_ecdsa_sig_t sig;

    make_valid_looking_inputs(&pub_key, &sig);

    TEST_ASSERT_EQUAL_INT(-1, xy_ecdsa_p256_verify(NULL, kMessage, sizeof(kMessage), &sig));
    TEST_ASSERT_EQUAL_INT(-1, xy_ecdsa_p256_verify(&pub_key, NULL, sizeof(kMessage), &sig));
    TEST_ASSERT_EQUAL_INT(-1, xy_ecdsa_p256_verify(&pub_key, kMessage, sizeof(kMessage), NULL));
    TEST_ASSERT_EQUAL_INT(-1, xy_ecdsa_verify_simple(NULL, kMessage, sizeof(kMessage), sig.r));
    TEST_ASSERT_EQUAL_INT(-1, xy_ecdsa_verify_simple(pub_key.x, NULL, sizeof(kMessage), sig.r));
    TEST_ASSERT_EQUAL_INT(-1, xy_ecdsa_verify_simple(pub_key.x, kMessage, sizeof(kMessage), NULL));
}

static void test_ecdsa_root_rejects_zero_public_key(void)
{
    xy_ecdsa_pub_key_t pub_key;
    xy_ecdsa_sig_t sig;

    make_valid_looking_inputs(&pub_key, &sig);
    memset(pub_key.x, 0, sizeof(pub_key.x));
    memset(pub_key.y, 0, sizeof(pub_key.y));

    TEST_ASSERT_EQUAL_INT(-1, xy_ecdsa_p256_verify(&pub_key, kMessage, sizeof(kMessage), &sig));
}

static void test_ecdsa_root_rejects_out_of_range_signature_or_key_bytes(void)
{
    xy_ecdsa_pub_key_t pub_key;
    xy_ecdsa_sig_t sig;

    make_valid_looking_inputs(&pub_key, &sig);
    fill_bytes(sig.r, sizeof(sig.r), 0xFFU);
    TEST_ASSERT_EQUAL_INT(-1, xy_ecdsa_p256_verify(&pub_key, kMessage, sizeof(kMessage), &sig));

    make_valid_looking_inputs(&pub_key, &sig);
    fill_bytes(sig.s, sizeof(sig.s), 0xFFU);
    TEST_ASSERT_EQUAL_INT(-1, xy_ecdsa_p256_verify(&pub_key, kMessage, sizeof(kMessage), &sig));

    make_valid_looking_inputs(&pub_key, &sig);
    fill_bytes(pub_key.x, sizeof(pub_key.x), 0xFFU);
    TEST_ASSERT_EQUAL_INT(-1, xy_ecdsa_p256_verify(&pub_key, kMessage, sizeof(kMessage), &sig));

    make_valid_looking_inputs(&pub_key, &sig);
    fill_bytes(pub_key.y, sizeof(pub_key.y), 0xFFU);
    TEST_ASSERT_EQUAL_INT(-1, xy_ecdsa_p256_verify(&pub_key, kMessage, sizeof(kMessage), &sig));
}

static void test_ecdsa_root_documents_format_only_success_contract(void)
{
    xy_ecdsa_pub_key_t pub_key;
    xy_ecdsa_sig_t sig;
    uint8_t pub_key_bytes[XY_ECDSA_P256_PUB_KEY_SIZE];
    uint8_t sig_bytes[XY_ECDSA_P256_SIG_SIZE];

    make_valid_looking_inputs(&pub_key, &sig);

    /* Current root ECDSA is placeholder-grade: valid-looking fields return success
     * without real elliptic-curve signature verification. This assertion documents the
     * existing contract so consumers do not confuse the source with security-reviewed ECDSA. */
    TEST_ASSERT_EQUAL_INT(0, xy_ecdsa_p256_verify(&pub_key, kMessage, sizeof(kMessage), &sig));

    memcpy(pub_key_bytes, pub_key.x, sizeof(pub_key.x));
    memcpy(pub_key_bytes + sizeof(pub_key.x), pub_key.y, sizeof(pub_key.y));
    memcpy(sig_bytes, sig.r, sizeof(sig.r));
    memcpy(sig_bytes + sizeof(sig.r), sig.s, sizeof(sig.s));
    TEST_ASSERT_EQUAL_INT(0, xy_ecdsa_verify_simple(pub_key_bytes, kMessage, sizeof(kMessage), sig_bytes));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ecdsa_root_rejects_null_inputs);
    RUN_TEST(test_ecdsa_root_rejects_zero_public_key);
    RUN_TEST(test_ecdsa_root_rejects_out_of_range_signature_or_key_bytes);
    RUN_TEST(test_ecdsa_root_documents_format_only_success_contract);
    return UNITY_END();
}
