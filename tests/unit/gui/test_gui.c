/**
 * @file test_gui.c
 * @brief GUI Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"
#include "xy_gui.h"

/* ==================== Test Fixtures ==================== */

static xy_gui_t test_gui;
static xy_gui_disp_drv_t mock_drv;

void setUp(void)
{
    memset(&test_gui, 0, sizeof(test_gui));
    memset(&mock_drv, 0, sizeof(mock_drv));
}

void tearDown(void)
{
    xy_gui_deinit(&test_gui);
}

/* ==================== GUI Init Tests ==================== */

void test_gui_init(void)
{
    int result = xy_gui_init(&test_gui, 128, 64, &mock_drv);
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
    TEST_ASSERT_EQUAL(128, test_gui.width);
    TEST_ASSERT_EQUAL(64, test_gui.height);
    TEST_ASSERT_TRUE(test_gui.initialized);
}

void test_gui_init_invalid_params(void)
{
    int result;
    
    result = xy_gui_init(NULL, 128, 64, &mock_drv);
    TEST_ASSERT_EQUAL(XY_GUI_INVALID_PARAM, result);
    
    result = xy_gui_init(&test_gui, 128, 64, NULL);
    TEST_ASSERT_EQUAL(XY_GUI_INVALID_PARAM, result);
}

/* ==================== GUI Clear Tests ==================== */

void test_gui_clear(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    int result = xy_gui_clear(&test_gui, GUI_COLOR_WHITE);
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
    TEST_ASSERT_EQUAL(GUI_COLOR_WHITE, test_gui.bg_color);
}

/* ==================== GUI Drawing Tests ==================== */

void test_gui_draw_pixel(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    int result = xy_gui_draw_pixel(&test_gui, 0, 0, GUI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
    
    /* Out of bounds */
    result = xy_gui_draw_pixel(&test_gui, 128, 0, GUI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(XY_GUI_INVALID_PARAM, result);
}

void test_gui_draw_line(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    int result = xy_gui_draw_line(&test_gui, 0, 0, 10, 10, GUI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
}

void test_gui_draw_rect(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    int result = xy_gui_draw_rect(&test_gui, 0, 0, 10, 10, GUI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
}

void test_gui_fill_rect(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    int result = xy_gui_fill_rect(&test_gui, 0, 0, 10, 10, GUI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
}

void test_gui_draw_char(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    int result = xy_gui_draw_char(&test_gui, 0, 0, 'A', GUI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
}

void test_gui_draw_string(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    int result = xy_gui_draw_string(&test_gui, 0, 0, "Hello", GUI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
}

/* ==================== GUI Object Tests ==================== */

void test_gui_obj_create(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    xy_gui_rect_t rect = {0, 0, 50, 20};
    xy_gui_obj_t *obj = xy_gui_obj_create(&test_gui, XY_GUI_OBJ_LABEL, &rect);
    
    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT_EQUAL(1, test_gui.obj_count);
    TEST_ASSERT_EQUAL(XY_GUI_OBJ_LABEL, obj->type);
}

void test_gui_obj_delete(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    xy_gui_rect_t rect = {0, 0, 50, 20};
    xy_gui_obj_t *obj = xy_gui_obj_create(&test_gui, XY_GUI_OBJ_LABEL, &rect);
    
    int result = xy_gui_obj_delete(&test_gui, obj);
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
    TEST_ASSERT_EQUAL(0, test_gui.obj_count);
}

void test_gui_obj_set_text(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    xy_gui_rect_t rect = {0, 0, 50, 20};
    xy_gui_obj_t *obj = xy_gui_obj_create(&test_gui, XY_GUI_OBJ_LABEL, &rect);
    
    int result = xy_gui_obj_set_text(&test_gui, obj, "Test");
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
    TEST_ASSERT_EQUAL_STRING("Test", obj->text);
}

void test_gui_obj_set_color(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    xy_gui_rect_t rect = {0, 0, 50, 20};
    xy_gui_obj_t *obj = xy_gui_obj_create(&test_gui, XY_GUI_OBJ_LABEL, &rect);
    
    int result = xy_gui_obj_set_color(&test_gui, obj, GUI_COLOR_WHITE, GUI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
    TEST_ASSERT_EQUAL(GUI_COLOR_WHITE, obj->fg_color.color);
    TEST_ASSERT_EQUAL(GUI_COLOR_BLACK, obj->bg_color.color);
}

void test_gui_obj_set_visible(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    xy_gui_rect_t rect = {0, 0, 50, 20};
    xy_gui_obj_t *obj = xy_gui_obj_create(&test_gui, XY_GUI_OBJ_LABEL, &rect);
    
    int result = xy_gui_obj_set_visible(&test_gui, obj, false);
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
    TEST_ASSERT_FALSE(obj->visible);
}

void test_gui_obj_redraw(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    xy_gui_rect_t rect = {0, 0, 50, 20};
    xy_gui_obj_t *obj = xy_gui_obj_create(&test_gui, XY_GUI_OBJ_LABEL, &rect);
    xy_gui_obj_set_text(&test_gui, obj, "Test");
    
    int result = xy_gui_obj_redraw(&test_gui, obj);
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
}

/* ==================== GUI Flush Tests ==================== */

void test_gui_flush(void)
{
    xy_gui_init(&test_gui, 128, 64, &mock_drv);
    
    int result = xy_gui_flush(&test_gui);
    TEST_ASSERT_EQUAL(XY_GUI_OK, result);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Init Tests */
    RUN_TEST(test_gui_init);
    RUN_TEST(test_gui_init_invalid_params);

    /* Clear Tests */
    RUN_TEST(test_gui_clear);

    /* Drawing Tests */
    RUN_TEST(test_gui_draw_pixel);
    RUN_TEST(test_gui_draw_line);
    RUN_TEST(test_gui_draw_rect);
    RUN_TEST(test_gui_fill_rect);
    RUN_TEST(test_gui_draw_char);
    RUN_TEST(test_gui_draw_string);

    /* Object Tests */
    RUN_TEST(test_gui_obj_create);
    RUN_TEST(test_gui_obj_delete);
    RUN_TEST(test_gui_obj_set_text);
    RUN_TEST(test_gui_obj_set_color);
    RUN_TEST(test_gui_obj_set_visible);
    RUN_TEST(test_gui_obj_redraw);

    /* Flush Tests */
    RUN_TEST(test_gui_flush);

    return UNITY_END();
}
