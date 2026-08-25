#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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

static void test_required_chinese_ui_glyphs_use_distinct_nonblank_replacement_bitmaps(void)
{
    static const uint16_t required_ui_codepoints[] = {
        0x4E0A, 0x4E0B, 0x5DE6, 0x53F3, 0x786E, 0x8BA4, 0x53D6, 0x6D88,
        0x8BBE, 0x7F6E, 0x8FD8, 0x56DE, 0x83DC, 0x5355, 0x5B9A,
    };

    for (uint16_t i = 0U; i < sizeof(required_ui_codepoints) / sizeof(required_ui_codepoints[0]);
         ++i) {
        const uint8_t *glyph = xy_font_chinese_16x16_get_char(required_ui_codepoints[i]);
        bool any_pixel = false;

        TEST_ASSERT_NOT_NULL(glyph);
        for (uint8_t byte = 0U; byte < 32U; ++byte) {
            any_pixel = any_pixel || glyph[byte] != 0U;
        }
        TEST_ASSERT_TRUE(any_pixel);

        for (uint16_t previous = 0U; previous < i; ++previous) {
            const uint8_t *previous_glyph =
                xy_font_chinese_16x16_get_char(required_ui_codepoints[previous]);
            TEST_ASSERT_TRUE(memcmp(glyph, previous_glyph, 32U) != 0);
        }
    }
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
    RUN_TEST(test_required_chinese_ui_glyphs_use_distinct_nonblank_replacement_bitmaps);
    RUN_TEST(test_chinese_font_lookup_and_measure_boundaries);
    return UNITY_END();
}
