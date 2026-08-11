#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "xy_font.h"

static uint32_t g_fake_tick;

uint32_t xy_os_tick_get(void)
{
    return ++g_fake_tick;
}

void xy_log_char(char ch)
{
    (void)ch;
}

void setUp(void)
{
    g_fake_tick = 0U;
}

void tearDown(void)
{
}

static void make_test_font(xy_font_t *font)
{
    static const uint8_t glyph_a_data[8] = {
        0x80, 0x40, 0x20, 0x10,
        0x08, 0x04, 0x02, 0x01,
    };
    static const uint8_t glyph_b_data[8] = {
        0xF0, 0x90, 0x90, 0xF0,
        0x90, 0x90, 0x90, 0x00,
    };
    static const xy_glyph_t glyphs[] = {
        {glyph_a_data, 2U, 4U, 3, 0, 0},
        {glyph_b_data, 2U, 4U, 4, 0, 0},
    };

    memset(font, 0, sizeof(*font));
    font->name = "test";
    font->type = XY_FONT_TYPE_BITMAP;
    font->style.size = 8U;
    font->glyphs = glyphs;
    font->glyph_count = 2U;
    font->first_char = 'A';
    font->last_char = 'B';
    font->line_height = 5U;
    font->baseline = 4U;
    font->initialized = true;
}

static void test_font_engine_glyph_measure_and_draw_contracts(void)
{
    xy_font_t font;
    xy_text_metrics_t metrics;
    uint16_t framebuffer[8 * 8];

    make_test_font(&font);
    memset(framebuffer, 0, sizeof(framebuffer));

    TEST_ASSERT_EQUAL_INT(0, xy_font_init(&font, "runtime", XY_FONT_TYPE_BITMAP));
    TEST_ASSERT_EQUAL_STRING("runtime", font.name);
    TEST_ASSERT_EQUAL_INT(XY_FONT_TYPE_BITMAP, font.type);
    TEST_ASSERT_EQUAL_UINT8(12U, font.style.size);

    make_test_font(&font);
    TEST_ASSERT_NOT_NULL(xy_font_get_glyph(&font, 'A'));
    TEST_ASSERT_NOT_NULL(xy_font_get_glyph(&font, 'B'));
    TEST_ASSERT_NULL(xy_font_get_glyph(&font, '@'));
    TEST_ASSERT_NULL(xy_font_get_glyph(&font, 'C'));
    TEST_ASSERT_NULL(xy_font_get_glyph(NULL, 'A'));

    TEST_ASSERT_EQUAL_INT(0, xy_font_measure_text(&font, "AB\nZ", &metrics));
    TEST_ASSERT_EQUAL_UINT16(7U, metrics.width);
    TEST_ASSERT_EQUAL_UINT16(16U, metrics.height);
    TEST_ASSERT_EQUAL_UINT16(2U, metrics.lines);
    TEST_ASSERT_EQUAL_UINT16(7U, xy_font_get_text_width(&font, "AB"));
    TEST_ASSERT_EQUAL_UINT16(8U, xy_font_get_text_height(&font));
    TEST_ASSERT_EQUAL_UINT8(5U, xy_font_get_line_height(&font));
    TEST_ASSERT_EQUAL_INT(-1, xy_font_measure_text(NULL, "A", &metrics));
    TEST_ASSERT_EQUAL_UINT16(0U, xy_font_get_text_width(NULL, "A"));
    TEST_ASSERT_EQUAL_UINT16(0U, xy_font_get_text_height(NULL));
    TEST_ASSERT_EQUAL_UINT8(0U, xy_font_get_line_height(NULL));

    TEST_ASSERT_EQUAL_INT(0, xy_font_draw_char(&font, 'A', 1, 1, 0x55AAU, framebuffer, 8U, 8U));
    TEST_ASSERT_EQUAL_UINT16(0x55AAU, framebuffer[1U * 8U + 1U]);
    TEST_ASSERT_EQUAL_UINT16(0U, framebuffer[1U * 8U + 2U]);
    TEST_ASSERT_EQUAL_INT(-1, xy_font_draw_char(&font, 'Z', 0, 0, 0x1234U, framebuffer, 8U, 8U));
    TEST_ASSERT_EQUAL_INT(-1, xy_font_draw_char(NULL, 'A', 0, 0, 0x1234U, framebuffer, 8U, 8U));
    TEST_ASSERT_EQUAL_INT(-1, xy_font_draw_char(&font, 'A', 0, 0, 0x1234U, NULL, 8U, 8U));

    memset(framebuffer, 0, sizeof(framebuffer));
    TEST_ASSERT_EQUAL_INT(0, xy_font_draw_string(&font, "AB", 0, 0, 0x00FFU, framebuffer, 8U, 8U));
    TEST_ASSERT_EQUAL_UINT16(0x00FFU, framebuffer[0]);
    TEST_ASSERT_EQUAL_UINT16(0x00FFU, framebuffer[3]);
    TEST_ASSERT_EQUAL_INT(-1, xy_font_draw_string(&font, NULL, 0, 0, 0x00FFU, framebuffer, 8U, 8U));

    memset(framebuffer, 0, sizeof(framebuffer));
    TEST_ASSERT_EQUAL_INT(0, xy_font_draw_string(&font, "A\nA", 0, 0, 0x0CC0U, framebuffer, 8U, 8U));
    TEST_ASSERT_EQUAL_UINT16(0x0CC0U, framebuffer[0U * 8U + 0U]);
    TEST_ASSERT_EQUAL_UINT16(0x0CC0U, framebuffer[5U * 8U + 0U]);
    TEST_ASSERT_EQUAL_UINT16(0U, framebuffer[5U * 8U + 3U]);

    memset(framebuffer, 0, sizeof(framebuffer));
    TEST_ASSERT_EQUAL_INT(0,
                          xy_font_draw_text(&font, "A", 0, 0, 8, 0x0F0FU, XY_ALIGN_CENTER,
                                            framebuffer, 8U, 8U));
    TEST_ASSERT_EQUAL_UINT16(0x0F0FU, framebuffer[2]);
    TEST_ASSERT_EQUAL_INT(-1,
                          xy_font_draw_text(&font, "A", 0, 0, 8, 0x0F0FU, XY_ALIGN_LEFT,
                                            NULL, 8U, 8U));
}

static void test_font_engine_draws_and_caches_wide_glyph_rows(void)
{
    static const uint8_t wide_data[4] = {
        0x80, 0x80, /* pixels 0 and 8 set */
        0x40, 0x40, /* pixels 1 and 9 set */
    };
    static const xy_glyph_t wide_glyph[] = {
        {wide_data, 10U, 2U, 10, 0, 0},
    };
    xy_font_t font;
    uint16_t framebuffer[12 * 3];
    const uint8_t *cached_wide;

    memset(&font, 0, sizeof(font));
    font.name = "wide";
    font.type = XY_FONT_TYPE_BITMAP;
    font.style.size = 10U;
    font.glyphs = wide_glyph;
    font.glyph_count = 1U;
    font.first_char = 'W';
    font.last_char = 'W';
    font.line_height = 3U;
    font.initialized = true;

    memset(framebuffer, 0, sizeof(framebuffer));
    TEST_ASSERT_EQUAL_INT(0, xy_font_draw_char(&font, 'W', 1, 1, 0x0ACEU, framebuffer, 12U, 3U));
    TEST_ASSERT_EQUAL_UINT16(0x0ACEU, framebuffer[1U * 12U + 1U]);
    TEST_ASSERT_EQUAL_UINT16(0x0ACEU, framebuffer[1U * 12U + 9U]);
    TEST_ASSERT_EQUAL_UINT16(0x0ACEU, framebuffer[2U * 12U + 2U]);
    TEST_ASSERT_EQUAL_UINT16(0x0ACEU, framebuffer[2U * 12U + 10U]);
    TEST_ASSERT_EQUAL_UINT16(0U, framebuffer[1U * 12U + 8U]);

    TEST_ASSERT_EQUAL_INT(0, xy_font_cache_init(&font, 1U));
    TEST_ASSERT_EQUAL_INT(0, xy_font_cache_glyph(&font, 'W'));
    cached_wide = xy_font_cache_get(&font, 'W');
    TEST_ASSERT_NOT_NULL(cached_wide);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(wide_data, cached_wide, sizeof(wide_data));
    xy_font_cache_clear(&font);
}

static void test_font_cache_lifecycle_and_lru_contracts(void)
{
    xy_font_t font;
    const uint8_t *cached_a;
    const uint8_t *cached_b;

    make_test_font(&font);
    TEST_ASSERT_EQUAL_INT(-1, xy_font_cache_init(NULL, 1U));
    TEST_ASSERT_EQUAL_INT(0, xy_font_cache_init(&font, 0U));
    TEST_ASSERT_FALSE(font.cache.enabled);
    TEST_ASSERT_NULL(font.cache.entries);
    TEST_ASSERT_EQUAL_INT(-1, xy_font_cache_glyph(&font, 'A'));
    TEST_ASSERT_NULL(xy_font_cache_get(&font, 'A'));

    TEST_ASSERT_EQUAL_INT(0, xy_font_cache_init(&font, 1U));
    TEST_ASSERT_TRUE(font.cache.enabled);
    TEST_ASSERT_EQUAL_UINT8(1U, font.cache.max_entries);
    TEST_ASSERT_EQUAL_INT(-1, xy_font_cache_glyph(&font, 'Z'));
    TEST_ASSERT_NULL(xy_font_cache_get(&font, 'A'));

    TEST_ASSERT_EQUAL_INT(0, xy_font_cache_glyph(&font, 'A'));
    cached_a = xy_font_cache_get(&font, 'A');
    TEST_ASSERT_NOT_NULL(cached_a);
    TEST_ASSERT_EQUAL_HEX8(0x80U, cached_a[0]);
    TEST_ASSERT_EQUAL_UINT32(2U, g_fake_tick);

    TEST_ASSERT_EQUAL_INT(0, xy_font_cache_glyph(&font, 'A'));
    TEST_ASSERT_EQUAL_UINT32(3U, g_fake_tick);

    TEST_ASSERT_EQUAL_INT(0, xy_font_cache_glyph(&font, 'B'));
    TEST_ASSERT_NULL(xy_font_cache_get(&font, 'A'));
    cached_b = xy_font_cache_get(&font, 'B');
    TEST_ASSERT_NOT_NULL(cached_b);
    TEST_ASSERT_EQUAL_HEX8(0xF0U, cached_b[0]);

    xy_font_cache_clear(&font);
    TEST_ASSERT_NULL(xy_font_cache_get(&font, 'B'));
    TEST_ASSERT_FALSE(font.cache.enabled);
    TEST_ASSERT_EQUAL_UINT8(0U, font.cache.max_entries);
    TEST_ASSERT_NULL(font.cache.entries);

    xy_font_cache_clear(NULL);
    xy_font_cache_clear(&font);
}

static void test_font_load_public_stub_contract(void)
{
    xy_font_t font;
    const uint8_t fake_font_blob[4] = {0x58U, 0x59U, 0x46U, 0x54U};

    make_test_font(&font);
    TEST_ASSERT_EQUAL_INT(-1, xy_font_load(NULL, fake_font_blob, sizeof(fake_font_blob)));
    TEST_ASSERT_EQUAL_INT(-1, xy_font_load(&font, NULL, sizeof(fake_font_blob)));
    TEST_ASSERT_EQUAL_INT(-1, xy_font_load(&font, fake_font_blob, 0U));
    TEST_ASSERT_EQUAL_INT(0, xy_font_load(&font, fake_font_blob, sizeof(fake_font_blob)));

    /* Loading a custom blob is currently a host-guarded stub: it validates inputs
     * but preserves the caller-provided runtime font descriptor until a real font
     * file format/parser is designed.
     */
    TEST_ASSERT_EQUAL_STRING("test", font.name);
    TEST_ASSERT_EQUAL_INT(XY_FONT_TYPE_BITMAP, font.type);
    TEST_ASSERT_EQUAL_UINT16(2U, font.glyph_count);
    TEST_ASSERT_TRUE(font.initialized);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_font_engine_glyph_measure_and_draw_contracts);
    RUN_TEST(test_font_engine_draws_and_caches_wide_glyph_rows);
    RUN_TEST(test_font_cache_lifecycle_and_lru_contracts);
    RUN_TEST(test_font_load_public_stub_contract);
    return UNITY_END();
}
