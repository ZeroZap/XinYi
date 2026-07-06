#include "unity.h"
#include "xy_25519.h"

#include <stdint.h>
#include <string.h>

static int random_result;
static uint8_t random_seed;
static unsigned random_call_count;
static unsigned sha512_call_count;

int xy_random_bytes(uint8_t *buffer, size_t len)
{
    random_call_count++;
    if (random_result != 0) {
        return random_result;
    }
    if (!buffer) {
        return -1;
    }
    for (size_t i = 0; i < len; ++i) {
        buffer[i] = (uint8_t)(random_seed + i);
    }
    return 0;
}

int xy_sha512_hash(const uint8_t *data, size_t len, uint8_t digest[64])
{
    sha512_call_count++;
    if (!data || !digest) {
        return -1;
    }
    for (size_t i = 0; i < 64; ++i) {
        digest[i] = (uint8_t)(0xA5U ^ (uint8_t)i ^ (uint8_t)len ^ data[i % len]);
    }
    return 0;
}

void setUp(void)
{
    random_result = 0;
    random_seed = 0x10;
    random_call_count = 0;
    sha512_call_count = 0;
}

void tearDown(void)
{
}

static void fill_nonzero(uint8_t out[32])
{
    for (size_t i = 0; i < 32; ++i) {
        out[i] = (uint8_t)(i + 1U);
    }
}

static void test_x25519_rejects_null_parameters(void)
{
    uint8_t private_key[32];
    uint8_t public_key[32];
    uint8_t shared_secret[32];

    fill_nonzero(private_key);
    fill_nonzero(public_key);

    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_generate_keypair(NULL, public_key));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_generate_keypair(private_key, NULL));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_public_key(NULL, public_key));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_public_key(private_key, NULL));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_shared_secret(NULL, private_key, public_key));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_shared_secret(shared_secret, NULL, public_key));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_shared_secret(shared_secret, private_key, NULL));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_validate_public_key(NULL));
}

static void test_x25519_validate_public_key_rejects_low_order_points(void)
{
    uint8_t key[32] = {0};

    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_WEAK_KEY, xy_x25519_validate_public_key(key));

    key[0] = 1;
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_WEAK_KEY, xy_x25519_validate_public_key(key));

    fill_nonzero(key);
    TEST_ASSERT_EQUAL_INT(XY_X25519_SUCCESS, xy_x25519_validate_public_key(key));
}

static void test_x25519_generate_keypair_uses_rng_and_derives_public_key(void)
{
    uint8_t private_key[32] = {0};
    uint8_t public_key[32] = {0};
    uint8_t expected_private[32];
    uint8_t expected_public[32];

    for (size_t i = 0; i < 32; ++i) {
        expected_private[i] = (uint8_t)(random_seed + i);
    }

    TEST_ASSERT_EQUAL_INT(XY_X25519_SUCCESS, xy_x25519_generate_keypair(private_key, public_key));
    TEST_ASSERT_EQUAL_UINT(1U, random_call_count);
    TEST_ASSERT_EQUAL_MEMORY(expected_private, private_key, sizeof(expected_private));

    TEST_ASSERT_EQUAL_INT(XY_X25519_SUCCESS, xy_x25519_public_key(expected_private, expected_public));
    TEST_ASSERT_EQUAL_MEMORY(expected_public, public_key, sizeof(expected_public));
}

static void test_x25519_generate_keypair_reports_rng_failure(void)
{
    uint8_t private_key[32] = {0};
    uint8_t public_key[32] = {0};

    random_result = -7;

    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR, xy_x25519_generate_keypair(private_key, public_key));
    TEST_ASSERT_EQUAL_UINT(1U, random_call_count);
}

static void test_ed25519_rejects_null_parameters_and_uses_hash_dependency(void)
{
    uint8_t private_key[32];
    uint8_t public_key[32];
    uint8_t signature[64];
    const uint8_t message[] = "abc";

    fill_nonzero(private_key);
    fill_nonzero(public_key);

    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_generate_keypair(NULL, private_key));
    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_generate_keypair(public_key, NULL));
    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_public_key(NULL, public_key));
    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_public_key(private_key, NULL));
    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_sign(NULL, message, sizeof(message), public_key, private_key));
    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_sign(signature, NULL, sizeof(message), public_key, private_key));
    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_sign(signature, message, sizeof(message), NULL, private_key));
    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_sign(signature, message, sizeof(message), public_key, NULL));
    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_verify(NULL, message, sizeof(message), public_key));
    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_verify(signature, NULL, sizeof(message), public_key));
    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_verify(signature, message, sizeof(message), NULL));
    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_sign_simple(NULL, message, sizeof(message), private_key));
    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_sign_simple(signature, NULL, sizeof(message), private_key));
    TEST_ASSERT_EQUAL_INT(XY_ED25519_ERROR_INVALID_PARAM,
                          xy_ed25519_sign_simple(signature, message, sizeof(message), NULL));

    TEST_ASSERT_EQUAL_INT(XY_ED25519_SUCCESS, xy_ed25519_public_key(private_key, public_key));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sha512_call_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_x25519_rejects_null_parameters);
    RUN_TEST(test_x25519_validate_public_key_rejects_low_order_points);
    RUN_TEST(test_x25519_generate_keypair_uses_rng_and_derives_public_key);
    RUN_TEST(test_x25519_generate_keypair_reports_rng_failure);
    RUN_TEST(test_ed25519_rejects_null_parameters_and_uses_hash_dependency);
    return UNITY_END();
}
