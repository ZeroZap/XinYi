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

static uint16_t count_codepoint(const xy_chinese_char_t *chars, uint16_t count, uint16_t unicode)
{
    uint16_t matches = 0;

    for (uint16_t i = 0; i < count; i++) {
        if (chars[i].unicode == unicode) {
            matches++;
        }
    }

    return matches;
}

static uint16_t count_duplicate_entries(const xy_chinese_char_t *chars, uint16_t count)
{
    uint16_t duplicates = 0;

    for (uint16_t i = 0; i < count; i++) {
        for (uint16_t j = 0; j < i; j++) {
            if (chars[i].unicode == chars[j].unicode) {
                duplicates++;
                break;
            }
        }
    }

    return duplicates;
}

static uint16_t count_entries_using_bitmap(const xy_chinese_char_t *chars, uint16_t count,
                                           const uint8_t *bitmap)
{
    uint16_t matches = 0;

    for (uint16_t i = 0; i < count; i++) {
        if (chars[i].data == bitmap) {
            matches++;
        }
    }

    return matches;
}

static void test_manifest_ascii_tables_match_public_contracts(void)
{
    const xy_font_8x16_t *font8 = xy_font_8x16_get();
    TEST_ASSERT_NOT_NULL(font8);
    TEST_ASSERT_EQUAL_UINT8(8U, font8->width);
    TEST_ASSERT_EQUAL_UINT8(16U, font8->height);
    TEST_ASSERT_EQUAL_UINT8(0x20U, font8->first_char);
    TEST_ASSERT_EQUAL_UINT8(95U, font8->char_count);
    TEST_ASSERT_EQUAL_UINT(16U, FONT_8X16_HEIGHT);
    TEST_ASSERT_EQUAL_PTR(xy_font_8x16_get_char(' '), xy_font_8x16_get_char(0x20U));
    TEST_ASSERT_EQUAL_PTR(xy_font_8x16_get_char('~'), xy_font_8x16_get_char(0x7EU));
    TEST_ASSERT_NULL(xy_font_8x16_get_char(0x1FU));
    TEST_ASSERT_NULL(xy_font_8x16_get_char(0x7FU));

    const xy_font_16x24_t *font16 = xy_font_16x24_get();
    TEST_ASSERT_NOT_NULL(font16);
    TEST_ASSERT_EQUAL_UINT8(16U, font16->width);
    TEST_ASSERT_EQUAL_UINT8(24U, font16->height);
    TEST_ASSERT_EQUAL_UINT8(0x20U, font16->first_char);
    TEST_ASSERT_EQUAL_UINT8(95U, font16->char_count);
    TEST_ASSERT_EQUAL_UINT(48U, FONT_16X24_HEIGHT * 2U);
    TEST_ASSERT_EQUAL_PTR(xy_font_16x24_get_char(' '), xy_font_16x24_get_char(0x20U));
    TEST_ASSERT_EQUAL_PTR(xy_font_16x24_get_char('~'), xy_font_16x24_get_char(0x7EU));
    TEST_ASSERT_NULL(xy_font_16x24_get_char(0x1FU));
    TEST_ASSERT_NULL(xy_font_16x24_get_char(0x7FU));
}

static void test_manifest_chinese_table_inventory_is_explicit(void)
{
    const xy_font_chinese_t *font = xy_font_chinese_16x16_get();
    TEST_ASSERT_NOT_NULL(font);
    TEST_ASSERT_EQUAL_UINT8(16U, font->width);
    TEST_ASSERT_EQUAL_UINT8(16U, font->height);
    TEST_ASSERT_EQUAL_UINT16(168U, font->char_count);

    const xy_chinese_char_t *chars = xy_font_chinese_16x16_get_chars();
    TEST_ASSERT_NOT_NULL(chars);
    for (uint16_t i = 0; i < font->char_count; i++) {
        TEST_ASSERT_NOT_NULL(chars[i].data);
        TEST_ASSERT_NOT_EQUAL(0U, chars[i].unicode);
    }

    static const uint16_t required_ui_codepoints[] = {
        0x4E0A, 0x4E0B, 0x5DE6, 0x53F3, 0x786E, 0x8BA4, 0x53D6, 0x6D88,
        0x8BBE, 0x7F6E, 0x8FD8, 0x56DE, 0x83DC, 0x5355, 0x5B9A,
    };

    for (uint16_t i = 0; i < sizeof(required_ui_codepoints) / sizeof(required_ui_codepoints[0]);
         i++) {
        TEST_ASSERT_NOT_NULL(xy_font_chinese_16x16_get_char(required_ui_codepoints[i]));
    }

    const uint16_t duplicate_entries = count_duplicate_entries(chars, font->char_count);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(1U, duplicate_entries);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(2U, count_codepoint(chars, font->char_count, 0x786EU));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(2U, count_codepoint(chars, font->char_count, 0x53D6U));

    const uint8_t *placeholder_bitmap = xy_font_chinese_16x16_get_char(0x4E00U);
    TEST_ASSERT_NOT_NULL(placeholder_bitmap);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(
        100U, count_entries_using_bitmap(chars, font->char_count, placeholder_bitmap));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_manifest_ascii_tables_match_public_contracts);
    RUN_TEST(test_manifest_chinese_table_inventory_is_explicit);
    return UNITY_END();
}
