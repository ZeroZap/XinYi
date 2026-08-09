#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "xy_gui.h"

#define FAKE_DISPLAY_WIDTH  16U
#define FAKE_DISPLAY_HEIGHT 8U

static uint16_t g_framebuffer[FAKE_DISPLAY_HEIGHT][FAKE_DISPLAY_WIDTH];
static unsigned int g_init_calls;
static unsigned int g_set_pixel_calls;
static unsigned int g_fill_rect_calls;
static unsigned int g_flush_calls;
static int16_t g_last_x;
static int16_t g_last_y;
static int16_t g_last_w;
static int16_t g_last_h;
static uint16_t g_last_color;
static bool g_backend_fail;

void setUp(void)
{
    memset(g_framebuffer, 0, sizeof(g_framebuffer));
    g_init_calls = 0;
    g_set_pixel_calls = 0;
    g_fill_rect_calls = 0;
    g_flush_calls = 0;
    g_last_x = -1;
    g_last_y = -1;
    g_last_w = -1;
    g_last_h = -1;
    g_last_color = 0;
    g_backend_fail = false;
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

static void test_gui_backend_failures_are_currently_normalized_to_success(void)
{
    xy_gui_t gui;
    xy_gui_disp_drv_t drv = fake_display_driver();

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_init(&gui, FAKE_DISPLAY_WIDTH, FAKE_DISPLAY_HEIGHT, &drv));
    g_backend_fail = true;

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_clear(&gui, XY_GUI_COLOR_WHITE));
    TEST_ASSERT_EQUAL_UINT(1U, g_fill_rect_calls);
    TEST_ASSERT_EQUAL_HEX16(0, g_framebuffer[0][0]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_pixel(&gui, 1, 1, XY_GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_UINT(1U, g_set_pixel_calls);
    TEST_ASSERT_EQUAL_HEX16(0, g_framebuffer[1][1]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_fill_rect(&gui, 0, 0, 2, 2, XY_GUI_COLOR_GREEN));
    TEST_ASSERT_EQUAL_UINT(2U, g_fill_rect_calls);
    TEST_ASSERT_EQUAL_HEX16(0, g_framebuffer[0][0]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_flush(&gui));
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
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_fill_rect(&gui, 0, 0, 2, 2, XY_GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_UINT(8U, g_set_pixel_calls);
    TEST_ASSERT_EQUAL_HEX16(0, g_framebuffer[0][0]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_gui_context_forwards_to_fake_display_backend);
    RUN_TEST(test_gui_backend_failures_are_currently_normalized_to_success);
    RUN_TEST(test_gui_backend_guard_paths_do_not_touch_fake_display);
    RUN_TEST(test_gui_fill_rect_fallback_clips_through_draw_pixel_guard);
    return UNITY_END();
}
