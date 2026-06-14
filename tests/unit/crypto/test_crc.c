/**
 * @file test_crc.c
 * @brief Unit tests for CRC core and common variants.
 */

#include "xy_crc.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static uint8_t vector[] = "123456789";

static void test_invalid_params(void)
{
    xy_crc_cfg_t cfg = { .width = 16, .polynomial = 0x1021, .init_value = 0xffff, .xor_out = 0, .ref_in = 0, .ref_out = 0 };
    xy_crc_opt_t opt = { .method = XY_CRC_METHOD_SW, .use_dma = 0 };
    uint64_t table[256];

    assert(xy_crc_calc(NULL, vector, sizeof(vector) - 1) == 0);
    assert(xy_crc_calc(&cfg, NULL, sizeof(vector) - 1) == 0);
    assert(xy_crc_calc_table(NULL, table, vector, sizeof(vector) - 1) == 0);
    assert(xy_crc_calc_table(&cfg, NULL, vector, sizeof(vector) - 1) == 0);
    assert(xy_crc_make_table(NULL, table) == -1);
    assert(xy_crc_make_table(&cfg, NULL) == -1);
    assert(xy_crc_calc_ex(&cfg, vector, sizeof(vector) - 1, NULL) == 0);

    cfg.width = 1;
    assert(xy_crc_calc(&cfg, vector, sizeof(vector) - 1) == 0);
    assert(xy_crc_calc_ex(&cfg, vector, sizeof(vector) - 1, &opt) == 0);
}

static void test_crc_known_vectors(void)
{
    assert(xy_crc16_modbus(vector, sizeof(vector) - 1) == 0x4B37U);
    assert(xy_crc16_ccitt_false(vector, sizeof(vector) - 1) == 0x29B1U);
    assert(xy_crc16_xmodem(vector, sizeof(vector) - 1) == 0x31C3U);
    assert(xy_crc32_mpeg2(vector, sizeof(vector) - 1) == 0x0376E6E7UL);
    assert(xy_crc32_bzip2(vector, sizeof(vector) - 1) == 0xFC891918UL);
    assert(xy_crc64_ecma(vector, sizeof(vector) - 1) == 0x6C40DF5F0B497347ULL);
}

static void test_extended_modes_fallback(void)
{
    xy_crc_cfg_t cfg = { .width = 16, .polynomial = 0x1021, .init_value = 0xffff, .xor_out = 0, .ref_in = 0, .ref_out = 0 };
    xy_crc_opt_t sw = { .method = XY_CRC_METHOD_SW, .use_dma = 0 };
    xy_crc_opt_t table = { .method = XY_CRC_METHOD_TABLE, .use_dma = 0 };
    xy_crc_opt_t hw = { .method = XY_CRC_METHOD_HW, .use_dma = 1 };
    xy_crc_opt_t unknown = { .method = (xy_crc_method_t)99, .use_dma = 0 };
    uint64_t expected = xy_crc_calc(&cfg, vector, sizeof(vector) - 1);

    assert(expected == 0x29B1U);
    assert(xy_crc_calc_ex(&cfg, vector, sizeof(vector) - 1, &sw) == expected);
    assert(xy_crc_calc_ex(&cfg, vector, sizeof(vector) - 1, &table) == expected);
    assert(xy_crc_calc_ex(&cfg, vector, sizeof(vector) - 1, &hw) == expected);
    assert(xy_crc_calc_ex(&cfg, vector, sizeof(vector) - 1, &unknown) == expected);
}

int main(void)
{
    test_invalid_params();
    test_crc_known_vectors();
    test_extended_modes_fallback();
    return 0;
}
