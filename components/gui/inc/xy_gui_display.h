/**
 * @file xy_gui_display.h
 * @brief GUI Display Driver Interface - Abstract Display Device
 * @version 1.0.0
 * @date 2026-03-02
 * 
 * GUI 显示接口 - LED 驱动通过实现此接口为 GUI 服务
 */

#ifndef XY_GUI_DISPLAY_H
#define XY_GUI_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 色彩格式 ==================== */

typedef enum {
    XY_GUI_COLOR_MONO = 0,    // 1bit 单色
    XY_GUI_COLOR_GRAY4,       // 4 级灰度
    XY_GUI_COLOR_GRAY256,     // 256 级灰度
    XY_GUI_COLOR_RGB565,      // 16 位 RGB
    XY_GUI_COLOR_RGB888,      // 24 位 RGB
    XY_GUI_COLOR_ARGB8888,    // 32 位 ARGB
} xy_gui_color_format_t;

/**
 * @brief GUI 颜色结构
 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} xy_gui_color_t;

/* 预定义颜色 */
#define GUI_COLOR_BLACK     {0, 0, 0, 255}
#define GUI_COLOR_WHITE     {255, 255, 255, 255}
#define GUI_COLOR_RED       {255, 0, 0, 255}
#define GUI_COLOR_GREEN     {0, 255, 0, 255}
#define GUI_COLOR_BLUE      {0, 0, 255, 255}
#define GUI_COLOR_YELLOW    {255, 255, 0, 255}
#define GUI_COLOR_CYAN      {0, 255, 255, 255}
#define GUI_COLOR_MAGENTA   {255, 0, 255, 255}

/**
 * @brief GUI 显示设备接口
 */
typedef struct {
    /* 显示参数 */
    uint16_t width;
    uint16_t height;
    xy_gui_color_format_t format;
    
    /* 驱动接口 (由 LED 驱动实现) */
    void (*set_pixel)(int16_t x, int16_t y, uint32_t color);
    uint32_t (*get_pixel)(int16_t x, int16_t y);
    void (*fill_rect)(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color);
    void (*flush)(void);
    void (*power_off)(void);
    
    /* 用户数据 */
    void *user_data;
} xy_gui_display_t;

/**
 * @brief 初始化 GUI 显示
 */
int xy_gui_display_init(xy_gui_display_t *display);

/**
 * @brief 设置像素 (内部使用)
 */
static inline void xy_gui_display_set_pixel(xy_gui_display_t *display,
                                            int16_t x, int16_t y,
                                            uint32_t color)
{
    if (display && display->set_pixel) {
        display->set_pixel(x, y, color);
    }
}

/**
 * @brief 获取像素 (内部使用)
 */
static inline uint32_t xy_gui_display_get_pixel(xy_gui_display_t *display,
                                                int16_t x, int16_t y)
{
    if (display && display->get_pixel) {
        return display->get_pixel(x, y);
    }
    return 0;
}

/**
 * @brief 刷新显示 (内部使用)
 */
static inline void xy_gui_display_flush(xy_gui_display_t *display)
{
    if (display && display->flush) {
        display->flush();
    }
}

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_DISPLAY_H */
