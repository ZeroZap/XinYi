#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "xy_gui.h"

static int g_init_count;
static int g_flush_count;
static int g_pixel_count;
static int g_line_count;
static int g_rect_count;
static int g_fill_count;
static int g_char_count;
static int16_t g_last_x;
static int16_t g_last_y;
static int16_t g_last_w;
static int16_t g_last_h;
static uint16_t g_last_color;

static void reset_mock(void)
{
    g_init_count = 0;
    g_flush_count = 0;
    g_pixel_count = 0;
    g_line_count = 0;
    g_rect_count = 0;
    g_fill_count = 0;
    g_char_count = 0;
    g_last_x = 0;
    g_last_y = 0;
    g_last_w = 0;
    g_last_h = 0;
    g_last_color = 0;
}

static int mock_init(void)
{
    g_init_count++;
    return XY_GUI_OK;
}

static int mock_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
    g_pixel_count++;
    g_last_x = x;
    g_last_y = y;
    g_last_color = color;
    return XY_GUI_OK;
}

static int mock_draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
    (void)x2;
    (void)y2;
    g_line_count++;
    g_last_x = x1;
    g_last_y = y1;
    g_last_color = color;
    return XY_GUI_OK;
}

static int mock_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    g_rect_count++;
    g_last_x = x;
    g_last_y = y;
    g_last_w = w;
    g_last_h = h;
    g_last_color = color;
    return XY_GUI_OK;
}

static int mock_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    g_fill_count++;
    g_last_x = x;
    g_last_y = y;
    g_last_w = w;
    g_last_h = h;
    g_last_color = color;
    return XY_GUI_OK;
}

static int mock_draw_char(int16_t x, int16_t y, char c, uint16_t color)
{
    (void)c;
    g_char_count++;
    g_last_x = x;
    g_last_y = y;
    g_last_color = color;
    return XY_GUI_OK;
}

static int mock_flush(void)
{
    g_flush_count++;
    return XY_GUI_OK;
}

static xy_gui_disp_drv_t mock_driver(void)
{
    xy_gui_disp_drv_t drv = {
        .init = mock_init,
        .draw_pixel = mock_draw_pixel,
        .draw_line = mock_draw_line,
        .draw_rect = mock_draw_rect,
        .fill_rect = mock_fill_rect,
        .draw_char = mock_draw_char,
        .flush = mock_flush,
    };
    return drv;
}

static void test_lifecycle_clear_flush_and_bounds(void)
{
    xy_gui_t gui;
    xy_gui_disp_drv_t drv = mock_driver();

    reset_mock();
    assert(xy_gui_init(NULL, 64, 32, &drv) == XY_GUI_INVALID_PARAM);
    assert(xy_gui_init(&gui, 64, 32, NULL) == XY_GUI_INVALID_PARAM);
    assert(xy_gui_init(&gui, 64, 32, &drv) == XY_GUI_OK);
    assert(gui.initialized);
    assert(gui.width == 64);
    assert(gui.height == 32);
    assert(g_init_count == 1);

    assert(xy_gui_clear(&gui, GUI_COLOR_BLUE) == XY_GUI_OK);
    assert(gui.bg_color == GUI_COLOR_BLUE);
    assert(g_fill_count == 1);
    assert(g_last_w == 64 && g_last_h == 32);

    assert(xy_gui_draw_pixel(&gui, 63, 31, GUI_COLOR_RED) == XY_GUI_OK);
    assert(g_pixel_count == 1);
    assert(g_last_x == 63 && g_last_y == 31);
    assert(xy_gui_draw_pixel(&gui, 64, 0, GUI_COLOR_RED) == XY_GUI_INVALID_PARAM);
    assert(xy_gui_draw_pixel(&gui, -1, 0, GUI_COLOR_RED) == XY_GUI_INVALID_PARAM);

    assert(xy_gui_flush(&gui) == XY_GUI_OK);
    assert(g_flush_count == 1);
    assert(xy_gui_deinit(&gui) == XY_GUI_OK);
    assert(!gui.initialized);
    assert(xy_gui_clear(&gui, GUI_COLOR_BLACK) == XY_GUI_INVALID_PARAM);
}

static void test_drawing_and_string_callbacks(void)
{
    xy_gui_t gui;
    xy_gui_disp_drv_t drv = mock_driver();

    reset_mock();
    assert(xy_gui_init(&gui, 64, 32, &drv) == XY_GUI_OK);
    assert(xy_gui_draw_line(&gui, 0, 0, 3, 0, GUI_COLOR_GREEN) == XY_GUI_OK);
    assert(g_pixel_count == 4);
    assert(xy_gui_draw_rect(&gui, 1, 2, 4, 5, GUI_COLOR_WHITE) == XY_GUI_OK);
    assert(g_pixel_count > 4);
    assert(xy_gui_fill_rect(&gui, 2, 3, 5, 6, GUI_COLOR_CYAN) == XY_GUI_OK);
    assert(g_fill_count == 1);
    assert(g_last_x == 2 && g_last_y == 3 && g_last_w == 5 && g_last_h == 6);
    assert(xy_gui_draw_char(&gui, 0, 0, 'A', GUI_COLOR_WHITE) == XY_GUI_OK);
    assert(g_pixel_count > 4);
    assert(xy_gui_draw_string(&gui, 0, 0, "Hi", GUI_COLOR_WHITE) == XY_GUI_OK);
    assert(xy_gui_draw_string(&gui, 0, 0, NULL, GUI_COLOR_WHITE) == XY_GUI_INVALID_PARAM);
}

static void test_object_lifecycle_properties_and_redraw(void)
{
    xy_gui_t gui;
    xy_gui_disp_drv_t drv = mock_driver();
    xy_gui_rect_t rect = {1, 2, 20, 8};
    xy_gui_obj_t *label;
    xy_gui_obj_t *button;
    xy_gui_obj_t external = {0};

    reset_mock();
    assert(xy_gui_init(&gui, 80, 40, &drv) == XY_GUI_OK);
    label = xy_gui_obj_create(&gui, XY_GUI_OBJ_LABEL, &rect);
    assert(label != NULL);
    assert(gui.obj_count == 1);
    assert(label->visible);
    assert(label->enabled);
    assert(label->fg_color.color == GUI_COLOR_BLACK);
    assert(label->bg_color.color == GUI_COLOR_WHITE);

    assert(xy_gui_obj_set_text(&gui, label, "OK") == XY_GUI_OK);
    assert(label->text != NULL && strcmp(label->text, "OK") == 0);
    assert(xy_gui_obj_set_color(&gui, label, GUI_COLOR_RED, GUI_COLOR_BLUE) == XY_GUI_OK);
    assert(label->fg_color.color == GUI_COLOR_RED);
    assert(label->bg_color.color == GUI_COLOR_BLUE);

    assert(xy_gui_obj_set_visible(&gui, label, false) == XY_GUI_INVALID_PARAM);
    assert(!label->visible);
    assert(xy_gui_obj_redraw(&gui, label) == XY_GUI_INVALID_PARAM);
    assert(xy_gui_obj_set_visible(&gui, label, true) == XY_GUI_OK);
    assert(label->visible);

    button = xy_gui_obj_create(&gui, XY_GUI_OBJ_BUTTON, &rect);
    assert(button != NULL);
    assert(gui.obj_count == 2);
    assert(xy_gui_obj_set_text(&gui, button, "B") == XY_GUI_OK);
    assert(g_fill_count >= 1);

    assert(xy_gui_obj_delete(&gui, &external) == XY_GUI_NOT_FOUND);
    assert(xy_gui_obj_delete(&gui, label) == XY_GUI_OK);
    assert(gui.obj_count == 1);
    assert(gui.objects[0].type == XY_GUI_OBJ_BUTTON);
    assert(xy_gui_obj_delete(NULL, button) == XY_GUI_INVALID_PARAM);
}

int main(void)
{
    test_lifecycle_clear_flush_and_bounds();
    test_drawing_and_string_callbacks();
    test_object_lifecycle_properties_and_redraw();
    return 0;
}
