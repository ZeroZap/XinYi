#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "fff.h"
#include "xy_gui.h"

static int16_t g_last_x;
static int16_t g_last_y;
static int16_t g_last_w;
static int16_t g_last_h;
static uint16_t g_last_color;

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, mock_init)
FAKE_VALUE_FUNC(int, mock_draw_pixel, int16_t, int16_t, uint16_t)
FAKE_VALUE_FUNC(int, mock_draw_line, int16_t, int16_t, int16_t, int16_t, uint16_t)
FAKE_VALUE_FUNC(int, mock_draw_rect, int16_t, int16_t, int16_t, int16_t, uint16_t)
FAKE_VALUE_FUNC(int, mock_fill_rect, int16_t, int16_t, int16_t, int16_t, uint16_t)
FAKE_VALUE_FUNC(int, mock_draw_char, int16_t, int16_t, char, uint16_t)
FAKE_VALUE_FUNC(int, mock_flush)

void setUp(void)
{
    RESET_FAKE(mock_init);
    RESET_FAKE(mock_draw_pixel);
    RESET_FAKE(mock_draw_line);
    RESET_FAKE(mock_draw_rect);
    RESET_FAKE(mock_fill_rect);
    RESET_FAKE(mock_draw_char);
    RESET_FAKE(mock_flush);
    FFF_RESET_HISTORY();

    mock_init_fake.return_val = XY_GUI_OK;
    mock_draw_pixel_fake.return_val = XY_GUI_OK;
    mock_draw_line_fake.return_val = XY_GUI_OK;
    mock_draw_rect_fake.return_val = XY_GUI_OK;
    mock_fill_rect_fake.return_val = XY_GUI_OK;
    mock_draw_char_fake.return_val = XY_GUI_OK;
    mock_flush_fake.return_val = XY_GUI_OK;

    g_last_x = 0;
    g_last_y = 0;
    g_last_w = 0;
    g_last_h = 0;
    g_last_color = 0;
}

void tearDown(void)
{
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

static void capture_pixel_from_fake(void)
{
    g_last_x = mock_draw_pixel_fake.arg0_val;
    g_last_y = mock_draw_pixel_fake.arg1_val;
    g_last_color = mock_draw_pixel_fake.arg2_val;
}

static void capture_fill_rect_from_fake(void)
{
    g_last_x = mock_fill_rect_fake.arg0_val;
    g_last_y = mock_fill_rect_fake.arg1_val;
    g_last_w = mock_fill_rect_fake.arg2_val;
    g_last_h = mock_fill_rect_fake.arg3_val;
    g_last_color = mock_fill_rect_fake.arg4_val;
}

static void capture_char_from_history(unsigned int history_index)
{
    g_last_x = mock_draw_pixel_fake.arg0_history[history_index];
    g_last_y = mock_draw_pixel_fake.arg1_history[history_index];
    g_last_color = mock_draw_pixel_fake.arg2_history[history_index];
}

static void test_lifecycle_clear_flush_and_bounds(void)
{
    xy_gui_t gui;
    xy_gui_disp_drv_t drv = mock_driver();

    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_init(NULL, 64, 32, &drv));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_init(&gui, 64, 32, NULL));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_init(&gui, 64, 32, &drv));
    TEST_ASSERT_TRUE(gui.initialized);
    TEST_ASSERT_EQUAL_INT16(64, gui.width);
    TEST_ASSERT_EQUAL_INT16(32, gui.height);
    TEST_ASSERT_EQUAL_UINT(1U, mock_init_fake.call_count);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_clear(&gui, GUI_COLOR_BLUE));
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_BLUE, gui.bg_color);
    TEST_ASSERT_EQUAL_UINT(1U, mock_fill_rect_fake.call_count);
    TEST_ASSERT_EQUAL_INT16(0, mock_fill_rect_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT16(0, mock_fill_rect_fake.arg1_val);
    TEST_ASSERT_EQUAL_INT16(64, mock_fill_rect_fake.arg2_val);
    TEST_ASSERT_EQUAL_INT16(32, mock_fill_rect_fake.arg3_val);
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_BLUE, mock_fill_rect_fake.arg4_val);
    capture_fill_rect_from_fake();
    TEST_ASSERT_EQUAL_INT16(64, g_last_w);
    TEST_ASSERT_EQUAL_INT16(32, g_last_h);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_pixel(&gui, 63, 31, GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_UINT(1U, mock_draw_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_INT16(63, mock_draw_pixel_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT16(31, mock_draw_pixel_fake.arg1_val);
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_RED, mock_draw_pixel_fake.arg2_val);
    capture_pixel_from_fake();
    TEST_ASSERT_EQUAL_INT16(63, g_last_x);
    TEST_ASSERT_EQUAL_INT16(31, g_last_y);
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_draw_pixel(&gui, 64, 0, GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_draw_pixel(&gui, -1, 0, GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_UINT(1U, mock_draw_pixel_fake.call_count);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_flush(&gui));
    TEST_ASSERT_EQUAL_UINT(1U, mock_flush_fake.call_count);
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_deinit(&gui));
    TEST_ASSERT_FALSE(gui.initialized);
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_clear(&gui, GUI_COLOR_BLACK));
}

static void test_drawing_and_string_callbacks(void)
{
    xy_gui_t gui;
    xy_gui_disp_drv_t drv = mock_driver();
    unsigned int pixel_count_after_line;
    unsigned int pixel_count_after_rect;
    unsigned int pixel_count_after_char;

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_init(&gui, 64, 32, &drv));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_line(&gui, 0, 0, 3, 0, GUI_COLOR_GREEN));
    TEST_ASSERT_EQUAL_UINT(4U, mock_draw_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_INT16(0, mock_draw_pixel_fake.arg0_history[0]);
    TEST_ASSERT_EQUAL_INT16(3, mock_draw_pixel_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT16(0, mock_draw_pixel_fake.arg1_val);
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_GREEN, mock_draw_pixel_fake.arg2_val);
    pixel_count_after_line = mock_draw_pixel_fake.call_count;

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_rect(&gui, 1, 2, 4, 5, GUI_COLOR_WHITE));
    TEST_ASSERT_GREATER_THAN_UINT(pixel_count_after_line, mock_draw_pixel_fake.call_count);
    pixel_count_after_rect = mock_draw_pixel_fake.call_count;

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_fill_rect(&gui, 2, 3, 5, 6, GUI_COLOR_CYAN));
    TEST_ASSERT_EQUAL_UINT(1U, mock_fill_rect_fake.call_count);
    TEST_ASSERT_EQUAL_INT16(2, mock_fill_rect_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT16(3, mock_fill_rect_fake.arg1_val);
    TEST_ASSERT_EQUAL_INT16(5, mock_fill_rect_fake.arg2_val);
    TEST_ASSERT_EQUAL_INT16(6, mock_fill_rect_fake.arg3_val);
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_CYAN, mock_fill_rect_fake.arg4_val);
    capture_fill_rect_from_fake();
    TEST_ASSERT_EQUAL_INT16(2, g_last_x);
    TEST_ASSERT_EQUAL_INT16(3, g_last_y);
    TEST_ASSERT_EQUAL_INT16(5, g_last_w);
    TEST_ASSERT_EQUAL_INT16(6, g_last_h);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_char(&gui, 0, 0, '0', GUI_COLOR_WHITE));
    TEST_ASSERT_GREATER_THAN_UINT(pixel_count_after_rect, mock_draw_pixel_fake.call_count);
    pixel_count_after_char = mock_draw_pixel_fake.call_count;
    capture_char_from_history(pixel_count_after_char - 1U);
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_WHITE, g_last_color);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_draw_string(&gui, 0, 0, "09", GUI_COLOR_WHITE));
    TEST_ASSERT_GREATER_THAN_UINT(pixel_count_after_char, mock_draw_pixel_fake.call_count);
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
    unsigned int pixel_count_after_label_text;

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_init(&gui, 80, 40, &drv));
    label = xy_gui_obj_create(&gui, XY_GUI_OBJ_LABEL, &rect);
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_EQUAL_INT(1, gui.obj_count);
    TEST_ASSERT_TRUE(label->visible);
    TEST_ASSERT_TRUE(label->enabled);
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_BLACK, label->fg_color.color);
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_WHITE, label->bg_color.color);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_obj_set_text(&gui, label, "09"));
    TEST_ASSERT_NOT_NULL(label->text);
    TEST_ASSERT_EQUAL_STRING("09", label->text);
    TEST_ASSERT_GREATER_THAN_UINT(0U, mock_draw_pixel_fake.call_count);
    pixel_count_after_label_text = mock_draw_pixel_fake.call_count;
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_obj_set_color(&gui, label, GUI_COLOR_RED, GUI_COLOR_BLUE));
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_RED, label->fg_color.color);
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_BLUE, label->bg_color.color);
    TEST_ASSERT_GREATER_THAN_UINT(pixel_count_after_label_text, mock_draw_pixel_fake.call_count);
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_RED, mock_draw_pixel_fake.arg2_val);

    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_obj_set_visible(&gui, label, false));
    TEST_ASSERT_FALSE(label->visible);
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_obj_redraw(&gui, label));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_obj_set_visible(&gui, label, true));
    TEST_ASSERT_TRUE(label->visible);

    button = xy_gui_obj_create(&gui, XY_GUI_OBJ_BUTTON, &rect);
    TEST_ASSERT_NOT_NULL(button);
    TEST_ASSERT_EQUAL_INT(2, gui.obj_count);
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_obj_set_text(&gui, button, "B"));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(1U, mock_fill_rect_fake.call_count);
    TEST_ASSERT_EQUAL_INT16(rect.x, mock_fill_rect_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT16(rect.y, mock_fill_rect_fake.arg1_val);
    TEST_ASSERT_EQUAL_INT16(rect.width, mock_fill_rect_fake.arg2_val);
    TEST_ASSERT_EQUAL_INT16(rect.height, mock_fill_rect_fake.arg3_val);
    TEST_ASSERT_EQUAL_HEX16(GUI_COLOR_WHITE, mock_fill_rect_fake.arg4_val);

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
