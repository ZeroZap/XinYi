#include <stdint.h>

#include "unity.h"
#include "xy_font_8x16.h"
#include "xy_font_16x24.h"
#include "xy_font_chinese_16x16.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_ascii_font_handles_and_boundaries(void)
{
    const xy_font_8x16_t *font8 = xy_font_8x16_get();
    TEST_ASSERT_NOT_NULL(font8);
    TEST_ASSERT_EQUAL_UINT8(FONT_8X16_WIDTH, font8->width);
    TEST_ASSERT_EQUAL_UINT8(FONT_8X16_HEIGHT, font8->height);
    TEST_ASSERT_EQUAL_UINT8(0x20U, font8->first_char);
    TEST_ASSERT_EQUAL_UINT8(FONT_8X16_CHAR_COUNT, font8->char_count);
    TEST_ASSERT_NOT_NULL(font8->data);

    TEST_ASSERT_NOT_NULL(xy_font_8x16_get_char(' '));
    TEST_ASSERT_NOT_NULL(xy_font_8x16_get_char('A'));
    TEST_ASSERT_NOT_NULL(xy_font_8x16_get_char('~'));
    TEST_ASSERT_NULL(xy_font_8x16_get_char(0x1FU));
    TEST_ASSERT_NULL(xy_font_8x16_get_char(0x7FU));
    TEST_ASSERT_EQUAL_UINT16(0U, xy_font_8x16_measure(NULL));
    TEST_ASSERT_EQUAL_UINT16(0U, xy_font_8x16_measure(""));
    TEST_ASSERT_EQUAL_UINT16(24U, xy_font_8x16_measure("A~ "));
    TEST_ASSERT_EQUAL_UINT16(16U, xy_font_8x16_measure("A\nB"));

    const xy_font_16x24_t *font16 = xy_font_16x24_get();
    TEST_ASSERT_NOT_NULL(font16);
    TEST_ASSERT_EQUAL_UINT8(FONT_16X24_WIDTH, font16->width);
    TEST_ASSERT_EQUAL_UINT8(FONT_16X24_HEIGHT, font16->height);
    TEST_ASSERT_EQUAL_UINT8(0x20U, font16->first_char);
    TEST_ASSERT_EQUAL_UINT8(FONT_16X24_CHAR_COUNT, font16->char_count);
    TEST_ASSERT_NOT_NULL(font16->data);

    TEST_ASSERT_NOT_NULL(xy_font_16x24_get_char(' '));
    TEST_ASSERT_NOT_NULL(xy_font_16x24_get_char('A'));
    TEST_ASSERT_NOT_NULL(xy_font_16x24_get_char('~'));
    TEST_ASSERT_NULL(xy_font_16x24_get_char(0x1FU));
    TEST_ASSERT_NULL(xy_font_16x24_get_char(0x7FU));
    TEST_ASSERT_EQUAL_UINT16(0U, xy_font_16x24_measure(NULL));
    TEST_ASSERT_EQUAL_UINT16(0U, xy_font_16x24_measure(""));
    TEST_ASSERT_EQUAL_UINT16(48U, xy_font_16x24_measure("A~ "));
    TEST_ASSERT_EQUAL_UINT16(32U, xy_font_16x24_measure("A\nB"));
}

static void test_chinese_font_lookup_and_measure_boundaries(void)
{
    const xy_font_chinese_t *font = xy_font_chinese_16x16_get();
    TEST_ASSERT_NOT_NULL(font);
    TEST_ASSERT_EQUAL_UINT8(FONT_CHINESE_WIDTH, font->width);
    TEST_ASSERT_EQUAL_UINT8(FONT_CHINESE_HEIGHT, font->height);
    TEST_ASSERT_EQUAL_UINT16(FONT_CHINESE_CHAR_COUNT, font->char_count);
    TEST_ASSERT_NOT_NULL(font->data);

    const xy_chinese_char_t *chars = xy_font_chinese_16x16_get_chars();
    TEST_ASSERT_NOT_NULL(chars);
    TEST_ASSERT_NOT_NULL(xy_font_chinese_16x16_get_char(chars[0].unicode));
    TEST_ASSERT_NULL(xy_font_chinese_16x16_get_char(0xFFFFU));
    TEST_ASSERT_NULL(xy_font_chinese_16x16_get_gb2312(0xC9CFU));

    TEST_ASSERT_EQUAL_UINT16(0U, xy_font_chinese_16x16_measure(NULL));
    TEST_ASSERT_EQUAL_UINT16(0U, xy_font_chinese_16x16_measure(""));
    TEST_ASSERT_EQUAL_UINT16(24U, xy_font_chinese_16x16_measure("A中"));
    TEST_ASSERT_EQUAL_UINT16(32U, xy_font_chinese_16x16_measure("中文"));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ascii_font_handles_and_boundaries);
    RUN_TEST(test_chinese_font_lookup_and_measure_boundaries);
    return UNITY_END();
}
