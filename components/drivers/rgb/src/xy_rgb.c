/**
 * @file xy_rgb.c
 * @brief RGB LED Effect Library - Core Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_rgb.h"
#include "xy_rgb_segment.h"
#include "xy_rgb_fx.h"
#include "xy_rgb_color.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* LED 缓冲区 */
static struct {
    rgb_color_t leds[XY_RGB_MAX_LEDS];
    uint16_t num_leds;
    uint8_t brightness;
    bool initialized;
} g_rgb = {0};

/* 效果状态 */
static struct {
    xy_rgb_effect_t current_effect;
    uint16_t speed;
    uint16_t intensity;
    uint32_t frame;
    uint32_t last_update;
} g_fx_state = {0};

/**
 * @brief 初始化 RGB 库
 */
int32_t xy_rgb_init(xy_rgb_config_t *config)
{
    if (!config || config->num_leds == 0 || config->num_leds > XY_RGB_MAX_LEDS) {
        return XY_RGB_ERROR_INVALID_PARAM;
    }
    
    memset(&g_rgb, 0, sizeof(g_rgb));
    g_rgb.num_leds = config->num_leds;
    g_rgb.brightness = config->brightness;
    g_rgb.initialized = true;
    
    /* 初始化分段管理 */
    xy_rgb_segment_init();
    
    /* 初始化效果引擎 */
    xy_rgb_fx_init();
    
    /* 初始化效果状态 */
    g_fx_state.current_effect = FX_STATIC;
    g_fx_state.speed = 128;
    g_fx_state.intensity = 128;
    g_fx_state.frame = 0;
    g_fx_state.last_update = 0;
    
    /* 清除所有 LED */
    xy_rgb_clear();
    
    xy_log_i("RGB initialized: leds=%d, brightness=%d\n", 
             config->num_leds, config->brightness);
    
    return XY_RGB_OK;
}

/**
 * @brief 反初始化
 */
int32_t xy_rgb_deinit(void)
{
    if (!g_rgb.initialized) {
        return XY_RGB_ERROR;
    }
    
    xy_rgb_clear();
    g_rgb.initialized = false;
    
    return XY_RGB_OK;
}

/**
 * @brief 设置亮度
 */
void xy_rgb_set_brightness(uint8_t brightness)
{
    g_rgb.brightness = brightness;
}

/**
 * @brief 获取亮度
 */
uint8_t xy_rgb_get_brightness(void)
{
    return g_rgb.brightness;
}

/**
 * @brief 设置所有 LED 颜色
 */
void xy_rgb_set_all(rgb_color_t color)
{
    if (!g_rgb.initialized) {
        return;
    }
    
    for (uint16_t i = 0; i < g_rgb.num_leds; i++) {
        g_rgb.leds[i] = color;
    }
}

/**
 * @brief 设置单个 LED 颜色
 */
void xy_rgb_set_pixel(uint16_t index, rgb_color_t color)
{
    if (!g_rgb.initialized || index >= g_rgb.num_leds) {
        return;
    }
    
    /* 应用亮度 */
    if (g_rgb.brightness < 255) {
        color.r = (uint16_t)color.r * g_rgb.brightness / 255;
        color.g = (uint16_t)color.g * g_rgb.brightness / 255;
        color.b = (uint16_t)color.b * g_rgb.brightness / 255;
    }
    
    g_rgb.leds[index] = color;
}

/**
 * @brief 获取 LED 颜色
 */
rgb_color_t xy_rgb_get_pixel(uint16_t index)
{
    rgb_color_t color = {0, 0, 0};
    
    if (g_rgb.initialized && index < g_rgb.num_leds) {
        color = g_rgb.leds[index];
    }
    
    return color;
}

/**
 * @brief 清除所有 LED
 */
void xy_rgb_clear(void)
{
    if (!g_rgb.initialized) {
        return;
    }
    
    memset(g_rgb.leds, 0, sizeof(g_rgb.leds));
}

/**
 * @brief 显示更新 (发送数据到 LED)
 * @note 实际实现需要调用底层驱动
 */
void xy_rgb_show(void)
{
    if (!g_rgb.initialized) {
        return;
    }
    
    /* TODO: 调用底层驱动发送数据 */
    /* xy_rgb_drv_send(g_rgb.leds, g_rgb.num_leds); */
}

/**
 * @brief 服务循环
 */
void xy_rgb_service(void)
{
    if (!g_rgb.initialized) {
        return;
    }
    
    /* 更新效果 */
    xy_rgb_fx_service();
    
    /* 更新分段 */
    xy_rgb_update_segments();
    
    g_fx_state.frame++;
}

/**
 * @brief 设置全局效果
 */
void xy_rgb_set_effect(xy_rgb_effect_t effect)
{
    if (effect >= FX_COUNT) {
        return;
    }
    
    g_fx_state.current_effect = effect;
}

/**
 * @brief 设置效果参数
 */
void xy_rgb_set_effect_params(uint16_t speed, uint16_t intensity)
{
    g_fx_state.speed = speed;
    g_fx_state.intensity = intensity;
}

/**
 * @brief 获取当前效果
 */
xy_rgb_effect_t xy_rgb_get_effect(void)
{
    return g_fx_state.current_effect;
}

/* ==================== 颜色工具实现 ==================== */

hsv_color_t xy_rgb_to_hsv(rgb_color_t rgb)
{
    return xy_rgb_to_hsv(rgb);
}

rgb_color_t xy_hsv_to_rgb(hsv_color_t hsv)
{
    return xy_hsv_to_rgb(hsv);
}

rgb_color_t xy_color_blend(rgb_color_t color1, rgb_color_t color2, uint8_t factor)
{
    return xy_color_blend(color1, color2, factor);
}

rgb_color_t xy_rainbow_color(uint8_t hue)
{
    return xy_rainbow_color(hue);
}
