#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"
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

void setUp(void)
{
}

void tearDown(void)
{
}

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
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_init(NULL, 64, 32, &drv));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_init(&gui, 64, 32, NULL));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_init(&gui, 64, 32, &drv));
    TEST_ASSERT_TRUE(gui.initialized);
    TEST_ASSERT_EQUAL_INT16(64, gui.width);
    TEST_ASSERT_EQUAL_INT16(32, gui.height);
    TEST_ASSERT_EQUAL_INT(1, g_init_count);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_clear(&gui, GUI_COLOR_BLUE));
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_BLUE, gui.bg_color);
    TEST_ASSERT_EQUAL_INT(1, g_fill_count);
    TEST_ASSERT_EQUAL_INT16(64, g_last_w);
    TEST_ASSERT_EQUAL_INT16(32, g_last_h);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_pixel(&gui, 63, 31, GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_INT(1, g_pixel_count);
    TEST_ASSERT_EQUAL_INT16(63, g_last_x);
    TEST_ASSERT_EQUAL_INT16(31, g_last_y);
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_draw_pixel(&gui, 64, 0, GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_draw_pixel(&gui, -1, 0, GUI_COLOR_RED));

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_flush(&gui));
    TEST_ASSERT_EQUAL_INT(1, g_flush_count);
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_deinit(&gui));
    TEST_ASSERT_FALSE(gui.initialized);
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_clear(&gui, GUI_COLOR_BLACK));
}

static void test_drawing_and_string_callbacks(void)
{
    xy_gui_t gui;
    xy_gui_disp_drv_t drv = mock_driver();

    reset_mock();
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_init(&gui, 64, 32, &drv));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_line(&gui, 0, 0, 3, 0, GUI_COLOR_GREEN));
    TEST_ASSERT_EQUAL_INT(4, g_pixel_count);
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_rect(&gui, 1, 2, 4, 5, GUI_COLOR_WHITE));
    TEST_ASSERT_GREATER_THAN_INT(4, g_pixel_count);
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_fill_rect(&gui, 2, 3, 5, 6, GUI_COLOR_CYAN));
    TEST_ASSERT_EQUAL_INT(1, g_fill_count);
    TEST_ASSERT_EQUAL_INT16(2, g_last_x);
    TEST_ASSERT_EQUAL_INT16(3, g_last_y);
    TEST_ASSERT_EQUAL_INT16(5, g_last_w);
    TEST_ASSERT_EQUAL_INT16(6, g_last_h);
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_char(&gui, 0, 0, 'A', GUI_COLOR_WHITE));
    TEST_ASSERT_GREATER_THAN_INT(4, g_pixel_count);
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_string(&gui, 0, 0, "Hi", GUI_COLOR_WHITE));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_draw_string(&gui, 0, 0, NULL, GUI_COLOR_WHITE));
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
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_init(&gui, 80, 40, &drv));
    label = xy_gui_obj_create(&gui, XY_GUI_OBJ_LABEL, &rect);
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_EQUAL_INT(1, gui.obj_count);
    TEST_ASSERT_TRUE(label->visible);
    TEST_ASSERT_TRUE(label->enabled);
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_BLACK, label->fg_color.color);
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_WHITE, label->bg_color.color);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_obj_set_text(&gui, label, "OK"));
    TEST_ASSERT_NOT_NULL(label->text);
    TEST_ASSERT_EQUAL_STRING("OK", label->text);
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_obj_set_color(&gui, label, GUI_COLOR_RED, GUI_COLOR_BLUE));
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_RED, label->fg_color.color);
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_BLUE, label->bg_color.color);

    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_obj_set_visible(&gui, label, false));
    TEST_ASSERT_FALSE(label->visible);
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_obj_redraw(&gui, label));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_obj_set_visible(&gui, label, true));
    TEST_ASSERT_TRUE(label->visible);

    button = xy_gui_obj_create(&gui, XY_GUI_OBJ_BUTTON, &rect);
    TEST_ASSERT_NOT_NULL(button);
    TEST_ASSERT_EQUAL_INT(2, gui.obj_count);
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_obj_set_text(&gui, button, "B"));
    TEST_ASSERT_GREATER_OR_EQUAL_INT(1, g_fill_count);

    TEST_ASSERT_EQUAL_INT(XY_GUI_NOT_FOUND, xy_gui_obj_delete(&gui, &external));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_obj_delete(&gui, label));
    TEST_ASSERT_EQUAL_INT(1, gui.obj_count);
    TEST_ASSERT_EQUAL_INT(XY_GUI_OBJ_BUTTON, gui.objects[0].type);
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_obj_delete(NULL, button));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lifecycle_clear_flush_and_bounds);
    RUN_TEST(test_drawing_and_string_callbacks);
    RUN_TEST(test_object_lifecycle_properties_and_redraw);
    return UNITY_END();
}
