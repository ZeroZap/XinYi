/**
 * @file main.c
 * @brief STM32U5 GUI Demo - XinYi Framework
 * @version 1.0.0
 * @date 2026-03-14
 * 
 * This demo showcases the XinYi GUI framework on STM32U5 platform.
 * Features:
 * - Button with click handling
 * - Slider for value adjustment
 * - Checkbox for toggle state
 * - Progress bar for status display
 * - Label for text display
 */

#include <stdint.h>
#include <stdbool.h>
#include "xy_gui.h"
#include "xy_gui_widget.h"
#include "xy_gui_button.h"
#include "xy_gui_slider.h"
#include "xy_gui_checkbox.h"
#include "xy_gui_progress.h"
#include "xy_gui_label.h"
#include "xy_gui_container.h"
#include "xy_hal.h"

/* ==================== Hardware Configuration ==================== */

/* Display Configuration (example: 320x240 TFT) */
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240

/* ==================== Global Variables ==================== */

static xy_gui_t g_gui;
static xy_gui_widget_t g_container;
static xy_gui_button_t g_button;
static xy_gui_slider_t g_slider;
static xy_gui_checkbox_t g_checkbox;
static xy_gui_progress_t g_progress;
static xy_gui_label_t g_label;

static volatile uint32_t g_system_ticks = 0;

/* ==================== Display Driver Implementation ==================== */

/**
 * @brief Initialize display hardware
 */
static int display_init(void)
{
    /* TODO: Initialize your display hardware here */
    /* Example: LCD_Init(), SPI_Init(), etc. */
    
    XY_LOG_I("Display initialized: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    return 0;
}

/**
 * @brief Draw a pixel on display
 */
static void display_draw_pixel(int16_t x, int16_t y, xy_gui_color_t *color)
{
    /* TODO: Implement pixel drawing for your display */
    /* Example: LCD_DrawPixel(x, y, RGB565(color)); */
    (void)x;
    (void)y;
    (void)color;
}

/**
 * @brief Draw a filled rectangle
 */
static void display_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, xy_gui_color_t *color)
{
    /* TODO: Implement filled rectangle drawing */
    /* Example: LCD_FillRect(x, y, w, h, RGB565(color)); */
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)color;
}

/**
 * @brief Draw a rectangle outline
 */
static void display_draw_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t width, xy_gui_color_t *color)
{
    /* TODO: Implement rectangle outline drawing */
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)width;
    (void)color;
}

/**
 * @brief Flush display buffer (if using double buffering)
 */
static void display_flush(void)
{
    /* TODO: Implement buffer flush if using double buffering */
    /* Example: LCD_Refresh(); */
}

/* Display Driver Structure */
static xy_gui_display_t g_display_drv = {
    .init = display_init,
    .draw_pixel = display_draw_pixel,
    .fill_rect = display_fill_rect,
    .draw_rect = display_draw_rect,
    .flush = display_flush,
};

/* ==================== Touch/Input Driver ==================== */

/**
 * @brief Read touch input
 * @return true if touch detected, false otherwise
 */
static bool touch_read(xy_gui_event_t *event)
{
    /* TODO: Implement touch input reading */
    /* Example: Read from FT5206, XPT2046, etc. */
    
    static bool last_touch = false;
    bool current_touch = false; /* TODO: Read actual touch state */
    
    if (current_touch && !last_touch) {
        /* Touch pressed */
        event->type = XY_GUI_EVENT_TOUCH;
        event->x = 0; /* TODO: Read touch X */
        event->y = 0; /* TODO: Read touch Y */
        last_touch = true;
        return true;
    } else if (!current_touch && last_touch) {
        /* Touch released */
        event->type = XY_GUI_EVENT_RELEASE;
        last_touch = false;
        return true;
    }
    
    return false;
}

/* ==================== Callback Functions ==================== */

/**
 * @brief Button click callback
 */
static void on_button_click(xy_gui_button_t *button)
{
    XY_LOG_I("Button clicked!");
    
    /* Toggle progress */
    static uint16_t progress = 0;
    progress += 10;
    if (progress > 100) {
        progress = 0;
    }
    
    xy_gui_progress_set_value(&g_progress, progress);
    
    /* Update label */
    char text[32];
    snprintf(text, sizeof(text), "Progress: %d%%", progress);
    xy_gui_label_set_text(&g_label, text);
}

/**
 * @brief Slider value changed callback
 */
static void on_slider_changed(xy_gui_slider_t *slider, int32_t value)
{
    XY_LOG_D("Slider value: %d", (int)value);
    
    /* Update progress to match slider */
    xy_gui_progress_set_value(&g_progress, (uint16_t)value);
    
    /* Update label */
    char text[32];
    snprintf(text, sizeof(text), "Slider: %d%%", (int)value);
    xy_gui_label_set_text(&g_label, text);
}

/**
 * @brief Checkbox state changed callback
 */
static void on_checkbox_changed(xy_gui_checkbox_t *checkbox, bool checked)
{
    XY_LOG_I("Checkbox: %s", checked ? "checked" : "unchecked");
    
    /* Enable/disable button based on checkbox */
    xy_gui_widget_set_enabled(&g_button.widget, !checked);
}

/* ==================== GUI Initialization ==================== */

/**
 * @brief Initialize GUI and all widgets
 */
static int gui_demo_init(void)
{
    int ret;
    
    /* Initialize GUI core */
    ret = xy_gui_init(&g_gui, DISPLAY_WIDTH, DISPLAY_HEIGHT, &g_display_drv);
    if (ret != XY_GUI_OK) {
        XY_LOG_E("GUI init failed: %d", ret);
        return ret;
    }
    
    /* Clear screen */
    xy_gui_color_t bg_color = XY_GUI_COLOR_WHITE;
    xy_gui_clear(&g_gui, &bg_color);
    
    /* ==================== Create Container ==================== */
    ret = xy_gui_container_init(&g_container, 10, 10, DISPLAY_WIDTH - 20, DISPLAY_HEIGHT - 20);
    if (ret != XY_GUI_STATUS_OK) {
        XY_LOG_E("Container init failed: %d", ret);
        return ret;
    }
    
    /* Container style */
    g_container.widget.style.bg_color = (xy_gui_color_t){240, 240, 240, 255};
    g_container.widget.style.border_color = (xy_gui_color_t){100, 100, 100, 255};
    g_container.widget.style.border_width = 2;
    
    /* ==================== Create Label ==================== */
    ret = xy_gui_label_init(&g_label, "XinYi GUI Demo", 20, 20, 280, 30);
    if (ret != XY_GUI_STATUS_OK) {
        XY_LOG_E("Label init failed: %d", ret);
        return ret;
    }
    
    g_label.widget.style.fg_color = XY_GUI_COLOR_BLUE;
    g_label.alignment = XY_GUI_ALIGN_CENTER;
    
    xy_gui_container_add_child(&g_container, &g_label.widget);
    
    /* ==================== Create Button ==================== */
    ret = xy_gui_button_init(&g_button, "Click Me", 20, 70, 130, 40);
    if (ret != XY_GUI_STATUS_OK) {
        XY_LOG_E("Button init failed: %d", ret);
        return ret;
    }
    
    g_button.widget.style.bg_color = XY_GUI_COLOR_BLUE;
    g_button.widget.style.fg_color = XY_GUI_COLOR_WHITE;
    g_button.widget.style.radius = 5;
    g_button.on_click = on_button_click;
    
    xy_gui_container_add_child(&g_container, &g_button.widget);
    
    /* ==================== Create Slider ==================== */
    ret = xy_gui_slider_init(&g_slider, 170, 70, 130, 30, 0, 100, 50);
    if (ret != XY_GUI_STATUS_OK) {
        XY_LOG_E("Slider init failed: %d", ret);
        return ret;
    }
    
    g_slider.widget.style.bg_color = (xy_gui_color_t){200, 200, 200, 255};
    g_slider.on_value_changed = on_slider_changed;
    
    xy_gui_container_add_child(&g_container, &g_slider.widget);
    
    /* ==================== Create Checkbox ==================== */
    ret = xy_gui_checkbox_init(&g_checkbox, "Disable Button", 20, 130, 150, 30);
    if (ret != XY_GUI_STATUS_OK) {
        XY_LOG_E("Checkbox init failed: %d", ret);
        return ret;
    }
    
    g_checkbox.on_changed = on_checkbox_changed;
    
    xy_gui_container_add_child(&g_container, &g_checkbox.widget);
    
    /* ==================== Create Progress Bar ==================== */
    ret = xy_gui_progress_init(&g_progress, 20, 180, 280, 20);
    if (ret != XY_GUI_STATUS_OK) {
        XY_LOG_E("Progress init failed: %d", ret);
        return ret;
    }
    
    g_progress.widget.style.bg_color = (xy_gui_color_t){200, 200, 200, 255};
    g_progress.bar_color = XY_GUI_COLOR_GREEN;
    xy_gui_progress_set_value(&g_progress, 0);
    
    xy_gui_container_add_child(&g_container, &g_progress.widget);
    
    XY_LOG_I("GUI Demo initialized successfully");
    return XY_GUI_OK;
}

/* ==================== Main Loop ==================== */

/**
 * @brief Process input events
 */
static void process_events(void)
{
    xy_gui_event_t event;
    
    if (touch_read(&event)) {
        xy_gui_container_handle_event(&g_container, &event);
        xy_gui_widget_update(&g_container.widget);
    }
}

/**
 * @brief Main application loop
 */
static void main_loop(void)
{
    while (1) {
        /* Process input events */
        process_events();
        
        /* Update GUI */
        xy_gui_widget_update(&g_container.widget);
        
        /* Draw GUI */
        xy_gui_widget_draw(&g_container.widget, &g_display_drv);
        
        /* Flush display */
        xy_gui_flush(&g_gui);
        
        /* Small delay */
        xy_hal_delay_ms(16); /* ~60 FPS */
    }
}

/* ==================== System Entry Point ==================== */

/**
 * @brief System Tick Handler
 */
void SysTick_Handler(void)
{
    g_system_ticks++;
}

/**
 * @brief Main entry point
 */
int main(void)
{
    int ret;
    
    /* Initialize HAL */
    xy_hal_init();
    
    XY_LOG_I("=================================");
    XY_LOG_I("XinYi GUI Demo - STM32U5");
    XY_LOG_I("Version: 1.0.0");
    XY_LOG_I("=================================");
    
    /* Initialize GUI Demo */
    ret = gui_demo_init();
    if (ret != XY_GUI_OK) {
        XY_LOG_E("GUI Demo initialization failed!");
        while (1);
    }
    
    XY_LOG_I("Starting main loop...");
    
    /* Enter main loop */
    main_loop();
    
    return 0;
}

/* ==================== STM32U5 Exception Handlers ==================== */

void NMI_Handler(void) { while (1); }
void HardFault_Handler(void) { while (1); }
void MemManage_Handler(void) { while (1); }
void BusFault_Handler(void) { while (1); }
void UsageFault_Handler(void) { while (1); }
void SVC_Handler(void) { }
void DebugMon_Handler(void) { }
void PendSV_Handler(void) { }
