#include "unity.h"

#include "xy_rgb_color.h"
#include "xy_rgb_fx.h"

#include <stdint.h>
#include <string.h>

#define FAKE_LED_COUNT 32U

uint32_t g_frame_count;

static rgb_color_t fake_pixels[FAKE_LED_COUNT];
static uint16_t fake_last_set_index;
static uint16_t fake_set_calls;
static uint16_t fake_get_calls;
static uint16_t fake_clear_calls;
static uint16_t fake_show_calls;
static uint16_t fake_delay_calls;

void setUp(void)
{
    memset(fake_pixels, 0, sizeof(fake_pixels));
    fake_last_set_index = 0U;
    fake_set_calls = 0U;
    fake_get_calls = 0U;
    fake_clear_calls = 0U;
    fake_show_calls = 0U;
    fake_delay_calls = 0U;
    g_frame_count = 0U;
}

void tearDown(void) {}

void xy_rgb_set_pixel(uint16_t index, rgb_color_t color)
{
    fake_set_calls++;
    fake_last_set_index = index;
    if (index < FAKE_LED_COUNT) {
        fake_pixels[index] = color;
    }
}

rgb_color_t xy_rgb_get_pixel(uint16_t index)
{
    fake_get_calls++;
    if (index < FAKE_LED_COUNT) {
        return fake_pixels[index];
    }
    return (rgb_color_t){0U, 0U, 0U};
}

void xy_rgb_clear(void)
{
    fake_clear_calls++;
    memset(fake_pixels, 0, sizeof(fake_pixels));
}

void xy_rgb_show(void)
{
    fake_show_calls++;
}

void xy_rgb_set_all(rgb_color_t color)
{
    for (uint16_t i = 0U; i < FAKE_LED_COUNT; ++i) {
        fake_pixels[i] = color;
    }
}

void xy_os_delay(uint32_t ms)
{
    (void)ms;
    fake_delay_calls++;
}

rgb_color_t xy_color_blend(rgb_color_t color1, rgb_color_t color2, uint8_t factor)
{
    rgb_color_t blended = {
        .r = (uint8_t)(((uint16_t)color1.r * (255U - factor) + (uint16_t)color2.r * factor) / 255U),
        .g = (uint8_t)(((uint16_t)color1.g * (255U - factor) + (uint16_t)color2.g * factor) / 255U),
        .b = (uint8_t)(((uint16_t)color1.b * (255U - factor) + (uint16_t)color2.b * factor) / 255U),
    };
    return blended;
}

rgb_color_t xy_color_darken(rgb_color_t color, uint8_t amount)
{
    rgb_color_t darkened = {
        .r = (uint8_t)((uint16_t)color.r * (255U - amount) / 255U),
        .g = (uint8_t)((uint16_t)color.g * (255U - amount) / 255U),
        .b = (uint8_t)((uint16_t)color.b * (255U - amount) / 255U),
    };
    return darkened;
}

rgb_color_t xy_color_random(void)
{
    return (rgb_color_t){0x12U, 0x34U, 0x56U};
}

bool xy_color_equal(rgb_color_t color1, rgb_color_t color2)
{
    return color1.r == color2.r && color1.g == color2.g && color1.b == color2.b;
}

rgb_color_t xy_rainbow_color(uint8_t hue)
{
    return (rgb_color_t){hue, (uint8_t)(255U - hue), (uint8_t)(hue / 2U)};
}

void xy_rgb_set_spectrum(uint8_t *spectrum, uint8_t bands);
void xy_rgb_set_volume(uint8_t volume);
void xy_rgb_set_beat(uint8_t beat);
void xy_rgb_fx_beat(xy_rgb_segment_t *seg);
void xy_rgb_fx_frequency(xy_rgb_segment_t *seg);
void xy_rgb_fx_autocorr(xy_rgb_segment_t *seg);
void xy_rgb_fx_music_fire(xy_rgb_segment_t *seg);
void xy_rgb_fx_vu_meter_enhanced(xy_rgb_segment_t *seg);
void xy_rgb_matrix_set_size(uint16_t width, uint16_t height);
void xy_rgb_fx_matrix_plasma(xy_rgb_segment_t *seg);

static xy_rgb_segment_t make_segment(void)
{
    xy_rgb_segment_t seg = {
        .start = 2U,
        .stop = 10U,
        .speed = 64U,
        .intensity = 128U,
        .color1 = {10U, 20U, 30U},
        .color2 = {40U, 50U, 60U},
        .color3 = {70U, 80U, 90U},
        .enabled = true,
    };
    return seg;
}

static void test_music_setters_and_beat_effect_use_rgb_strip_seam(void)
{
    xy_rgb_segment_t seg = make_segment();
    uint8_t spectrum[16] = {0U};

    xy_rgb_set_spectrum(spectrum, 16U);
    xy_rgb_set_volume(200U);
    xy_rgb_set_beat(128U);
    xy_rgb_fx_beat(&seg);

    TEST_ASSERT_EQUAL_UINT16(8U, fake_set_calls);
    TEST_ASSERT_EQUAL_UINT16(9U, fake_last_set_index);
    TEST_ASSERT_EQUAL_UINT8(5U, fake_pixels[2].r);
    TEST_ASSERT_EQUAL_UINT8(10U, fake_pixels[2].g);
    TEST_ASSERT_EQUAL_UINT8(15U, fake_pixels[2].b);
}

static void test_music_frequency_effect_compiles_against_spectrum_and_color_helpers(void)
{
    xy_rgb_segment_t seg = make_segment();
    uint8_t spectrum[16] = {255U, 128U, 64U, 32U, 16U, 8U, 4U, 2U};

    seg.stop = 18U;
    xy_rgb_set_spectrum(spectrum, 16U);
    xy_rgb_fx_frequency(&seg);

    TEST_ASSERT_GREATER_THAN_UINT16(0U, fake_set_calls);
    TEST_ASSERT_EQUAL_UINT8(255U, fake_pixels[2].r);
    TEST_ASSERT_EQUAL_UINT8(0U, fake_pixels[2].g);
    TEST_ASSERT_EQUAL_UINT8(0U, fake_pixels[2].b);
}

static void test_music_autocorr_effect_uses_test_owned_frame_counter(void)
{
    xy_rgb_segment_t seg = make_segment();

    g_frame_count = 4U;
    xy_rgb_set_volume(255U);
    xy_rgb_fx_autocorr(&seg);

    TEST_ASSERT_EQUAL_UINT16(8U, fake_set_calls);
    TEST_ASSERT_EQUAL_UINT16(9U, fake_last_set_index);
}

static void test_music_vu_meter_enhanced_uses_peak_and_frame_counter_contract(void)
{
    xy_rgb_segment_t seg = make_segment();

    g_frame_count = 10U;
    xy_rgb_set_volume(255U);
    xy_rgb_fx_vu_meter_enhanced(&seg);

    TEST_ASSERT_EQUAL_UINT16(8U, fake_set_calls);
    TEST_ASSERT_EQUAL_UINT8(10U, fake_pixels[2].r);
    TEST_ASSERT_EQUAL_UINT8(20U, fake_pixels[2].g);
    TEST_ASSERT_EQUAL_UINT8(30U, fake_pixels[2].b);
    TEST_ASSERT_EQUAL_UINT8(40U, fake_pixels[7].r);
    TEST_ASSERT_EQUAL_UINT8(50U, fake_pixels[7].g);
    TEST_ASSERT_EQUAL_UINT8(60U, fake_pixels[7].b);
    TEST_ASSERT_EQUAL_UINT8(10U, fake_pixels[9].r);
    TEST_ASSERT_EQUAL_UINT8(10U, fake_pixels[9].g);
    TEST_ASSERT_EQUAL_UINT8(10U, fake_pixels[9].b);
}

static void test_matrix_plasma_effect_compiles_against_2d_seam(void)
{
    xy_rgb_segment_t seg = make_segment();

    xy_rgb_matrix_set_size(4U, 4U);
    g_frame_count = 0U;
    xy_rgb_fx_matrix_plasma(&seg);

    TEST_ASSERT_EQUAL_UINT16(8U, fake_set_calls);
    TEST_ASSERT_EQUAL_UINT16(9U, fake_last_set_index);
    TEST_ASSERT_EQUAL_UINT8(128U, fake_pixels[2].r);
    TEST_ASSERT_EQUAL_UINT8(127U, fake_pixels[2].g);
    TEST_ASSERT_EQUAL_UINT8(64U, fake_pixels[2].b);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_music_setters_and_beat_effect_use_rgb_strip_seam);
    RUN_TEST(test_music_frequency_effect_compiles_against_spectrum_and_color_helpers);
    RUN_TEST(test_music_autocorr_effect_uses_test_owned_frame_counter);
    RUN_TEST(test_music_vu_meter_enhanced_uses_peak_and_frame_counter_contract);
    RUN_TEST(test_matrix_plasma_effect_compiles_against_2d_seam);
    return UNITY_END();
}
