#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "xy_gui.h"
#include "xy_led_driver.h"

#define FAKE_DISPLAY_WIDTH  16U
#define FAKE_DISPLAY_HEIGHT 8U

static uint16_t g_framebuffer[FAKE_DISPLAY_HEIGHT][FAKE_DISPLAY_WIDTH];
static uint16_t g_alt_framebuffer[FAKE_DISPLAY_HEIGHT][FAKE_DISPLAY_WIDTH];
static unsigned int g_init_calls;
static unsigned int g_set_pixel_calls;
static unsigned int g_alt_set_pixel_calls;
static unsigned int g_fill_rect_calls;
static unsigned int g_flush_calls;
static unsigned int g_alt_flush_calls;
static int16_t g_last_x;
static int16_t g_last_y;
static int16_t g_last_w;
static int16_t g_last_h;
static uint16_t g_last_color;
static uint16_t g_alt_last_color;
static bool g_backend_fail;
static xy_gui_display_t *g_led_gui_display;

void setUp(void)
{
    memset(g_framebuffer, 0, sizeof(g_framebuffer));
    memset(g_alt_framebuffer, 0, sizeof(g_alt_framebuffer));
    g_init_calls = 0;
    g_set_pixel_calls = 0;
    g_alt_set_pixel_calls = 0;
    g_fill_rect_calls = 0;
    g_flush_calls = 0;
    g_alt_flush_calls = 0;
    g_last_x = -1;
    g_last_y = -1;
    g_last_w = -1;
    g_last_h = -1;
    g_last_color = 0;
    g_alt_last_color = 0;
    g_backend_fail = false;
    g_led_gui_display = NULL;
}

void tearDown(void)
{
}

static int fake_display_init(void)
{
    g_init_calls++;
    return g_backend_fail ? XY_GUI_ERROR : XY_GUI_OK;
}

static int fake_display_set_pixel(int16_t x, int16_t y, uint16_t color)
{
    g_set_pixel_calls++;
    g_last_x = x;
    g_last_y = y;
    g_last_color = color;

    if (g_backend_fail) {
        return XY_GUI_ERROR;
    }

    if (x >= 0 && x < (int16_t)FAKE_DISPLAY_WIDTH && y >= 0 && y < (int16_t)FAKE_DISPLAY_HEIGHT) {
        g_framebuffer[y][x] = color;
    }
    return XY_GUI_OK;
}

static int fake_display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    g_fill_rect_calls++;
    g_last_x = x;
    g_last_y = y;
    g_last_w = w;
    g_last_h = h;
    g_last_color = color;

    if (g_backend_fail) {
        return XY_GUI_ERROR;
    }

    for (int16_t row = 0; row < h; row++) {
        for (int16_t col = 0; col < w; col++) {
            const int16_t px = x + col;
            const int16_t py = y + row;
            if (px >= 0 && px < (int16_t)FAKE_DISPLAY_WIDTH && py >= 0 && py < (int16_t)FAKE_DISPLAY_HEIGHT) {
                g_framebuffer[py][px] = color;
            }
        }
    }
    return XY_GUI_OK;
}

static int fake_display_flush(void)
{
    g_flush_calls++;
    return g_backend_fail ? XY_GUI_ERROR : XY_GUI_OK;
}

static xy_gui_disp_drv_t fake_display_driver(void)
{
    xy_gui_disp_drv_t drv = {
        .init = fake_display_init,
        .draw_pixel = fake_display_set_pixel,
        .draw_line = NULL,
        .draw_rect = NULL,
        .fill_rect = fake_display_fill_rect,
        .draw_char = NULL,
        .flush = fake_display_flush,
    };
    return drv;
}

static xy_led_driver_t make_led_driver(void)
{
    xy_led_driver_t driver = {
        .width = FAKE_DISPLAY_WIDTH,
        .height = FAKE_DISPLAY_HEIGHT,
        .bpp = 16,
        .set_pixel = NULL,
        .get_pixel = NULL,
        .show = NULL,
        .user_data = NULL,
    };
    return driver;
}

static void fake_led_set_pixel(uint16_t x, uint16_t y, uint32_t color)
{
    g_set_pixel_calls++;
    g_last_x = (int16_t)x;
    g_last_y = (int16_t)y;
    g_last_color = (uint16_t)color;

    if (x < FAKE_DISPLAY_WIDTH && y < FAKE_DISPLAY_HEIGHT) {
        g_framebuffer[y][x] = (uint16_t)color;
    }
}

static uint32_t fake_led_get_pixel(uint16_t x, uint16_t y)
{
    if (x >= FAKE_DISPLAY_WIDTH || y >= FAKE_DISPLAY_HEIGHT) {
        return 0xDEADU;
    }

    return g_framebuffer[y][x];
}

static void fake_led_show(void)
{
    g_flush_calls++;
}

static void fake_alt_led_set_pixel(uint16_t x, uint16_t y, uint32_t color)
{
    g_alt_set_pixel_calls++;
    g_alt_last_color = (uint16_t)color;

    if (x < FAKE_DISPLAY_WIDTH && y < FAKE_DISPLAY_HEIGHT) {
        g_alt_framebuffer[y][x] = (uint16_t)color;
    }
}

static uint32_t fake_alt_led_get_pixel(uint16_t x, uint16_t y)
{
    if (x >= FAKE_DISPLAY_WIDTH || y >= FAKE_DISPLAY_HEIGHT) {
        return 0xBEEFU;
    }

    return g_alt_framebuffer[y][x];
}

static void fake_alt_led_show(void)
{
    g_alt_flush_calls++;
}

static int led_gui_adapter_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
    xy_gui_display_set_pixel(g_led_gui_display, x, y, color);
    return XY_GUI_OK;
}

static int led_gui_adapter_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (g_led_gui_display && g_led_gui_display->fill_rect) {
        g_led_gui_display->fill_rect(x, y, w, h, color);
    }
    return XY_GUI_OK;
}

static int led_gui_adapter_flush(void)
{
    xy_gui_display_flush(g_led_gui_display);
    return XY_GUI_OK;
}

static xy_gui_disp_drv_t led_gui_adapter_driver(void)
{
    xy_gui_disp_drv_t drv = {
        .init = NULL,
        .draw_pixel = led_gui_adapter_draw_pixel,
        .draw_line = NULL,
        .draw_rect = NULL,
        .fill_rect = led_gui_adapter_fill_rect,
        .draw_char = NULL,
        .flush = led_gui_adapter_flush,
    };
    return drv;
}

static void test_gui_context_forwards_to_fake_display_backend(void)
{
    xy_gui_t gui;
    xy_gui_disp_drv_t drv = fake_display_driver();

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_init(&gui, FAKE_DISPLAY_WIDTH, FAKE_DISPLAY_HEIGHT, &drv));
    TEST_ASSERT_EQUAL_UINT(1U, g_init_calls);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_clear(&gui, XY_GUI_COLOR_BLUE));
    TEST_ASSERT_EQUAL_UINT(1U, g_fill_rect_calls);
    TEST_ASSERT_EQUAL_INT16(0, g_last_x);
    TEST_ASSERT_EQUAL_INT16(0, g_last_y);
    TEST_ASSERT_EQUAL_INT16(FAKE_DISPLAY_WIDTH, g_last_w);
    TEST_ASSERT_EQUAL_INT16(FAKE_DISPLAY_HEIGHT, g_last_h);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_BLUE, g_last_color);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_BLUE, g_framebuffer[0][0]);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_BLUE, g_framebuffer[FAKE_DISPLAY_HEIGHT - 1U][FAKE_DISPLAY_WIDTH - 1U]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_pixel(&gui, 3, 4, XY_GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_UINT(1U, g_set_pixel_calls);
    TEST_ASSERT_EQUAL_INT16(3, g_last_x);
    TEST_ASSERT_EQUAL_INT16(4, g_last_y);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_RED, g_last_color);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_RED, g_framebuffer[4][3]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_fill_rect(&gui, 2, 1, 4, 3, XY_GUI_COLOR_GREEN));
    TEST_ASSERT_EQUAL_UINT(2U, g_fill_rect_calls);
    TEST_ASSERT_EQUAL_INT16(2, g_last_x);
    TEST_ASSERT_EQUAL_INT16(1, g_last_y);
    TEST_ASSERT_EQUAL_INT16(4, g_last_w);
    TEST_ASSERT_EQUAL_INT16(3, g_last_h);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_GREEN, g_last_color);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_GREEN, g_framebuffer[1][2]);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_GREEN, g_framebuffer[3][5]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_flush(&gui));
    TEST_ASSERT_EQUAL_UINT(1U, g_flush_calls);
}

static void test_gui_backend_failures_propagate_without_committing_state(void)
{
    xy_gui_t gui;
    xy_gui_disp_drv_t drv = fake_display_driver();

    g_backend_fail = true;
    TEST_ASSERT_EQUAL_INT(XY_GUI_ERROR,
                          xy_gui_init(&gui, FAKE_DISPLAY_WIDTH, FAKE_DISPLAY_HEIGHT, &drv));
    TEST_ASSERT_FALSE(gui.initialized);
    TEST_ASSERT_EQUAL_UINT(1U, g_init_calls);

    g_backend_fail = false;
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_init(&gui, FAKE_DISPLAY_WIDTH, FAKE_DISPLAY_HEIGHT, &drv));
    gui.bg_color = XY_GUI_COLOR_BLUE;
    g_backend_fail = true;

    TEST_ASSERT_EQUAL_INT(XY_GUI_ERROR, xy_gui_clear(&gui, XY_GUI_COLOR_WHITE));
    TEST_ASSERT_EQUAL_UINT(1U, g_fill_rect_calls);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_BLUE, gui.bg_color);
    TEST_ASSERT_EQUAL_HEX16(0, g_framebuffer[0][0]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_ERROR, xy_gui_draw_pixel(&gui, 1, 1, XY_GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_UINT(1U, g_set_pixel_calls);
    TEST_ASSERT_EQUAL_HEX16(0, g_framebuffer[1][1]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_ERROR,
                          xy_gui_fill_rect(&gui, 0, 0, 2, 2, XY_GUI_COLOR_GREEN));
    TEST_ASSERT_EQUAL_UINT(2U, g_fill_rect_calls);
    TEST_ASSERT_EQUAL_HEX16(0, g_framebuffer[0][0]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_ERROR, xy_gui_flush(&gui));
    TEST_ASSERT_EQUAL_UINT(1U, g_flush_calls);
}

static void test_gui_backend_guard_paths_do_not_touch_fake_display(void)
{
    xy_gui_t gui;
    xy_gui_disp_drv_t drv = fake_display_driver();

    memset(&gui, 0, sizeof(gui));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_clear(NULL, XY_GUI_COLOR_BLACK));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_draw_pixel(NULL, 0, 0, XY_GUI_COLOR_BLACK));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_fill_rect(NULL, 0, 0, 1, 1, XY_GUI_COLOR_BLACK));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_flush(NULL));
    TEST_ASSERT_EQUAL_UINT(0U, g_set_pixel_calls);
    TEST_ASSERT_EQUAL_UINT(0U, g_fill_rect_calls);
    TEST_ASSERT_EQUAL_UINT(0U, g_flush_calls);

    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_clear(&gui, XY_GUI_COLOR_BLACK));
    TEST_ASSERT_EQUAL_UINT(0U, g_fill_rect_calls);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_init(&gui, FAKE_DISPLAY_WIDTH, FAKE_DISPLAY_HEIGHT, &drv));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_draw_pixel(&gui, -1, 0, XY_GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_draw_pixel(&gui, FAKE_DISPLAY_WIDTH, 0, XY_GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_draw_pixel(&gui, 0, FAKE_DISPLAY_HEIGHT, XY_GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_UINT(0U, g_set_pixel_calls);
}

static void test_gui_fill_rect_fallback_clips_through_draw_pixel_guard(void)
{
    xy_gui_t gui;
    xy_gui_disp_drv_t drv = fake_display_driver();
    drv.fill_rect = NULL;

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_init(&gui, FAKE_DISPLAY_WIDTH, FAKE_DISPLAY_HEIGHT, &drv));

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_fill_rect(&gui, 14, 6, 4, 3, XY_GUI_COLOR_YELLOW));
    TEST_ASSERT_EQUAL_UINT(4U, g_set_pixel_calls);
    TEST_ASSERT_EQUAL_INT16(15, g_last_x);
    TEST_ASSERT_EQUAL_INT16(7, g_last_y);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_YELLOW, g_last_color);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_YELLOW, g_framebuffer[6][14]);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_YELLOW, g_framebuffer[6][15]);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_YELLOW, g_framebuffer[7][14]);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_YELLOW, g_framebuffer[7][15]);
    TEST_ASSERT_EQUAL_HEX16(0, g_framebuffer[5][14]);

    g_backend_fail = true;
    TEST_ASSERT_EQUAL_INT(XY_GUI_ERROR,
                          xy_gui_fill_rect(&gui, 0, 0, 2, 2, XY_GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_UINT(5U, g_set_pixel_calls);
    TEST_ASSERT_EQUAL_HEX16(0, g_framebuffer[0][0]);
}

static void test_gui_context_can_drive_led_gui_display_adapter(void)
{
    xy_led_driver_t led = make_led_driver();
    led.set_pixel = fake_led_set_pixel;
    led.get_pixel = fake_led_get_pixel;
    led.show = fake_led_show;

    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&led));
    g_led_gui_display = xy_led_get_gui_interface(&led);
    TEST_ASSERT_NOT_NULL(g_led_gui_display);
    TEST_ASSERT_EQUAL_UINT16(FAKE_DISPLAY_WIDTH, g_led_gui_display->width);
    TEST_ASSERT_EQUAL_UINT16(FAKE_DISPLAY_HEIGHT, g_led_gui_display->height);
    TEST_ASSERT_NOT_NULL(g_led_gui_display->get_pixel);

    xy_gui_t gui;
    xy_gui_disp_drv_t drv = led_gui_adapter_driver();
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_init(&gui, FAKE_DISPLAY_WIDTH, FAKE_DISPLAY_HEIGHT, &drv));

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_pixel(&gui, 6, 2, XY_GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_UINT(1U, g_set_pixel_calls);
    TEST_ASSERT_EQUAL_INT16(6, g_last_x);
    TEST_ASSERT_EQUAL_INT16(2, g_last_y);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_RED, g_last_color);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_RED, g_framebuffer[2][6]);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_RED, g_led_gui_display->get_pixel(6, 2));
    TEST_ASSERT_EQUAL_HEX16(0, g_led_gui_display->get_pixel(-1, 2));
    TEST_ASSERT_EQUAL_HEX16(0, g_led_gui_display->get_pixel(FAKE_DISPLAY_WIDTH, 2));

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_fill_rect(&gui, 14, 6, 4, 3, XY_GUI_COLOR_GREEN));
    TEST_ASSERT_EQUAL_UINT(5U, g_set_pixel_calls);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_GREEN, g_framebuffer[6][14]);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_GREEN, g_framebuffer[6][15]);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_GREEN, g_framebuffer[7][14]);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_GREEN, g_framebuffer[7][15]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_flush(&gui));
    TEST_ASSERT_EQUAL_UINT(1U, g_flush_calls);
}

static void test_led_gui_display_adapter_omits_get_pixel_when_backend_lacks_reader(void)
{
    xy_led_driver_t led = make_led_driver();
    led.set_pixel = fake_led_set_pixel;
    led.show = fake_led_show;

    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&led));
    g_led_gui_display = xy_led_get_gui_interface(&led);
    TEST_ASSERT_NOT_NULL(g_led_gui_display);
    TEST_ASSERT_NULL(g_led_gui_display->get_pixel);

    xy_gui_display_set_pixel(g_led_gui_display, 0, 0, XY_GUI_COLOR_BLUE);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_BLUE, g_framebuffer[0][0]);
}

static void test_gui_context_respects_disabled_led_gui_display_adapter(void)
{
    xy_led_driver_t led = make_led_driver();
    led.set_pixel = fake_led_set_pixel;
    led.show = fake_led_show;

    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&led));
    g_led_gui_display = xy_led_get_gui_interface(&led);
    TEST_ASSERT_NOT_NULL(g_led_gui_display);
    xy_led_enable_gui(&led, false);
    TEST_ASSERT_NULL(xy_led_get_gui_interface(&led));

    xy_gui_t gui;
    xy_gui_disp_drv_t drv = led_gui_adapter_driver();
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_init(&gui, FAKE_DISPLAY_WIDTH, FAKE_DISPLAY_HEIGHT, &drv));

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_pixel(&gui, 1, 1, XY_GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_fill_rect(&gui, 0, 0, 2, 2, XY_GUI_COLOR_GREEN));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_flush(&gui));
    TEST_ASSERT_EQUAL_UINT(0U, g_set_pixel_calls);
    TEST_ASSERT_EQUAL_UINT(0U, g_flush_calls);
    TEST_ASSERT_EQUAL_HEX16(0, g_framebuffer[1][1]);
}

static void test_led_gui_display_adapter_keeps_channels_isolated(void)
{
    xy_led_driver_t led_a = make_led_driver();
    xy_led_driver_t led_b = make_led_driver();
    led_a.set_pixel = fake_led_set_pixel;
    led_a.get_pixel = fake_led_get_pixel;
    led_a.show = fake_led_show;
    led_b.set_pixel = fake_alt_led_set_pixel;
    led_b.get_pixel = fake_alt_led_get_pixel;
    led_b.show = fake_alt_led_show;

    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&led_a));
    TEST_ASSERT_EQUAL_INT(0, xy_led_register_gui(&led_b));
    xy_gui_display_t *display_a = xy_led_get_gui_interface(&led_a);
    xy_gui_display_t *display_b = xy_led_get_gui_interface(&led_b);
    TEST_ASSERT_NOT_NULL(display_a);
    TEST_ASSERT_NOT_NULL(display_b);
    TEST_ASSERT_NOT_EQUAL(display_a, display_b);
    TEST_ASSERT_EQUAL_PTR(&led_a, display_a->user_data);
    TEST_ASSERT_EQUAL_PTR(&led_b, display_b->user_data);

    xy_gui_display_set_pixel(display_a, 2, 3, XY_GUI_COLOR_RED);
    xy_gui_display_set_pixel(display_b, 2, 3, XY_GUI_COLOR_GREEN);
    TEST_ASSERT_EQUAL_UINT(1U, g_set_pixel_calls);
    TEST_ASSERT_EQUAL_UINT(1U, g_alt_set_pixel_calls);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_RED, g_framebuffer[3][2]);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_GREEN, g_alt_framebuffer[3][2]);
    TEST_ASSERT_EQUAL_HEX16(0, g_framebuffer[0][0]);
    TEST_ASSERT_EQUAL_HEX16(0, g_alt_framebuffer[0][0]);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_RED, display_a->get_pixel(2, 3));
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_GREEN, display_b->get_pixel(2, 3));

    xy_gui_display_flush(display_a);
    TEST_ASSERT_EQUAL_UINT(1U, g_flush_calls);
    TEST_ASSERT_EQUAL_UINT(0U, g_alt_flush_calls);

    xy_led_enable_gui(&led_a, false);
    xy_gui_display_set_pixel(display_a, 4, 4, XY_GUI_COLOR_BLUE);
    xy_gui_display_set_pixel(display_b, 4, 4, XY_GUI_COLOR_YELLOW);
    xy_gui_display_flush(display_a);
    xy_gui_display_flush(display_b);
    TEST_ASSERT_EQUAL_UINT(1U, g_set_pixel_calls);
    TEST_ASSERT_EQUAL_UINT(2U, g_alt_set_pixel_calls);
    TEST_ASSERT_EQUAL_UINT(1U, g_flush_calls);
    TEST_ASSERT_EQUAL_UINT(1U, g_alt_flush_calls);
    TEST_ASSERT_EQUAL_HEX16(0, g_framebuffer[4][4]);
    TEST_ASSERT_EQUAL_HEX16(XY_GUI_COLOR_YELLOW, g_alt_framebuffer[4][4]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_gui_context_forwards_to_fake_display_backend);
    RUN_TEST(test_gui_backend_failures_propagate_without_committing_state);
    RUN_TEST(test_gui_backend_guard_paths_do_not_touch_fake_display);
    RUN_TEST(test_gui_fill_rect_fallback_clips_through_draw_pixel_guard);
    RUN_TEST(test_gui_context_can_drive_led_gui_display_adapter);
    RUN_TEST(test_led_gui_display_adapter_omits_get_pixel_when_backend_lacks_reader);
    RUN_TEST(test_gui_context_respects_disabled_led_gui_display_adapter);
    RUN_TEST(test_led_gui_display_adapter_keeps_channels_isolated);
    return UNITY_END();
}
