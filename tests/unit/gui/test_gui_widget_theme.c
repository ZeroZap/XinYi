#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "xy_gui_theme.h"
#include "xy_gui_widget.h"

static void assert_color(xy_gui_color_t color, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    assert(color.r == r);
    assert(color.g == g);
    assert(color.b == b);
    assert(color.a == a);
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

    assert(xy_gui_widget_init(NULL, XY_GUI_WIDGET_LABEL, 0, 0, 1, 1) == -1);
    assert(xy_gui_widget_init(&widget, XY_GUI_WIDGET_LABEL, 10, 20, 30, 40) == 0);
    assert(widget.type == XY_GUI_WIDGET_LABEL);
    assert(widget.state == XY_GUI_STATE_NORMAL);
    assert(widget.rect.x == 10 && widget.rect.y == 20);
    assert(widget.rect.width == 30 && widget.rect.height == 40);
    assert(widget.style.visible);
    assert(widget.style.enabled);
    assert(widget.min_value == 0 && widget.max_value == 100);
    assert(widget.need_redraw);

    assert(xy_gui_widget_set_pos(&widget, -2, 5) == 0);
    assert(widget.rect.x == -2 && widget.rect.y == 5);
    assert(xy_gui_widget_set_size(&widget, 11, 12) == 0);
    assert(widget.rect.width == 11 && widget.rect.height == 12);

    assert(xy_gui_widget_set_style(&widget, &style) == 0);
    assert(widget.style.border_width == 2);
    assert(widget.style.corner_radius == 3);
    assert_color(widget.style.bg_color, 0, 0, 255, 255);

    assert(xy_gui_widget_set_text(&widget, "abc") == 0);
    assert(strcmp(xy_gui_widget_get_text(&widget), "abc") == 0);
    assert(widget.text_len == 3);
    assert(xy_gui_widget_set_text(&widget, "xy") == 0);
    assert(strcmp(xy_gui_widget_get_text(&widget), "xy") == 0);
    assert(widget.text_len == 2);

    assert(xy_gui_widget_set_value(&widget, 42) == 0);
    assert(xy_gui_widget_get_value(&widget) == 42);
    assert(xy_gui_widget_hit_test(&widget, -2, 5));
    assert(xy_gui_widget_hit_test(&widget, 8, 16));
    assert(!xy_gui_widget_hit_test(&widget, 9, 16));
    assert(!xy_gui_widget_hit_test(&widget, -3, 5));

    assert(xy_gui_widget_set_visible(&widget, false) == 0);
    assert(!xy_gui_widget_hit_test(&widget, -2, 5));
    assert(xy_gui_widget_draw(&widget, NULL, 0, 0) == -1);
    assert(xy_gui_widget_set_visible(&widget, true) == 0);
    assert(xy_gui_widget_set_enabled(&widget, false) == 0);
    assert(widget.state == XY_GUI_STATE_DISABLED);
    assert(xy_gui_widget_update(&widget, NULL) == -1);
    assert(xy_gui_widget_set_enabled(&widget, true) == 0);
    assert(widget.state == XY_GUI_STATE_NORMAL);

    assert(xy_gui_widget_deinit(&widget) == 0);
    assert(widget.text == NULL);
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
    assert(xy_gui_theme_system_init() == 0);
    assert(xy_gui_theme_system_init() == -1);
    assert(xy_gui_theme_find("Light") != NULL);
    assert(xy_gui_theme_find("Dark") != NULL);
    assert(xy_gui_theme_is_dark_mode_supported());
    assert(xy_gui_theme_get_current() != NULL);
    assert(strcmp(xy_gui_theme_get_current()->name, "Light") == 0);

    assert(xy_gui_theme_set_dark() == 0);
    current = xy_gui_theme_get_current();
    assert(current != NULL && strcmp(current->name, "Dark") == 0);
    assert(current->is_active);
    assert(!xy_gui_theme_find("Light")->is_active);
    assert(xy_gui_theme_get_list(list, 4) == 2);

    xy_gui_theme_create_high_contrast(&custom);
    assert(strcmp(custom.name, "High Contrast") == 0);
    assert(custom.style.animation_enabled == 0);
    assert(xy_gui_theme_register(&custom) == 0);
    assert(xy_gui_theme_find("High Contrast") != NULL);
    assert(xy_gui_theme_get_list(list, 4) == 3);

    assert(xy_gui_theme_apply("High Contrast") == 0);
    current = xy_gui_theme_get_current();
    assert(current != NULL && strcmp(current->name, "High Contrast") == 0);

    assert(xy_gui_widget_init(&root, XY_GUI_WIDGET_CONTAINER, 0, 0, 100, 100) == 0);
    assert(xy_gui_widget_init(&child_a, XY_GUI_WIDGET_LABEL, 1, 1, 10, 10) == 0);
    assert(xy_gui_widget_init(&child_b, XY_GUI_WIDGET_BUTTON, 2, 2, 20, 10) == 0);
    assert(xy_gui_widget_add_child(&root, &child_a) == 0);
    assert(xy_gui_widget_add_child(&root, &child_b) == 0);
    assert(root.child == &child_a);
    assert(child_a.next == &child_b);
    assert(child_b.parent == &root);

    assert(xy_gui_theme_apply_to_all(&root, current) == 0);
    assert(root.style.border_width == current->style.border_width);
    assert(child_a.style.border_width == current->style.border_width);
    assert(child_b.style.corner_radius == current->style.corner_radius_medium);
    assert_color(child_a.style.fg_color, 255, 255, 255, 255);

    assert(xy_gui_widget_remove_child(&root, &child_a) == 0);
    assert(root.child == &child_b);
    assert(child_a.parent == NULL && child_a.next == NULL);
    assert(xy_gui_widget_remove_child(&root, &child_a) == -1);

    assert(xy_gui_theme_unregister("High Contrast") == 0);
    assert(xy_gui_theme_find("High Contrast") == NULL);
    assert(xy_gui_theme_apply("High Contrast") == -1);

    xy_gui_widget_deinit(&root);
    xy_gui_widget_deinit(&child_a);
    xy_gui_widget_deinit(&child_b);
    xy_gui_theme_system_deinit();
}

int main(void)
{
    test_widget_contracts();
    test_theme_contracts_and_widget_application();
    return 0;
}
