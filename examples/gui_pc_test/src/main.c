/**
 * @file main.c
 * @brief XinYi GUI PC Test - New Widget API
 * @version 1.0.0
 * @date 2026-03-14
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* New GUI Widget Headers */
#include "xy_gui_widget.h"
#include "xy_gui_button.h"
#include "xy_gui_slider.h"
#include "xy_gui_checkbox.h"
#include "xy_gui_progress.h"
#include "xy_gui_label.h"
#include "xy_gui_container.h"
#include "xy_font.h"

/* Display Configuration */
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240

/* Global Variables */
static xy_gui_container_t g_container;
static xy_gui_button_t g_button;
static xy_gui_slider_t g_slider;
static xy_gui_checkbox_t g_checkbox;
static xy_gui_progress_t g_progress;
static xy_gui_label_t g_label;

/* ==================== Display Driver ==================== */

static int display_init(void)
{
    printf("[DISPLAY] Initialized: %dx%d\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    return 0;
}

static void display_draw_pixel(int16_t x, int16_t y, xy_gui_color_t *color)
{
    (void)x;
    (void)y;
    (void)color;
}

static void display_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, xy_gui_color_t *color)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)color;
}

static void display_draw_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t width, xy_gui_color_t *color)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)width;
    (void)color;
}

static void display_flush(void)
{
}

static xy_gui_display_t g_display_drv = {
    .width = DISPLAY_WIDTH,
    .height = DISPLAY_HEIGHT,
    .init = display_init,
    .draw_pixel = display_draw_pixel,
    .fill_rect = display_fill_rect,
    .draw_rect = display_draw_rect,
    .flush = display_flush,
};

/* ==================== Callbacks ==================== */

static void on_button_click(xy_gui_button_t *button)
{
    (void)button;
    printf("[GUI] Button clicked!\n");
    
    static uint16_t progress = 0;
    progress += 10;
    if (progress > 100) progress = 0;
    
    xy_gui_progress_set_value(&g_progress, progress);
    
    char text[32];
    snprintf(text, sizeof(text), "Progress: %d%%", progress);
    xy_gui_label_set_text(&g_label, text);
}

static void on_slider_changed(xy_gui_slider_t *slider, int32_t value)
{
    printf("[GUI] Slider value: %d\n", (int)value);
    xy_gui_progress_set_value(&g_progress, (uint16_t)value);
    
    char text[32];
    snprintf(text, sizeof(text), "Slider: %d%%", (int)value);
    xy_gui_label_set_text(&g_label, text);
    (void)slider;
}

static void on_checkbox_changed(xy_gui_checkbox_t *checkbox, bool checked)
{
    printf("[GUI] Checkbox: %s\n", checked ? "checked" : "unchecked");
    xy_gui_widget_set_enabled(&g_button.widget, !checked);
    (void)checkbox;
}

/* ==================== GUI Initialization ==================== */

static int gui_demo_init(void)
{
    int ret;
    
    printf("[GUI] Initializing widgets...\n");
    
    /* Create Container */
    ret = xy_gui_container_init(&g_container, 10, 10, DISPLAY_WIDTH - 20, DISPLAY_HEIGHT - 20);
    if (ret != XY_GUI_STATUS_OK) {
        printf("[ERROR] Container init failed: %d\n", ret);
        return ret;
    }
    
    /* Create Label */
    ret = xy_gui_label_init(&g_label, "XinYi GUI Demo", 20, 20, 280, 30);
    if (ret != XY_GUI_STATUS_OK) {
        printf("[ERROR] Label init failed: %d\n", ret);
        return ret;
    }
    
    g_label.widget.style.fg_color = XY_GUI_COLOR_BLUE;
    g_label.alignment = XY_GUI_ALIGN_CENTER;
    xy_gui_container_add_child(&g_container, &g_label.widget);
    
    /* Create Button */
    ret = xy_gui_button_init(&g_button, "Click Me", 20, 70, 130, 40);
    if (ret != XY_GUI_STATUS_OK) {
        printf("[ERROR] Button init failed: %d\n", ret);
        return ret;
    }
    
    g_button.widget.style.bg_color = XY_GUI_COLOR_BLUE;
    g_button.widget.style.fg_color = XY_GUI_COLOR_WHITE;
    g_button.widget.style.radius = 5;
    g_button.on_click = on_button_click;
    xy_gui_container_add_child(&g_container, &g_button.widget);
    
    /* Create Slider */
    ret = xy_gui_slider_init(&g_slider, 170, 70, 130, 30, 0, 100, 50);
    if (ret != XY_GUI_STATUS_OK) {
        printf("[ERROR] Slider init failed: %d\n", ret);
        return ret;
    }
    
    g_slider.widget.style.bg_color = (xy_gui_color_t){200, 200, 200, 255};
    g_slider.on_value_changed = on_slider_changed;
    xy_gui_container_add_child(&g_container, &g_slider.widget);
    
    /* Create Checkbox */
    ret = xy_gui_checkbox_init(&g_checkbox, "Disable Button", 20, 130, 150, 30);
    if (ret != XY_GUI_STATUS_OK) {
        printf("[ERROR] Checkbox init failed: %d\n", ret);
        return ret;
    }
    
    g_checkbox.on_changed = on_checkbox_changed;
    xy_gui_container_add_child(&g_container, &g_checkbox.widget);
    
    /* Create Progress Bar */
    ret = xy_gui_progress_init(&g_progress, 20, 180, 280, 20);
    if (ret != XY_GUI_STATUS_OK) {
        printf("[ERROR] Progress init failed: %d\n", ret);
        return ret;
    }
    
    g_progress.widget.style.bg_color = (xy_gui_color_t){200, 200, 200, 255};
    g_progress.bar_color = XY_GUI_COLOR_GREEN;
    xy_gui_progress_set_value(&g_progress, 0);
    xy_gui_container_add_child(&g_container, &g_progress.widget);
    
    printf("[GUI] All widgets initialized successfully!\n");
    return XY_GUI_STATUS_OK;
}

/* ==================== Main ==================== */

int main(void)
{
    int ret;
    
    printf("=================================\n");
    printf("XinYi GUI PC Test\n");
    printf("Version: 1.0.0\n");
    printf("=================================\n\n");
    
    /* Initialize Display */
    display_init();
    
    /* Initialize GUI Demo */
    ret = gui_demo_init();
    if (ret != XY_GUI_STATUS_OK) {
        printf("[ERROR] GUI Demo initialization failed!\n");
        return 1;
    }
    
    printf("\n[TEST] Running GUI tests...\n\n");
    
    /* Test button click */
    printf("[TEST] Simulating button click...\n");
    on_button_click(&g_button);
    
    /* Test slider */
    printf("\n[TEST] Simulating slider change to 75...\n");
    on_slider_changed(&g_slider, 75);
    
    /* Test checkbox */
    printf("\n[TEST] Simulating checkbox toggle...\n");
    on_checkbox_changed(&g_checkbox, true);
    
    /* Test drawing */
    printf("\n[TEST] Testing widget draw...\n");
    xy_gui_widget_draw(&g_container.widget, &g_display_drv);
    display_flush();
    
    printf("\n=================================\n");
    printf("All tests passed!\n");
    printf("=================================\n");
    
    return 0;
}
