/**
 * @file test_xy_bcd.c
 * @brief BCD digits and the RTC year spelling, checked against values that can be read by eye
 *
 * The year cases are the three backends' defects, written as expectations: a full year in and out
 * whichever format is asked for, a two-digit register underneath, and a refusal rather than a
 * truncation for anything the register cannot hold.
 */

#include "unity.h"
#include "xy_bcd.h"

void setUp(void) {}
void tearDown(void) {}

static void test_bcd_eight_bit_round_trips(void)
{
    uint8_t out = 0xFF;
    int value;

    for (value = 0; value <= 99; ++value) {
        uint8_t encoded = xy_bcd_encode8((uint8_t)value);

        TEST_ASSERT_TRUE(xy_bcd_decode8(encoded, &out));
        TEST_ASSERT_EQUAL_UINT8((uint8_t)value, out);
    }
}

static void test_bcd_eight_bit_known_values(void)
{
    uint8_t out = 0;

    TEST_ASSERT_EQUAL_HEX8(0x00, xy_bcd_encode8(0));
    TEST_ASSERT_EQUAL_HEX8(0x09, xy_bcd_encode8(9));
    TEST_ASSERT_EQUAL_HEX8(0x10, xy_bcd_encode8(10));
    TEST_ASSERT_EQUAL_HEX8(0x26, xy_bcd_encode8(26));
    TEST_ASSERT_EQUAL_HEX8(0x59, xy_bcd_encode8(59));
    TEST_ASSERT_EQUAL_HEX8(0x99, xy_bcd_encode8(99));

    TEST_ASSERT_TRUE(xy_bcd_decode8(0x26, &out));
    TEST_ASSERT_EQUAL_UINT8(26, out);
    TEST_ASSERT_TRUE(xy_bcd_decode8(0x59, &out));
    TEST_ASSERT_EQUAL_UINT8(59, out);
}

static void test_a_nibble_above_nine_is_not_a_number(void)
{
    uint8_t out = 0xAA;

    TEST_ASSERT_FALSE(xy_bcd_decode8(0x1A, &out));
    TEST_ASSERT_FALSE(xy_bcd_decode8(0xA1, &out));
    TEST_ASSERT_FALSE(xy_bcd_decode8(0xFF, &out));
    /* The caller's value is left alone rather than replaced with a plausible number. */
    TEST_ASSERT_EQUAL_HEX8(0xAA, out);
}

static void test_bcd_sixteen_bit_round_trips(void)
{
    uint16_t out = 0;
    int value;

    for (value = 0; value <= 9999; value += 7) {
        TEST_ASSERT_TRUE(xy_bcd_decode16(xy_bcd_encode16((uint16_t)value), &out));
        TEST_ASSERT_EQUAL_UINT16((uint16_t)value, out);
    }
}

static void test_bcd_sixteen_bit_known_values(void)
{
    uint16_t out = 0;

    TEST_ASSERT_EQUAL_HEX16(0x2026, xy_bcd_encode16(2026));
    TEST_ASSERT_EQUAL_HEX16(0x0000, xy_bcd_encode16(0));
    TEST_ASSERT_EQUAL_HEX16(0x9999, xy_bcd_encode16(9999));
    TEST_ASSERT_TRUE(xy_bcd_decode16(0x2026, &out));
    TEST_ASSERT_EQUAL_UINT16(2026, out);
    TEST_ASSERT_FALSE(xy_bcd_decode16(0x2A26, &out));
}

/* The contract: a full year out of a two-digit register, whichever format it was read with. */
static void test_a_full_year_comes_back_from_two_digits(void)
{
    TEST_ASSERT_EQUAL_INT(2026, xy_rtc_year_decode(26, 0));
    TEST_ASSERT_EQUAL_INT(2026, xy_rtc_year_decode(0x26, 1));
    TEST_ASSERT_EQUAL_INT(2000, xy_rtc_year_decode(0, 0));
    TEST_ASSERT_EQUAL_INT(2000, xy_rtc_year_decode(0x00, 1));
    TEST_ASSERT_EQUAL_INT(2099, xy_rtc_year_decode(0x99, 1));
}

/* A register holding something that is not a date must not become a plausible year. */
static void test_an_invalid_register_value_is_refused(void)
{
    TEST_ASSERT_EQUAL_INT(-1, xy_rtc_year_decode(0x1A, 1));
    TEST_ASSERT_EQUAL_INT(-1, xy_rtc_year_decode(100, 0));
    TEST_ASSERT_EQUAL_INT(-1, xy_rtc_year_decode(200, 0));
}

static void test_writing_a_year_produces_the_register_spelling(void)
{
    TEST_ASSERT_EQUAL_INT(26, xy_rtc_year_encode(2026, 0));
    TEST_ASSERT_EQUAL_INT(0x26, xy_rtc_year_encode(2026, 1));
    TEST_ASSERT_EQUAL_INT(0, xy_rtc_year_encode(2000, 0));
    TEST_ASSERT_EQUAL_INT(0x99, xy_rtc_year_encode(2099, 1));
}

/* The defect U5 had: 2026 handed to a two-digit encoder, truncated into nonsense rather than refused. */
static void test_a_year_the_register_cannot_hold_is_refused(void)
{
    TEST_ASSERT_EQUAL_INT(-1, xy_rtc_year_encode(1999, 1));
    TEST_ASSERT_EQUAL_INT(-1, xy_rtc_year_encode(2100, 1));
    TEST_ASSERT_EQUAL_INT(-1, xy_rtc_year_encode(0, 0));
    TEST_ASSERT_EQUAL_INT(-1, xy_rtc_year_encode(26, 0));
}

static void test_encode_and_decode_agree(void)
{
    uint16_t year;

    for (year = XY_BCD_CENTURY_BASE; year < XY_BCD_CENTURY_BASE + 100U; ++year) {
        int bcd;

        for (bcd = 0; bcd <= 1; ++bcd) {
            int encoded = xy_rtc_year_encode(year, bcd);

            TEST_ASSERT_TRUE(encoded >= 0);
            TEST_ASSERT_EQUAL_INT((int)year,
                                  xy_rtc_year_decode((uint8_t)encoded, bcd));
        }
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bcd_eight_bit_round_trips);
    RUN_TEST(test_bcd_eight_bit_known_values);
    RUN_TEST(test_a_nibble_above_nine_is_not_a_number);
    RUN_TEST(test_bcd_sixteen_bit_round_trips);
    RUN_TEST(test_bcd_sixteen_bit_known_values);
    RUN_TEST(test_a_full_year_comes_back_from_two_digits);
    RUN_TEST(test_an_invalid_register_value_is_refused);
    RUN_TEST(test_writing_a_year_produces_the_register_spelling);
    RUN_TEST(test_a_year_the_register_cannot_hold_is_refused);
    RUN_TEST(test_encode_and_decode_agree);
    return UNITY_END();
}
