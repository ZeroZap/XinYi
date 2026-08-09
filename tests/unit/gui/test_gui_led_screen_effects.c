#include "unity.h"

#include "xy_gui_screen_fx.h"
#include "xy_led_screen.h"

#include <stdbool.h>
#include <stdint.h>

void setUp(void) {}
void tearDown(void) {}

static void test_led_screen_headers_expose_buffer_and_screen_contracts(void)
{
    xy_screen_buffer_t buffer = {
        .data = NULL,
        .width = 8U,
        .height = 4U,
        .stride = 8U * sizeof(uint32_t),
        .format = XY_SCREEN_ARGB8888,
        .palette_size = 0U,
    };
    xy_led_screen_t screen = {
        .buffer = {buffer, buffer},
        .current_buffer = 0U,
        .width = 8U,
        .height = 4U,
        .format = XY_SCREEN_ARGB8888,
        .dirty = false,
        .effect_frame = 0U,
        .effect_progress = 0U,
        .send_buffer = NULL,
    };
    xy_screen_color_t color = {.a = 0x12U, .r = 0x34U, .g = 0x56U, .b = 0x78U};

    TEST_ASSERT_EQUAL_UINT16(8U, buffer.width);
    TEST_ASSERT_EQUAL_UINT16(4U, screen.height);
    TEST_ASSERT_EQUAL_INT(XY_SCREEN_ARGB8888, screen.format);
    TEST_ASSERT_FALSE(screen.dirty);
    TEST_ASSERT_EQUAL_UINT8(0x12U, color.a);
    TEST_ASSERT_EQUAL_UINT8(0x34U, color.r);
    TEST_ASSERT_EQUAL_UINT8(0x56U, color.g);
    TEST_ASSERT_EQUAL_UINT8(0x78U, color.b);
}

static void test_led_screen_headers_expose_public_function_signatures(void)
{
    typedef int (*screen_init_fn_t)(xy_led_screen_t *, uint16_t, uint16_t, xy_screen_format_t,
                                    void (*)(uint32_t *, uint16_t, uint16_t));
    typedef void (*set_pixel_fn_t)(xy_led_screen_t *, int16_t, int16_t, xy_screen_color_t);
    typedef xy_screen_color_t (*get_pixel_fn_t)(xy_led_screen_t *, int16_t, int16_t);
    typedef void (*fill_fn_t)(xy_led_screen_t *, xy_screen_color_t);
    typedef void (*draw_rect_fn_t)(xy_led_screen_t *, int16_t, int16_t, int16_t, int16_t,
                                   xy_screen_color_t, bool);
    typedef void (*scroll_text_fn_t)(xy_led_screen_t *, const char *, xy_screen_scroll_dir_t,
                                     uint8_t, xy_screen_color_t);

    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(screen_init_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(set_pixel_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(get_pixel_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(fill_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(draw_rect_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(scroll_text_fn_t));
}

static void test_led_screen_fx_headers_expose_engine_contracts(void)
{
    xy_led_screen_t screen = {.width = 16U, .height = 8U, .format = XY_SCREEN_RGB565};
    xy_led_screen_fx_config_t config = {
        .fx_id = FX_SCREEN_FADE_IN,
        .speed = 10U,
        .intensity = 20U,
        .param1 = 3U,
        .param2 = 4U,
        .color = 0x00ABCDEFU,
    };
    xy_led_screen_fx_engine_t engine = {
        .screen = &screen,
        .current_fx = FX_SCREEN_FADE_IN,
        .config = config,
        .frame = 7U,
        .last_update = 100U,
        .running = true,
    };

    TEST_ASSERT_EQUAL_INT(FX_SCREEN_NONE, 0);
    TEST_ASSERT_GREATER_THAN_INT(FX_SCREEN_NONE, FX_SCREEN_FADE_IN);
    TEST_ASSERT_LESS_THAN_INT(FX_SCREEN_COUNT, FX_SCREEN_FADE_IN);
    TEST_ASSERT_EQUAL_PTR(&screen, engine.screen);
    TEST_ASSERT_EQUAL_INT(FX_SCREEN_FADE_IN, engine.current_fx);
    TEST_ASSERT_EQUAL_UINT16(10U, engine.config.speed);
    TEST_ASSERT_EQUAL_UINT32(7U, engine.frame);
    TEST_ASSERT_TRUE(engine.running);
}

static void test_led_screen_fx_headers_expose_public_function_signatures(void)
{
    typedef int (*fx_init_fn_t)(xy_led_screen_fx_engine_t *, xy_led_screen_t *);
    typedef int (*fx_set_fn_t)(xy_led_screen_fx_engine_t *, xy_led_screen_fx_id_t, uint16_t,
                               uint16_t, uint32_t);
    typedef void (*fx_stop_fn_t)(xy_led_screen_fx_engine_t *);
    typedef void (*fx_update_fn_t)(xy_led_screen_fx_engine_t *);
    typedef uint8_t (*fx_count_fn_t)(void);
    typedef const char *(*fx_name_fn_t)(xy_led_screen_fx_id_t);
    typedef xy_led_screen_fx_id_t (*fx_next_fn_t)(xy_led_screen_fx_id_t);

    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(fx_init_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(fx_set_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(fx_stop_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(fx_update_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(fx_count_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(fx_name_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(fx_next_fn_t));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_led_screen_headers_expose_buffer_and_screen_contracts);
    RUN_TEST(test_led_screen_headers_expose_public_function_signatures);
    RUN_TEST(test_led_screen_fx_headers_expose_engine_contracts);
    RUN_TEST(test_led_screen_fx_headers_expose_public_function_signatures);
    return UNITY_END();
}
