#include "unity.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "xy_gui_theme.h"
#include "xy_gui_widget.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void assert_color(xy_gui_color_t color, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    TEST_ASSERT_EQUAL_UINT8(r, color.r);
    TEST_ASSERT_EQUAL_UINT8(g, color.g);
    TEST_ASSERT_EQUAL_UINT8(b, color.b);
    TEST_ASSERT_EQUAL_UINT8(a, color.a);
}

static void test_widget_contracts(void)
{
    xy_gui_widget_t widget;
    xy_gui_style_t style = {
        .bg_color = XY_GUI_COLOR_BLUE,
        .fg_color = XY_GUI_COLOR_WHITE,
        .border_color = XY_GUI_COLOR_RED,
        .border_width = 2,
        .corner_radius = 3,
        .padding = 4,
        .align = XY_GUI_ALIGN_CENTER,
        .visible = true,
        .enabled = true,
    };

    TEST_ASSERT_EQUAL_INT(-1, xy_gui_widget_init(NULL, XY_GUI_WIDGET_LABEL, 0, 0, 1, 1));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_init(&widget, XY_GUI_WIDGET_LABEL, 10, 20, 30, 40));
    TEST_ASSERT_EQUAL(XY_GUI_WIDGET_LABEL, widget.type);
    TEST_ASSERT_EQUAL(XY_GUI_STATE_NORMAL, widget.state);
    TEST_ASSERT_EQUAL_INT16(10, widget.rect.x);
    TEST_ASSERT_EQUAL_INT16(20, widget.rect.y);
    TEST_ASSERT_EQUAL_INT16(30, widget.rect.width);
    TEST_ASSERT_EQUAL_INT16(40, widget.rect.height);
    TEST_ASSERT_TRUE(widget.style.visible);
    TEST_ASSERT_TRUE(widget.style.enabled);
    TEST_ASSERT_EQUAL_INT(0, widget.min_value);
    TEST_ASSERT_EQUAL_INT(100, widget.max_value);
    TEST_ASSERT_TRUE(widget.need_redraw);

    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_set_pos(&widget, -2, 5));
    TEST_ASSERT_EQUAL_INT16(-2, widget.rect.x);
    TEST_ASSERT_EQUAL_INT16(5, widget.rect.y);
    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_set_size(&widget, 11, 12));
    TEST_ASSERT_EQUAL_INT16(11, widget.rect.width);
    TEST_ASSERT_EQUAL_INT16(12, widget.rect.height);

    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_set_style(&widget, &style));
    TEST_ASSERT_EQUAL_UINT8(2, widget.style.border_width);
    TEST_ASSERT_EQUAL_UINT8(3, widget.style.corner_radius);
    assert_color(widget.style.bg_color, 0, 0, 255, 255);

    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_set_text(&widget, "abc"));
    TEST_ASSERT_EQUAL_STRING("abc", xy_gui_widget_get_text(&widget));
    TEST_ASSERT_EQUAL_size_t(3, widget.text_len);
    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_set_text(&widget, "xy"));
    TEST_ASSERT_EQUAL_STRING("xy", xy_gui_widget_get_text(&widget));
    TEST_ASSERT_EQUAL_size_t(2, widget.text_len);

    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_set_value(&widget, 42));
    TEST_ASSERT_EQUAL_INT(42, xy_gui_widget_get_value(&widget));
    TEST_ASSERT_TRUE(xy_gui_widget_hit_test(&widget, -2, 5));
    TEST_ASSERT_TRUE(xy_gui_widget_hit_test(&widget, 8, 16));
    TEST_ASSERT_FALSE(xy_gui_widget_hit_test(&widget, 9, 16));
    TEST_ASSERT_FALSE(xy_gui_widget_hit_test(&widget, -3, 5));

    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_set_visible(&widget, false));
    TEST_ASSERT_FALSE(xy_gui_widget_hit_test(&widget, -2, 5));
    TEST_ASSERT_EQUAL_INT(-1, xy_gui_widget_draw(&widget, NULL, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_set_visible(&widget, true));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_set_enabled(&widget, false));
    TEST_ASSERT_EQUAL(XY_GUI_STATE_DISABLED, widget.state);
    TEST_ASSERT_EQUAL_INT(-1, xy_gui_widget_update(&widget, NULL));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_set_enabled(&widget, true));
    TEST_ASSERT_EQUAL(XY_GUI_STATE_NORMAL, widget.state);

    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_deinit(&widget));
    TEST_ASSERT_NULL(widget.text);
}

static void test_theme_contracts_and_widget_application(void)
{
    xy_gui_theme_t custom;
    xy_gui_theme_t *list[4];
    xy_gui_theme_t *current;
    xy_gui_widget_t root;
    xy_gui_widget_t child_a;
    xy_gui_widget_t child_b;

    xy_gui_theme_system_deinit();
    TEST_ASSERT_EQUAL_INT(0, xy_gui_theme_system_init());
    TEST_ASSERT_EQUAL_INT(-1, xy_gui_theme_system_init());
    TEST_ASSERT_NOT_NULL(xy_gui_theme_find("Light"));
    TEST_ASSERT_NOT_NULL(xy_gui_theme_find("Dark"));
    TEST_ASSERT_TRUE(xy_gui_theme_is_dark_mode_supported());
    TEST_ASSERT_NOT_NULL(xy_gui_theme_get_current());
    TEST_ASSERT_EQUAL_STRING("Light", xy_gui_theme_get_current()->name);

    TEST_ASSERT_EQUAL_INT(0, xy_gui_theme_set_dark());
    current = xy_gui_theme_get_current();
    TEST_ASSERT_NOT_NULL(current);
    TEST_ASSERT_EQUAL_STRING("Dark", current->name);
    TEST_ASSERT_TRUE(current->is_active);
    TEST_ASSERT_FALSE(xy_gui_theme_find("Light")->is_active);
    TEST_ASSERT_EQUAL_INT(2, xy_gui_theme_get_list(list, 4));

    xy_gui_theme_create_high_contrast(&custom);
    TEST_ASSERT_EQUAL_STRING("High Contrast", custom.name);
    TEST_ASSERT_EQUAL_UINT8(0, custom.style.animation_enabled);
    TEST_ASSERT_EQUAL_INT(0, xy_gui_theme_register(&custom));
    TEST_ASSERT_NOT_NULL(xy_gui_theme_find("High Contrast"));
    TEST_ASSERT_EQUAL_INT(3, xy_gui_theme_get_list(list, 4));

    TEST_ASSERT_EQUAL_INT(0, xy_gui_theme_apply("High Contrast"));
    current = xy_gui_theme_get_current();
    TEST_ASSERT_NOT_NULL(current);
    TEST_ASSERT_EQUAL_STRING("High Contrast", current->name);

    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_init(&root, XY_GUI_WIDGET_CONTAINER, 0, 0, 100, 100));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_init(&child_a, XY_GUI_WIDGET_LABEL, 1, 1, 10, 10));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_init(&child_b, XY_GUI_WIDGET_BUTTON, 2, 2, 20, 10));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_add_child(&root, &child_a));
    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_add_child(&root, &child_b));
    TEST_ASSERT_EQUAL_PTR(&child_a, root.child);
    TEST_ASSERT_EQUAL_PTR(&child_b, child_a.next);
    TEST_ASSERT_EQUAL_PTR(&root, child_b.parent);

    TEST_ASSERT_EQUAL_INT(0, xy_gui_theme_apply_to_all(&root, current));
    TEST_ASSERT_EQUAL_UINT8(current->style.border_width, root.style.border_width);
    TEST_ASSERT_EQUAL_UINT8(current->style.border_width, child_a.style.border_width);
    TEST_ASSERT_EQUAL_UINT8(current->style.corner_radius_medium, child_b.style.corner_radius);
    assert_color(child_a.style.fg_color, 255, 255, 255, 255);

    TEST_ASSERT_EQUAL_INT(0, xy_gui_widget_remove_child(&root, &child_a));
    TEST_ASSERT_EQUAL_PTR(&child_b, root.child);
    TEST_ASSERT_NULL(child_a.parent);
    TEST_ASSERT_NULL(child_a.next);
    TEST_ASSERT_EQUAL_INT(-1, xy_gui_widget_remove_child(&root, &child_a));

    TEST_ASSERT_EQUAL_INT(0, xy_gui_theme_unregister("High Contrast"));
    TEST_ASSERT_NULL(xy_gui_theme_find("High Contrast"));
    TEST_ASSERT_EQUAL_INT(-1, xy_gui_theme_apply("High Contrast"));

    xy_gui_widget_deinit(&root);
    xy_gui_widget_deinit(&child_a);
    xy_gui_widget_deinit(&child_b);
    xy_gui_theme_system_deinit();
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_widget_contracts);
    RUN_TEST(test_theme_contracts_and_widget_application);
    return UNITY_END();
}
