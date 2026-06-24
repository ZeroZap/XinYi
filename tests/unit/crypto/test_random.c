/**
 * @file test_random.c
 * @brief Unit tests for simple RNG API validation and output shape.
 */

#include "xy_rng.h"
#include "unity.h"

#include <stdint.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static int any_nonzero(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (buf[i] != 0U) {
            return 1;
        }
    }
    return 0;
}

static void test_random_bytes_validation(void)
{
    uint8_t buf[32];

    memset(buf, 0xA5, sizeof(buf));
    TEST_ASSERT_TRUE(xy_random_bytes(NULL, sizeof(buf)) == XY_RNG_INVALID_PARAM);

    TEST_ASSERT_TRUE(xy_random_bytes(buf, 0) == XY_RNG_SUCCESS);
    for (size_t i = 0; i < sizeof(buf); i++) {
        TEST_ASSERT_TRUE(buf[i] == 0xA5U);
    }
}

static void test_random_bytes_output(void)
{
    uint8_t buf[64];

    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_TRUE(xy_random_bytes(buf, sizeof(buf)) == XY_RNG_SUCCESS);
    TEST_ASSERT_TRUE(any_nonzero(buf, sizeof(buf)));

    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_TRUE(xy_random_bytes(buf, 7) == XY_RNG_SUCCESS);
    TEST_ASSERT_TRUE(any_nonzero(buf, 7));
    for (size_t i = 7; i < sizeof(buf); i++) {
        TEST_ASSERT_TRUE(buf[i] == 0U);
    }
}

static void test_random_uint32_runs(void)
{
    uint32_t a = xy_random_uint32();
    uint32_t b = xy_random_uint32();

    /* The API contract is availability, not uniqueness; just ensure calls run. */
    (void)a;
    (void)b;
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_random_bytes_validation);
    RUN_TEST(test_random_bytes_output);
    RUN_TEST(test_random_uint32_runs);
    return UNITY_END();
}
