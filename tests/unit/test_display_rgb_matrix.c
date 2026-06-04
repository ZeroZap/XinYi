/*
 * Focused host contracts for RGB matrix over WS2812 backing storage.
 */
#include "xy_rgb_matrix.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static unsigned s_gpio_transitions;
static uint8_t s_last_pin;
static uint8_t s_last_value;

static void fake_gpio_write(uint8_t pin, uint8_t value)
{
    s_last_pin = pin;
    s_last_value = value;
    s_gpio_transitions++;
}

static xy_ws2812_color_t color(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    xy_ws2812_color_t c = {r, g, b, w};
    return c;
}

static int same_color(xy_ws2812_color_t a, xy_ws2812_color_t b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.w == b.w;
}

static xy_rgb_matrix_handle_t make_matrix(xy_rgb_matrix_layout_t layout, uint8_t brightness)
{
    xy_rgb_matrix_handle_t matrix;
    xy_rgb_matrix_config_t matrix_config = {
        .width = 4,
        .height = 3,
        .layout = layout,
        .color_order = WS2812_COLOR_GRB,
        .brightness = brightness,
    };
    xy_ws2812_config_t ws_config = {
        .num_leds = 1,
        .led_type = WS2812_TYPE_WS2812B,
        .data_pin = 7,
        .color_order = WS2812_COLOR_RGB,
        .use_dma = false,
        .brightness = 0,
    };

    memset(&matrix, 0, sizeof(matrix));
    assert(xy_rgb_matrix_init(&matrix, &matrix_config, &ws_config) == XY_RGB_MATRIX_OK);
    assert(matrix.initialized);
    assert(matrix.ws2812.config.num_leds == 12);
    assert(matrix.ws2812.config.data_pin == 7);
    assert(matrix.ws2812.config.color_order == WS2812_COLOR_GRB);
    assert(matrix.ws2812.config.brightness == brightness);
    return matrix;
}

static void test_init_validation_and_layout_mapping(void)
{
    xy_rgb_matrix_handle_t matrix;
    xy_rgb_matrix_config_t bad_matrix = {0};
    xy_ws2812_config_t ws_config = {.num_leds = 1};
    uint16_t x = 99;
    uint16_t y = 99;

    assert(xy_rgb_matrix_init(NULL, &bad_matrix, &ws_config) == XY_RGB_MATRIX_ERROR_INVALID_PARAM);
    assert(xy_rgb_matrix_init(&matrix, NULL, &ws_config) == XY_RGB_MATRIX_ERROR_INVALID_PARAM);
    assert(xy_rgb_matrix_init(&matrix, &bad_matrix, &ws_config) == XY_RGB_MATRIX_ERROR_INVALID_PARAM);

    matrix = make_matrix(XY_RGB_MATRIX_LAYOUT_ZIGZAG, 255);
    assert(xy_rgb_matrix_xy_to_index(&matrix, 0, 0) == 0);
    assert(xy_rgb_matrix_xy_to_index(&matrix, 3, 0) == 3);
    assert(xy_rgb_matrix_xy_to_index(&matrix, 0, 1) == 7);
    assert(xy_rgb_matrix_xy_to_index(&matrix, 3, 1) == 4);
    assert(xy_rgb_matrix_xy_to_index(&matrix, 2, 2) == 10);

    xy_rgb_matrix_index_to_xy(&matrix, 7, &x, &y);
    assert(x == 0 && y == 1);
    xy_rgb_matrix_index_to_xy(&matrix, 99, &x, &y);
    assert(x == 0 && y == 0);
    xy_rgb_matrix_deinit(&matrix);

    matrix = make_matrix(XY_RGB_MATRIX_LAYOUT_LINEAR, 255);
    assert(xy_rgb_matrix_xy_to_index(&matrix, 0, 1) == 4);
    xy_rgb_matrix_index_to_xy(&matrix, 7, &x, &y);
    assert(x == 3 && y == 1);
    xy_rgb_matrix_deinit(&matrix);
}

static void test_pixel_line_rect_and_brightness_contracts(void)
{
    xy_rgb_matrix_handle_t matrix = make_matrix(XY_RGB_MATRIX_LAYOUT_LINEAR, 128);
    xy_ws2812_color_t red = color(255, 0, 0, 0);
    xy_ws2812_color_t green = color(0, 255, 0, 0);
    xy_ws2812_color_t blue = color(0, 0, 255, 0);
    xy_ws2812_color_t scaled_red = color(127, 0, 0, 0);
    xy_ws2812_color_t scaled_green = color(0, 127, 0, 0);
    xy_ws2812_color_t scaled_blue = color(0, 0, 127, 0);

    xy_rgb_matrix_set_pixel(&matrix, 1, 1, red);
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 1, 1), scaled_red));
    assert(matrix.ws2812.dirty);

    xy_rgb_matrix_set_pixel(&matrix, 9, 9, green);
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 9, 9), WS2812_COLOR_BLACK));

    xy_rgb_matrix_draw_hline(&matrix, 0, green);
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 0, 0), scaled_green));
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 3, 0), scaled_green));

    xy_rgb_matrix_draw_vline(&matrix, 3, blue);
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 3, 0), scaled_blue));
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 3, 2), scaled_blue));

    xy_rgb_matrix_clear(&matrix);
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 3, 2), WS2812_COLOR_BLACK));

    xy_rgb_matrix_set_brightness(&matrix, 255);
    assert(xy_rgb_matrix_get_brightness(&matrix) == 255);
    xy_rgb_matrix_draw_rect(&matrix, 0, 0, 3, 2, red, false);
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 0, 0), color(254, 0, 0, 0)));
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 1, 1), WS2812_COLOR_BLACK));
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 3, 2), color(254, 0, 0, 0)));

    xy_rgb_matrix_draw_rect(&matrix, 0, 0, 1, 1, blue, true);
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 0, 0), color(0, 0, 254, 0)));
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 1, 1), color(0, 0, 254, 0)));

    xy_rgb_matrix_deinit(&matrix);
}

static void test_show_and_effect_contracts(void)
{
    xy_rgb_matrix_handle_t matrix = make_matrix(XY_RGB_MATRIX_LAYOUT_LINEAR, 255);
    xy_ws2812_color_t effect = color(32, 64, 128, 0);

    xy_ws2812_set_gpio_callback(fake_gpio_write);
    s_gpio_transitions = 0;
    s_last_pin = 0;
    s_last_value = 1;

    xy_rgb_matrix_fill(&matrix, color(255, 0, 0, 0));
    assert(matrix.ws2812.dirty);
    xy_rgb_matrix_show(&matrix);
    assert(!matrix.ws2812.dirty);
    assert(s_gpio_transitions > 0);
    assert(s_last_pin == 7);
    assert(s_last_value == 0);

    xy_rgb_matrix_set_effect(&matrix, XY_RGB_MATRIX_EFFECT_SOLID, 255, effect);
    assert(xy_rgb_matrix_update_effect(&matrix));
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 0, 0), color(31, 63, 127, 0)));

    xy_rgb_matrix_set_effect(&matrix, XY_RGB_MATRIX_EFFECT_BREATHING, 255, effect);
    xy_rgb_matrix_effect_breathing(&matrix, 64);
    assert(same_color(xy_rgb_matrix_get_pixel(&matrix, 0, 0), color(15, 31, 63, 0)));

    xy_rgb_matrix_effect_rainbow(&matrix, 0);
    assert(!same_color(xy_rgb_matrix_get_pixel(&matrix, 0, 0), WS2812_COLOR_BLACK));

    xy_rgb_matrix_deinit(&matrix);
}

int main(void)
{
    test_init_validation_and_layout_mapping();
    test_pixel_line_rect_and_brightness_contracts();
    test_show_and_effect_contracts();
    return 0;
}
