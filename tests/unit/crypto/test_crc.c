/**
 * @file test_crc.c
 * @brief Unit tests for CRC core and common variants.
 */

#include "xy_crc.h"
#include "unity.h"

#include <stdint.h>
#include <string.h>

static uint8_t vector[] = "123456789";

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_invalid_params(void)
{
    xy_crc_cfg_t cfg = { .width = 16, .polynomial = 0x1021, .init_value = 0xffff, .xor_out = 0, .ref_in = 0, .ref_out = 0 };
    xy_crc_opt_t opt = { .method = XY_CRC_METHOD_SW, .use_dma = 0 };
    uint64_t table[256];

    TEST_ASSERT_EQUAL_UINT64(0U, xy_crc_calc(NULL, vector, sizeof(vector) - 1));
    TEST_ASSERT_EQUAL_UINT64(0U, xy_crc_calc(&cfg, NULL, sizeof(vector) - 1));
    TEST_ASSERT_EQUAL_UINT64(0U, xy_crc_calc_table(NULL, table, vector, sizeof(vector) - 1));
    TEST_ASSERT_EQUAL_UINT64(0U, xy_crc_calc_table(&cfg, NULL, vector, sizeof(vector) - 1));
    TEST_ASSERT_EQUAL_INT(-1, xy_crc_make_table(NULL, table));
    TEST_ASSERT_EQUAL_INT(-1, xy_crc_make_table(&cfg, NULL));
    TEST_ASSERT_EQUAL_UINT64(0U, xy_crc_calc_ex(&cfg, vector, sizeof(vector) - 1, NULL));

    cfg.width = 1;
    TEST_ASSERT_EQUAL_UINT64(0U, xy_crc_calc(&cfg, vector, sizeof(vector) - 1));
    TEST_ASSERT_EQUAL_UINT64(0U, xy_crc_calc_ex(&cfg, vector, sizeof(vector) - 1, &opt));
}

static void test_crc_known_vectors(void)
{
    TEST_ASSERT_EQUAL_UINT16(0x4B37U, xy_crc16_modbus(vector, sizeof(vector) - 1));
    TEST_ASSERT_EQUAL_UINT16(0x29B1U, xy_crc16_ccitt_false(vector, sizeof(vector) - 1));
    TEST_ASSERT_EQUAL_UINT16(0x31C3U, xy_crc16_xmodem(vector, sizeof(vector) - 1));
    TEST_ASSERT_EQUAL_UINT32(0x0376E6E7UL, xy_crc32_mpeg2(vector, sizeof(vector) - 1));
    TEST_ASSERT_EQUAL_UINT32(0xFC891918UL, xy_crc32_bzip2(vector, sizeof(vector) - 1));
    TEST_ASSERT_EQUAL_UINT64(0x6C40DF5F0B497347ULL, xy_crc64_ecma(vector, sizeof(vector) - 1));
}

static void test_extended_modes_fallback(void)
{
    xy_crc_cfg_t cfg = { .width = 16, .polynomial = 0x1021, .init_value = 0xffff, .xor_out = 0, .ref_in = 0, .ref_out = 0 };
    xy_crc_opt_t sw = { .method = XY_CRC_METHOD_SW, .use_dma = 0 };
    xy_crc_opt_t table = { .method = XY_CRC_METHOD_TABLE, .use_dma = 0 };
    xy_crc_opt_t hw = { .method = XY_CRC_METHOD_HW, .use_dma = 1 };
    xy_crc_opt_t unknown = { .method = (xy_crc_method_t)99, .use_dma = 0 };
    uint64_t expected = xy_crc_calc(&cfg, vector, sizeof(vector) - 1);

    TEST_ASSERT_EQUAL_UINT64(0x29B1U, expected);
    TEST_ASSERT_EQUAL_UINT64(expected, xy_crc_calc_ex(&cfg, vector, sizeof(vector) - 1, &sw));
    TEST_ASSERT_EQUAL_UINT64(expected, xy_crc_calc_ex(&cfg, vector, sizeof(vector) - 1, &table));
    TEST_ASSERT_EQUAL_UINT64(expected, xy_crc_calc_ex(&cfg, vector, sizeof(vector) - 1, &hw));
    TEST_ASSERT_EQUAL_UINT64(expected, xy_crc_calc_ex(&cfg, vector, sizeof(vector) - 1, &unknown));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_invalid_params);
    RUN_TEST(test_crc_known_vectors);
    RUN_TEST(test_extended_modes_fallback);
    return UNITY_END();
}
