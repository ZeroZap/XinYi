/**
 * @file main.c
 * @brief XinYi Framework - GUI Stage 2 Complete Test
 * @version 1.0.0
 * @date 2026-03-14
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* OSAL */
#include "xy_os.h"

/* GUI Core */
#include "xy_gui_widget.h"
#include "xy_gui_event.h"
#include "xy_gui_layout.h"
#include "xy_gui_theme.h"

static void print_result(const char *name, bool pass)
{
    printf("  [ %s ] %s\n", pass ? "OK" : "FAIL", name);
}

int main(void)
{
    printf("==========================================\n");
    printf("XinYi Framework - GUI Stage 2 Test\n");
    printf("==========================================\n\n");
    
    int pass = 0, fail = 0;
    
    /* ========== 事件系统测试 ========== */
    printf("=== GUI Event System ===\n");
    
    int event_ret = xy_gui_event_system_init();
    print_result("xy_gui_event_system_init", event_ret == 0);
    (event_ret == 0) ? pass++ : fail++;
    
    xy_gui_event_queue_t queue;
    event_ret = xy_gui_event_queue_init(&queue);
    print_result("xy_gui_event_queue_init", event_ret == 0);
    (event_ret == 0) ? pass++ : fail++;
    
    xy_gui_event_t event = xy_gui_event_create_click(50, 50);
    print_result("xy_gui_event_create_click", event.type == XY_GUI_EVENT_CLICK);
    (event.type == XY_GUI_EVENT_CLICK) ? pass++ : fail++;
    
    event = xy_gui_event_create_touch(XY_GUI_EVENT_TOUCH_DOWN, 100, 100);
    print_result("xy_gui_event_create_touch", event.type == XY_GUI_EVENT_TOUCH_DOWN);
    (event.type == XY_GUI_EVENT_TOUCH_DOWN) ? pass++ : fail++;
    
    event_ret = xy_gui_event_push(&queue, &event);
    print_result("xy_gui_event_push", event_ret == 0);
    (event_ret == 0) ? pass++ : fail++;
    
    xy_gui_event_t popped;
    event_ret = xy_gui_event_pop(&queue, &popped);
    print_result("xy_gui_event_pop", event_ret == 0);
    (event_ret == 0) ? pass++ : fail++;
    
    /* ========== 布局系统测试 ========== */
    printf("\n=== GUI Layout System ===\n");
    
    int layout_ret = xy_gui_layout_system_init();
    print_result("xy_gui_layout_system_init", layout_ret == 0);
    (layout_ret == 0) ? pass++ : fail++;
    
    xy_gui_widget_t container;
    xy_gui_widget_init(&container, 0, 0, 0, 400, 300);
    
    /* 添加子控件用于布局测试 */
    xy_gui_widget_t child1, child2;
    xy_gui_widget_init(&child1, 0, 0, 0, 100, 40);
    xy_gui_widget_init(&child2, 0, 0, 0, 100, 40);
    xy_gui_widget_add_child(&container, &child1);
    xy_gui_widget_add_child(&container, &child2);
    
    xy_gui_layout_config_t config;
    memset(&config, 0, sizeof(config));
    config.type = XY_GUI_LAYOUT_VERTICAL;
    config.main_align = XY_GUI_LAYOUT_ALIGN_START;
    config.cross_align = XY_GUI_LAYOUT_ALIGN_CENTER;
    config.main_spacing = 10;
    
    layout_ret = xy_gui_layout_set_config(&container, &config);
    print_result("xy_gui_layout_set_config", layout_ret == 0);
    (layout_ret == 0) ? pass++ : fail++;
    
    xy_gui_layout_config_t retrieved_config;
    layout_ret = xy_gui_layout_get_config(&container, &retrieved_config);
    print_result("xy_gui_layout_get_config", layout_ret == 0 && retrieved_config.type == XY_GUI_LAYOUT_VERTICAL);
    (layout_ret == 0 && retrieved_config.type == XY_GUI_LAYOUT_VERTICAL) ? pass++ : fail++;
    
    layout_ret = xy_gui_layout_vertical(&container);
    print_result("xy_gui_layout_vertical", layout_ret == 0);
    (layout_ret == 0) ? pass++ : fail++;
    
    /* ========== 主题系统测试 ========== */
    printf("\n=== GUI Theme System ===\n");
    
    int theme_ret = xy_gui_theme_system_init();
    print_result("xy_gui_theme_system_init", theme_ret == 0);
    (theme_ret == 0) ? pass++ : fail++;
    
    xy_gui_theme_t *current = xy_gui_theme_get_current();
    print_result("xy_gui_theme_get_current", current != NULL);
    (current != NULL) ? pass++ : fail++;
    
    xy_gui_theme_t *light = xy_gui_theme_find("Light");
    print_result("xy_gui_theme_find(Light)", light != NULL);
    (light != NULL) ? pass++ : fail++;
    
    xy_gui_theme_t *dark = xy_gui_theme_find("Dark");
    print_result("xy_gui_theme_find(Dark)", dark != NULL);
    (dark != NULL) ? pass++ : fail++;
    
    theme_ret = xy_gui_theme_set_dark();
    print_result("xy_gui_theme_set_dark", theme_ret == 0);
    (theme_ret == 0) ? pass++ : fail++;
    
    current = xy_gui_theme_get_current();
    print_result("theme switched to Dark", current != NULL && strcmp(current->name, "Dark") == 0);
    (current != NULL && strcmp(current->name, "Dark") == 0) ? pass++ : fail++;
    
    theme_ret = xy_gui_theme_set_light();
    print_result("xy_gui_theme_set_light", theme_ret == 0);
    (theme_ret == 0) ? pass++ : fail++;
    
    /* 主题应用到控件 */
    xy_gui_widget_t test_widget;
    xy_gui_widget_init(&test_widget, 0, 10, 20, 100, 50);
    
    theme_ret = xy_gui_theme_apply_to_widget(&test_widget, light);
    print_result("xy_gui_theme_apply_to_widget", theme_ret == 0);
    (theme_ret == 0) ? pass++ : fail++;
    
    /* ========== Widget 基础测试 ========== */
    printf("\n=== GUI Widget Base ===\n");
    
    xy_gui_widget_t widget;
    int widget_ret = xy_gui_widget_init(&widget, 0, 10, 20, 100, 50);
    print_result("xy_gui_widget_init", widget_ret == 0);
    (widget_ret == 0) ? pass++ : fail++;
    
    widget_ret = xy_gui_widget_set_pos(&widget, 50, 60);
    print_result("xy_gui_widget_set_pos", widget_ret == 0);
    (widget_ret == 0) ? pass++ : fail++;
    
    widget_ret = xy_gui_widget_set_size(&widget, 150, 75);
    print_result("xy_gui_widget_set_size", widget_ret == 0);
    (widget_ret == 0) ? pass++ : fail++;
    
    widget_ret = xy_gui_widget_set_visible(&widget, true);
    print_result("xy_gui_widget_set_visible", widget_ret == 0);
    (widget_ret == 0) ? pass++ : fail++;
    
    widget_ret = xy_gui_widget_set_enabled(&widget, true);
    print_result("xy_gui_widget_set_enabled", widget_ret == 0);
    (widget_ret == 0) ? pass++ : fail++;
    
    /* Summary */
    printf("\n==========================================\n");
    printf("Results: %d passed, %d failed\n", pass, fail);
    if (fail == 0) {
        printf("✅ GUI STAGE 2 COMPLETE - ALL TESTS PASSED!\n");
    } else {
        printf("❌ %d TEST(S) FAILED!\n", fail);
    }
    printf("==========================================\n");
    
    return fail;
}
