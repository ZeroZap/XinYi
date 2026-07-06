#include "unity.h"
#include "xy_25519.h"
#include "fe25519_m0.h"

#include <stdint.h>
#include <string.h>

int xy_x25519_m0_scalarmult(uint8_t result[32], const uint8_t scalar[32], const uint8_t point[32]);
int xy_x25519_m0_public_key(const uint8_t private_key[32], uint8_t public_key[32]);
int xy_x25519_m0_shared_secret(uint8_t shared_secret[32], const uint8_t our_private_key[32], const uint8_t their_public_key[32]);
int xy_x25519_m0_validate_public_key(const uint8_t public_key[32]);

void multiply256x256_asm(uint32_t result[16], const uint32_t a[8], const uint32_t b[8])
{
    memset(result, 0, 16U * sizeof(result[0]));
    for (size_t i = 0; i < 8; ++i) {
        uint64_t carry = 0;
        for (size_t j = 0; j < 8; ++j) {
            uint64_t acc = (uint64_t)result[i + j] + ((uint64_t)a[i] * (uint64_t)b[j]) + carry;
            result[i + j] = (uint32_t)acc;
            carry = acc >> 32;
        }
        result[i + 8] = (uint32_t)((uint64_t)result[i + 8] + carry);
    }
}

void square256_asm(uint32_t result[16], const uint32_t a[8])
{
    multiply256x256_asm(result, a, a);
}

void fe25519_reduceTo256Bits_asm(uint32_t result[8], const uint32_t input[16])
{
    memcpy(result, input, 8U * sizeof(result[0]));
    result[7] &= 0x7FFFFFFFU;
}

void fe25519_mpyWith121666_asm(uint32_t out[8], const uint32_t in[8])
{
    uint64_t carry = 0;
    for (size_t i = 0; i < 8; ++i) {
        uint64_t acc = ((uint64_t)in[i] * 121666U) + carry;
        out[i] = (uint32_t)acc;
        carry = acc >> 32;
    }
    out[7] &= 0x7FFFFFFFU;
}

void setUp(void)
{
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

static void test_m0_public_api_rejects_null_parameters(void)
{
    uint8_t scalar[32];
    uint8_t point[32];
    uint8_t result[32];

    fill_nonzero(scalar);
    fill_nonzero(point);

    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_m0_scalarmult(NULL, scalar, point));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_m0_scalarmult(result, NULL, point));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_m0_scalarmult(result, scalar, NULL));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_m0_public_key(NULL, result));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_m0_public_key(scalar, NULL));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_m0_shared_secret(NULL, scalar, point));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_m0_shared_secret(result, NULL, point));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_m0_shared_secret(result, scalar, NULL));
    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_INVALID_PARAM,
                          xy_x25519_m0_validate_public_key(NULL));
}

static void test_m0_validate_public_key_rejects_zero_only(void)
{
    uint8_t public_key[32] = {0};

    TEST_ASSERT_EQUAL_INT(XY_X25519_ERROR_WEAK_KEY, xy_x25519_m0_validate_public_key(public_key));

    public_key[31] = 0x80;
    TEST_ASSERT_EQUAL_INT(XY_X25519_SUCCESS, xy_x25519_m0_validate_public_key(public_key));
}

static void test_m0_field_basic_operations_match_legacy_smoke_cases(void)
{
    fe25519_m0 a, b, c;

    fe25519_setzero_m0(&a);
    a.limbs[0] = 2;
    fe25519_setzero_m0(&b);
    b.limbs[0] = 3;

    fe25519_add_m0(&c, &a, &b);
    TEST_ASSERT_EQUAL_UINT32(5U, c.limbs[0]);

    fe25519_sub_m0(&c, &c, &b);
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFEFU, c.limbs[0]);

    fe25519_setone_m0(&a);
    fe25519_mul_m0(&c, &a, &a);
    TEST_ASSERT_EQUAL_UINT32(1U, c.limbs[0]);
    TEST_ASSERT_EQUAL_UINT32(0U, c.limbs[1]);

    fe25519_square_m0(&c, &a);
    TEST_ASSERT_EQUAL_UINT32(1U, c.limbs[0]);
    TEST_ASSERT_EQUAL_UINT32(0U, c.limbs[1]);
}

static void test_m0_field_pack_unpack_round_trips_low_values(void)
{
    uint8_t input[32] = {0};
    uint8_t output[32] = {0};
    fe25519_m0 value;

    input[0] = 0x34;
    input[1] = 0x12;
    input[31] = 0x80;

    fe25519_unpack_m0(&value, input);
    TEST_ASSERT_EQUAL_UINT32(0x1234U, value.limbs[0]);
    TEST_ASSERT_BITS_LOW(0x80000000U, value.limbs[7]);

    fe25519_pack_m0(output, &value);
    TEST_ASSERT_EQUAL_UINT8(0x34U, output[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12U, output[1]);
    TEST_ASSERT_BITS_LOW(0x80U, output[31]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_m0_public_api_rejects_null_parameters);
    RUN_TEST(test_m0_validate_public_key_rejects_zero_only);
    RUN_TEST(test_m0_field_basic_operations_match_legacy_smoke_cases);
    RUN_TEST(test_m0_field_pack_unpack_round_trips_low_values);
    return UNITY_END();
}
