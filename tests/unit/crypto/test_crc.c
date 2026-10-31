/**
 * @file test_crc.c
 * @brief Unit tests for CRC core and common variants.
 *
 * Two classes of defect are pinned here, both found by computing the reveng catalogue check
 * value ("123456789") for every variant the component publishes.
 *
 * 1. `xy_crc_calc()` injected each byte with `byte << (width - 8)`. For a register narrower than
 *    a byte that count is negative, i.e. undefined behaviour, and every sub-byte variant returned
 *    a value that did not even match the parameters its own source declares - `xy_crc7_mmc()`
 *    answered 0x00 where CRC-7/MMC is 0x75, `xy_crc5_usb()` 0x0C where CRC-5/USB is 0x19. The
 *    negative shift is gone; these cases keep it from coming back.
 * 2. `xy_crc16_ccitt()` computed CRC-16/KERMIT (0x2189) while carrying the name of a family
 *    with four mutually different members, and `xy_crc8_bluetooth()` - declared in the header -
 *    was implemented as `..._hid`, so the declared name never linked. Both now answer to the name
 *    that describes what they compute.
 */

#include "xy_crc.h"
#include "xy_tiny_crypto.h"
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

static void test_catalogue_check_values_wide_widths(void)
{
    uint16_t n = (uint16_t)(sizeof(vector) - 1);

    /* 8-bit catalogue entries */
    TEST_ASSERT_EQUAL_UINT8(0xF4U, xy_crc8_normal(vector, n));       /* CRC-8/SMBUS */
    TEST_ASSERT_EQUAL_UINT8(0xA1U, xy_crc8_maxim(vector, n));        /* CRC-8/MAXIM-DOW */
    TEST_ASSERT_EQUAL_UINT8(0xD0U, xy_crc8_rohc(vector, n));         /* CRC-8/ROHC */
    TEST_ASSERT_EQUAL_UINT8(0xA1U, xy_crc8_itu(vector, n));          /* CRC-8/ITU */
    TEST_ASSERT_EQUAL_UINT8(0x4BU, xy_crc8_sae_j1850(vector, n));    /* CRC-8/SAE-J1850 */
    TEST_ASSERT_EQUAL_UINT8(0x25U, xy_crc8_wcdma(vector, n));        /* CRC-8/WCDMA */
    TEST_ASSERT_EQUAL_UINT8(0xDFU, xy_crc8_autosar(vector, n));      /* CRC-8/AUTOSAR */
    TEST_ASSERT_EQUAL_UINT8(0x26U, xy_crc8_bluetooth(vector, n));    /* CRC-8/BLUETOOTH */
    TEST_ASSERT_EQUAL_UINT8(0xDAU, xy_crc8_cdma2000(vector, n));     /* CRC-8/CDMA2000 */
    TEST_ASSERT_EQUAL_UINT8(0xBCU, xy_crc8_dvb_s2(vector, n));       /* CRC-8/DVB-S2 */
    TEST_ASSERT_EQUAL_UINT8(0x97U, xy_crc8_ebu(vector, n));          /* CRC-8/TECH-3250 */
    TEST_ASSERT_EQUAL_UINT8(0x37U, xy_crc8_gsm_a(vector, n));        /* CRC-8/GSM-A */
    TEST_ASSERT_EQUAL_UINT8(0x7EU, xy_crc8_i_code(vector, n));       /* CRC-8/I-CODE */
    TEST_ASSERT_EQUAL_UINT8(0x3EU, xy_crc8_opensafety(vector, n));   /* CRC-8/OPENSAFETY */
    TEST_ASSERT_EQUAL_UINT8(0x99U, xy_crc8_mifare_mad(vector, n));   /* CRC-8/MIFARE-MAD */

    /* 16-bit catalogue entries */
    TEST_ASSERT_EQUAL_UINT16(0x2189U, xy_crc16_kermit(vector, n));   /* CRC-16/KERMIT */
    TEST_ASSERT_EQUAL_UINT16(0xBB3DU, xy_crc16_ibm(vector, n));      /* CRC-16/ARC (IBM SDLC) */
    TEST_ASSERT_EQUAL_UINT16(0xBB3DU, xy_crc16_arc(vector, n));      /* CRC-16/ARC */
    TEST_ASSERT_EQUAL_UINT16(0x44C2U, xy_crc16_maxim(vector, n));    /* CRC-16/MAXIM-DOW */
    TEST_ASSERT_EQUAL_UINT16(0xB4C8U, xy_crc16_usb(vector, n));      /* CRC-16/USB */
    TEST_ASSERT_EQUAL_UINT16(0x906EU, xy_crc16_x25(vector, n));      /* CRC-16/X-25 */
    TEST_ASSERT_EQUAL_UINT16(0xEA82U, xy_crc16_dnp(vector, n));      /* CRC-16/DNP */

    /* 32-bit catalogue entries */
    TEST_ASSERT_EQUAL_UINT32(0xCBF43926UL, xy_crc32_normal(vector, n));  /* CRC-32/ISO-HDLC */
    TEST_ASSERT_EQUAL_UINT32(0xCBF43926UL, xy_crc32_iso_hdlc(vector, n));
    TEST_ASSERT_EQUAL_UINT32(0x340BC6D9UL, xy_crc32_jamcrc(vector, n)); /* CRC-32/JAMCRC */
    TEST_ASSERT_EQUAL_UINT32(0xE3069283UL, xy_crc32_c(vector, n));      /* CRC-32/ISCSI (Castagnoli) */
    TEST_ASSERT_EQUAL_UINT32(0x765E7680UL, xy_crc32_posix(vector, n));  /* CRC-32/CKSUM */
    TEST_ASSERT_EQUAL_UINT32(0x1697D06AUL, xy_crc32_autosar(vector, n)); /* CRC-32/AUTOSAR */

    /* 64-bit catalogue entries */
    TEST_ASSERT_EQUAL_UINT64(0x995DC9BBDF1939FAULL, xy_crc64_xz(vector, n));
    TEST_ASSERT_EQUAL_UINT64(0xE9C6D914C4B8D9CAULL, xy_crc64_redis(vector, n));
}

/*
 * Sub-byte registers. The first group's expected values are the reveng catalogue check values for
 * the entry each function is named after. The second group is pinned to the value the parameters
 * declared in `xy_crc.c` imply; the catalogue entry for those names could not be confirmed
 * offline (CRC-6/G-704 in particular), so the pin holds the current contract rather than claiming
 * standard conformance.
 */
static void test_catalogue_check_values_sub_byte_widths(void)
{
    uint16_t n = (uint16_t)(sizeof(vector) - 1);

    TEST_ASSERT_EQUAL_UINT8(0x04U, xy_crc3_gsm(vector, n));         /* CRC-3/GSM */
    TEST_ASSERT_EQUAL_UINT8(0x06U, xy_crc3_rohc(vector, n));        /* CRC-3/ROHC */
    TEST_ASSERT_EQUAL_UINT8(0x07U, xy_crc4_itu(vector, n));         /* CRC-4/G-704 */
    TEST_ASSERT_EQUAL_UINT8(0x0BU, xy_crc4_interlaken(vector, n));  /* CRC-4/INTERLAKEN */
    TEST_ASSERT_EQUAL_UINT8(0x00U, xy_crc5_epc(vector, n));         /* CRC-5/EPC-C1G2 */
    TEST_ASSERT_EQUAL_UINT8(0x07U, xy_crc5_itu(vector, n));         /* CRC-5/G-704 */
    TEST_ASSERT_EQUAL_UINT8(0x19U, xy_crc5_usb(vector, n));         /* CRC-5/USB */
    TEST_ASSERT_EQUAL_UINT8(0x0DU, xy_crc6_cdma2000a(vector, n));   /* CRC-6/CDMA2000-A */
    TEST_ASSERT_EQUAL_UINT8(0x26U, xy_crc6_darc(vector, n));        /* CRC-6/DARC */
    TEST_ASSERT_EQUAL_UINT8(0x75U, xy_crc7_mmc(vector, n));         /* CRC-7/MMC */
    TEST_ASSERT_EQUAL_UINT8(0x61U, xy_crc7_umts(vector, n));        /* CRC-7/UMTS */

    TEST_ASSERT_EQUAL_UINT8(0x02U, xy_crc2_g704(vector, n));
    TEST_ASSERT_EQUAL_UINT8(0x01U, xy_crc2_gsm(vector, n));
    TEST_ASSERT_EQUAL_UINT8(0x11U, xy_crc6_g704(vector, n));
    TEST_ASSERT_EQUAL_UINT8(0x13U, xy_crc6_gsm(vector, n));
    TEST_ASSERT_EQUAL_UINT8(0x75U, xy_crc7_darc(vector, n));
}

/* A byte-indexed table cannot represent a register narrower than a byte. The table path refuses to
 * build one and delegates instead of shifting by a negative count and returning garbage. */
static void test_narrow_widths_refuse_byte_table(void)
{
    xy_crc_cfg_t cfg = { .width = 5, .polynomial = 0x05, .init_value = 0x1f, .xor_out = 0x1f, .ref_in = 1, .ref_out = 1 };
    xy_crc_opt_t table = { .method = XY_CRC_METHOD_TABLE, .use_dma = 0 };
    uint64_t table_entries[256];
    uint64_t expected = xy_crc_calc(&cfg, vector, sizeof(vector) - 1);

    TEST_ASSERT_EQUAL_INT(-1, xy_crc_make_table(&cfg, table_entries));
    TEST_ASSERT_EQUAL_UINT64(0x19U, expected);
    TEST_ASSERT_EQUAL_UINT64(expected, xy_crc_calc_table(&cfg, table_entries, vector, sizeof(vector) - 1));
    TEST_ASSERT_EQUAL_UINT64(expected, xy_crc_calc_ex(&cfg, vector, sizeof(vector) - 1, &table));
}

/* `xy_crc32()` and `xy_crc32_update()` are declared by `xy_tiny_crypto.h`, documented in the
 * component README and called by `components/crypto/example.c`. Until this slice neither had a
 * definition anywhere in the tree, so every caller linked against nothing. The contract is the one
 * the name implies: CRC-32/ISO-HDLC with the zlib calling convention. */
static void test_tiny_crc32_contract(void)
{
    uint8_t empty[1] = { 0 };

    TEST_ASSERT_EQUAL_UINT32(0xCBF43926UL, xy_crc32(vector, sizeof(vector) - 1));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_crc32(empty, 0));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_crc32(NULL, sizeof(vector) - 1));

    /* The incremental form takes what the previous call returned: any other convention makes the
     * second chunk disagree with the one-shot value. */
    TEST_ASSERT_EQUAL_UINT32(0xCBF43926UL,
                            xy_crc32_update(xy_crc32(vector, 4), vector + 4, sizeof(vector) - 5));
    TEST_ASSERT_EQUAL_UINT32(0xCBF43926UL,
                            xy_crc32_update(xy_crc32_update(0U, vector, 1), vector + 1, sizeof(vector) - 2));

    /* Mid-chain values must equal the one-shot CRC of the same prefix. */
    TEST_ASSERT_EQUAL_UINT32(xy_crc32(vector, 4), xy_crc32_update(0U, vector, 4));
}

/* The table cache was built once, from whichever configuration asked first, and reused for every
 * later caller - so a table request for a second polynomial returned a plausible number computed
 * with the first one's polynomial. It also marked the cache ready when `xy_crc_make_table()` had
 * refused the width, putting 256 zero entries into service. */
static void test_table_cache_is_keyed_by_configuration(void)
{
    xy_crc_cfg_t ccitt = { .width = 16, .polynomial = 0x1021, .init_value = 0xffff, .xor_out = 0, .ref_in = 0, .ref_out = 0 };
    xy_crc_cfg_t ibm = { .width = 16, .polynomial = 0x8005, .init_value = 0x0000, .xor_out = 0, .ref_in = 1, .ref_out = 1 };
    xy_crc_opt_t table = { .method = XY_CRC_METHOD_TABLE, .use_dma = 0 };
    uint16_t n = (uint16_t)(sizeof(vector) - 1);

    TEST_ASSERT_EQUAL_UINT64(xy_crc_calc(&ccitt, vector, n),
                             xy_crc_calc_ex(&ccitt, vector, n, &table));
    TEST_ASSERT_EQUAL_UINT64(xy_crc_calc(&ibm, vector, n),
                             xy_crc_calc_ex(&ibm, vector, n, &table));
    TEST_ASSERT_EQUAL_UINT64(xy_crc_calc(&ccitt, vector, n),
                             xy_crc_calc_ex(&ccitt, vector, n, &table));
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
    RUN_TEST(test_catalogue_check_values_wide_widths);
    RUN_TEST(test_catalogue_check_values_sub_byte_widths);
    RUN_TEST(test_narrow_widths_refuse_byte_table);
    RUN_TEST(test_tiny_crc32_contract);
    RUN_TEST(test_table_cache_is_keyed_by_configuration);
    RUN_TEST(test_extended_modes_fallback);
    return UNITY_END();
}
