#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "xy_font.h"

#define SNAPSHOT_WIDTH 32U
#define SNAPSHOT_HEIGHT 16U
#define SNAPSHOT_COLOR 0xFFFFU
#define FNV1A32_OFFSET 2166136261UL
#define FNV1A32_PRIME 16777619UL

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

static void make_snapshot_font(xy_font_t *font)
{
    static const uint8_t glyph_space_data[4] = {
        0x00, 0x00, 0x00, 0x00,
    };
    static const uint8_t glyph_bang_data[4] = {
        0x80, 0x80, 0x00, 0x80,
    };
    static const uint8_t glyph_o_data[4] = {
        0x60, 0x90, 0x90, 0x60,
    };
    static const uint8_t glyph_k_data[4] = {
        0x90, 0xA0, 0xC0, 0xA0,
    };
    static const xy_glyph_t glyphs[] = {
        {glyph_space_data, 4U, 4U, 4, 0, 0}, /* ' ' */
        {glyph_bang_data, 1U, 4U, 2, 0, 0},  /* '!' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '"' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '#' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '$' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '%' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '&' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '\'' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '(' */
        {NULL, 0U, 0U, 0, 0, 0},             /* ')' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '*' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '+' */
        {NULL, 0U, 0U, 0, 0, 0},             /* ',' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '-' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '.' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '/' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '0' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '1' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '2' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '3' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '4' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '5' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '6' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '7' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '8' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '9' */
        {NULL, 0U, 0U, 0, 0, 0},             /* ':' */
        {NULL, 0U, 0U, 0, 0, 0},             /* ';' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '<' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '=' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '>' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '?' */
        {NULL, 0U, 0U, 0, 0, 0},             /* '@' */
        {NULL, 0U, 0U, 0, 0, 0},             /* 'A' */
        {NULL, 0U, 0U, 0, 0, 0},             /* 'B' */
        {NULL, 0U, 0U, 0, 0, 0},             /* 'C' */
        {NULL, 0U, 0U, 0, 0, 0},             /* 'D' */
        {NULL, 0U, 0U, 0, 0, 0},             /* 'E' */
        {NULL, 0U, 0U, 0, 0, 0},             /* 'F' */
        {NULL, 0U, 0U, 0, 0, 0},             /* 'G' */
        {NULL, 0U, 0U, 0, 0, 0},             /* 'H' */
        {NULL, 0U, 0U, 0, 0, 0},             /* 'I' */
        {NULL, 0U, 0U, 0, 0, 0},             /* 'J' */
        {glyph_k_data, 4U, 4U, 5, 0, 0},     /* 'K' */
        {NULL, 0U, 0U, 0, 0, 0},             /* 'L' */
        {NULL, 0U, 0U, 0, 0, 0},             /* 'M' */
        {NULL, 0U, 0U, 0, 0, 0},             /* 'N' */
        {glyph_o_data, 4U, 4U, 5, 0, 0},     /* 'O' */
    };

    memset(font, 0, sizeof(*font));
    font->name = "snapshot-host-font";
    font->type = XY_FONT_TYPE_BITMAP;
    font->style.size = 4U;
    font->glyphs = glyphs;
    font->glyph_count = (uint16_t)(sizeof(glyphs) / sizeof(glyphs[0]));
    font->first_char = ' ';
    font->last_char = 'O';
    font->line_height = 5U;
    font->baseline = 4U;
    font->initialized = true;
}

static uint16_t count_lit_pixels(const uint16_t *framebuffer, uint16_t width, uint16_t height)
{
    uint16_t count = 0U;

    for (uint16_t index = 0U; index < (uint16_t)(width * height); ++index) {
        if (framebuffer[index] != 0U) {
            ++count;
        }
    }

    return count;
}

static uint32_t checksum_framebuffer(const uint16_t *framebuffer, uint16_t width, uint16_t height)
{
    uint32_t hash = FNV1A32_OFFSET;

    for (uint16_t index = 0U; index < (uint16_t)(width * height); ++index) {
        uint16_t pixel = framebuffer[index];
        hash ^= (uint8_t)(pixel & 0xFFU);
        hash *= FNV1A32_PRIME;
        hash ^= (uint8_t)(pixel >> 8U);
        hash *= FNV1A32_PRIME;
    }

    return hash;
}

static void render_ascii_art(const uint16_t *framebuffer, char preview[SNAPSHOT_HEIGHT][SNAPSHOT_WIDTH + 1U])
{
    for (uint16_t row = 0U; row < SNAPSHOT_HEIGHT; ++row) {
        for (uint16_t col = 0U; col < SNAPSHOT_WIDTH; ++col) {
            preview[row][col] = (framebuffer[row * SNAPSHOT_WIDTH + col] != 0U) ? '#' : '.';
        }
        preview[row][SNAPSHOT_WIDTH] = '\0';
    }
}

static void test_font_snapshot_renders_stable_ascii_art_metadata(void)
{
    static const char *expected_rows[SNAPSHOT_HEIGHT] = {
        ".##..#..#.......................",
        "#..#.#.#........................",
        "#..#.##.........................",
        ".##..#.#........................",
        "................................",
        "#...............................",
        "#...............................",
        "................................",
        "#...............................",
        "................................",
        "................................",
        "................................",
        "................................",
        "................................",
        "................................",
        "................................",
    };
    xy_font_t font;
    uint16_t framebuffer[SNAPSHOT_WIDTH * SNAPSHOT_HEIGHT];
    char preview[SNAPSHOT_HEIGHT][SNAPSHOT_WIDTH + 1U];

    make_snapshot_font(&font);
    memset(framebuffer, 0, sizeof(framebuffer));

    TEST_ASSERT_EQUAL_INT(0, xy_font_draw_string(&font, "OK\n!~", 0, 0, SNAPSHOT_COLOR,
                                                 framebuffer, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT));
    render_ascii_art(framebuffer, preview);

    TEST_ASSERT_EQUAL_UINT16(SNAPSHOT_WIDTH, 32U);
    TEST_ASSERT_EQUAL_UINT16(SNAPSHOT_HEIGHT, 16U);
    TEST_ASSERT_EQUAL_UINT16(19U, count_lit_pixels(framebuffer, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT));
    TEST_ASSERT_EQUAL_UINT32(0x8DD0D797UL,
                             checksum_framebuffer(framebuffer, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT));
    for (uint16_t row = 0U; row < SNAPSHOT_HEIGHT; ++row) {
        TEST_ASSERT_EQUAL_STRING(expected_rows[row], preview[row]);
    }
}

static void test_font_snapshot_preserves_buffer_on_unknown_glyph(void)
{
    xy_font_t font;
    uint16_t framebuffer[SNAPSHOT_WIDTH * SNAPSHOT_HEIGHT];
    uint32_t checksum_before;

    make_snapshot_font(&font);
    memset(framebuffer, 0xA5, sizeof(framebuffer));
    checksum_before = checksum_framebuffer(framebuffer, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT);

    TEST_ASSERT_NULL(xy_font_get_glyph(&font, '~'));
    TEST_ASSERT_EQUAL_INT(-1, xy_font_draw_char(&font, '~', 0, 0, SNAPSHOT_COLOR,
                                                framebuffer, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT));
    TEST_ASSERT_EQUAL_UINT32(checksum_before,
                             checksum_framebuffer(framebuffer, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT));
}

static void test_font_snapshot_clips_without_overflowing_framebuffer(void)
{
    xy_font_t font;
    uint16_t framebuffer[SNAPSHOT_WIDTH * SNAPSHOT_HEIGHT];
    char preview[SNAPSHOT_HEIGHT][SNAPSHOT_WIDTH + 1U];

    make_snapshot_font(&font);
    memset(framebuffer, 0, sizeof(framebuffer));

    TEST_ASSERT_EQUAL_INT(0, xy_font_draw_char(&font, 'O', 30, 14, SNAPSHOT_COLOR,
                                               framebuffer, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT));
    render_ascii_art(framebuffer, preview);

    TEST_ASSERT_EQUAL_UINT16(2U, count_lit_pixels(framebuffer, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT));
    TEST_ASSERT_EQUAL_STRING("...............................#", preview[14]);
    TEST_ASSERT_EQUAL_STRING("..............................#.", preview[15]);
    TEST_ASSERT_EQUAL_UINT32(0x3ABE3861UL,
                             checksum_framebuffer(framebuffer, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_font_snapshot_renders_stable_ascii_art_metadata);
    RUN_TEST(test_font_snapshot_preserves_buffer_on_unknown_glyph);
    RUN_TEST(test_font_snapshot_clips_without_overflowing_framebuffer);
    return UNITY_END();
}
