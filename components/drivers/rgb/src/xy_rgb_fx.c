/**
 * @file xy_rgb_fx.c
 * @brief RGB LED Effects Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_rgb_fx.h"
#include "xy_rgb_color.h"
#include "xy_log.h"
#include <stdlib.h>
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 效果信息表 */
static const xy_rgb_fx_info_t g_fx_info[FX_COUNT] = {
    /* 静态效果 */
    {FX_STATIC, "Static", xy_rgb_fx_static, 0, 255},
    {FX_RAINBOW, "Rainbow", xy_rgb_fx_rainbow, 1, 255},
    {FX_GRADIENT, "Gradient", xy_rgb_fx_gradient, 1, 255},
    
    /* 动态效果 */
    {FX_SCAN, "Scan", xy_rgb_fx_scan, 1, 255},
    {FX_CHASE, "Chase", xy_rgb_fx_chase, 1, 255},
    {FX_FADE, "Fade", xy_rgb_fx_fade, 1, 255},
    {FX_BLINK, "Blink", xy_rgb_fx_blink, 1, 255},
    {FX_TWINKLE, "Twinkle", xy_rgb_fx_twinkle, 1, 255},
    {FX_COMET, "Comet", xy_rgb_fx_comet, 1, 255},
    {FX_FIRE, "Fire", xy_rgb_fx_fire, 1, 255},
    {FX_WATER, "Water", xy_rgb_fx_water, 1, 255},
    {FX_BREATH, "Breath", xy_rgb_fx_breath, 1, 255},
    {FX流星，"Meteor", xy_rgb_fx_meteor, 1, 255},
    
    /* 音乐效果 */
    {FX_SPECTRUM, "Spectrum", NULL, 1, 255},
    {FX_VU_METER, "VU Meter", NULL, 1, 255},
};

/* 全局帧计数器 */
static uint32_t g_frame_count = 0;

/**
 * @brief 初始化效果引擎
 */
void xy_rgb_fx_init(void)
{
    g_frame_count = 0;
}

/**
 * @brief 获取效果信息
 */
const xy_rgb_fx_info_t* xy_rgb_fx_get_info(xy_rgb_effect_t effect)
{
    if (effect >= FX_COUNT) {
        return NULL;
    }
    return &g_fx_info[effect];
}

/**
 * @brief 获取效果数量
 */
uint8_t xy_rgb_fx_get_count(void)
{
    return FX_COUNT;
}

/**
 * @brief 获取效果名称
 */
const char* xy_rgb_fx_get_name(xy_rgb_effect_t effect)
{
    if (effect >= FX_COUNT) {
        return "Unknown";
    }
    return g_fx_info[effect].name;
}

/**
 * @brief 效果服务
 */
void xy_rgb_fx_service(void)
{
    g_frame_count++;
}

/* ==================== 静态效果 ==================== */

/**
 * @brief 静态颜色效果
 */
void xy_rgb_fx_static(xy_rgb_segment_t *seg)
{
    for (uint16_t i = seg->start; i < seg->stop; i++) {
        xy_rgb_set_pixel(i, seg->color1);
    }
}

/**
 * @brief 彩虹效果
 */
void xy_rgb_fx_rainbow(xy_rgb_segment_t *seg)
{
    uint8_t hue = (g_frame_count * seg->speed / 256) % 256;
    
    for (uint16_t i = seg->start; i < seg->stop; i++) {
        uint8_t h = (hue + (i - seg->start) * 256 / (seg->stop - seg->start)) % 256;
        xy_rgb_set_pixel(i, xy_rainbow_color(h));
    }
}

/**
 * @brief 渐变效果
 */
void xy_rgb_fx_gradient(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t pos = i * 255 / len;
        rgb_color_t color = xy_gradient_color(seg->color1, seg->color2, pos);
        xy_rgb_set_pixel(seg->start + i, color);
    }
}

/* ==================== 动态效果 ==================== */

/**
 * @brief 扫描灯效果
 */
void xy_rgb_fx_scan(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint16_t pos = (g_frame_count * seg->speed / 256) % (len * 2 - 2);
    
    xy_rgb_clear();
    
    if (pos < len) {
        xy_rgb_set_pixel(seg->start + pos, seg->color1);
    } else {
        xy_rgb_set_pixel(seg->start + (len * 2 - 2 - pos), seg->color1);
    }
}

/**
 * @brief 追逐灯效果
 */
void xy_rgb_fx_chase(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint16_t offset = (g_frame_count * seg->speed / 256) % len;
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t idx = (i + offset) % len;
        rgb_color_t color = (i < 3) ? seg->color1 : seg->color2;
        xy_rgb_set_pixel(seg->start + idx, color);
    }
}

/**
 * @brief 淡入淡出效果
 */
void xy_rgb_fx_fade(xy_rgb_segment_t *seg)
{
    uint8_t brightness = (uint8_t)(128 + 127 * sinf(g_frame_count * seg->speed / 4096.0f));
    
    for (uint16_t i = seg->start; i < seg->stop; i++) {
        rgb_color_t color = xy_color_darken(seg->color1, 255 - brightness);
        xy_rgb_set_pixel(i, color);
    }
}

/**
 * @brief 闪烁效果
 */
void xy_rgb_fx_blink(xy_rgb_segment_t *seg)
{
    uint16_t period = 256 * 256 / seg->speed;
    bool on = (g_frame_count % period) < (period / 2);
    
    for (uint16_t i = seg->start; i < seg->stop; i++) {
        xy_rgb_set_pixel(i, on ? seg->color1 : seg->color2);
    }
}

/**
 * @brief 闪烁星光效果
 */
void xy_rgb_fx_twinkle(xy_rgb_segment_t *seg)
{
    for (uint16_t i = seg->start; i < seg->stop; i++) {
        if (rand() % 256 < seg->intensity) {
            xy_rgb_set_pixel(i, seg->color1);
        } else {
            xy_rgb_set_pixel(i, seg->color2);
        }
    }
}

/**
 * @brief 彗星效果
 */
void xy_rgb_fx_comet(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint16_t pos = (g_frame_count * seg->speed / 256) % len;
    
    xy_rgb_clear();
    
    /* 头部 */
    xy_rgb_set_pixel(seg->start + pos, seg->color1);
    
    /* 尾部 */
    for (int i = 1; i <= 3; i++) {
        if (pos >= i) {
            rgb_color_t color = xy_color_darken(seg->color1, i * 64);
            xy_rgb_set_pixel(seg->start + pos - i, color);
        }
    }
}

/**
 * @brief 火焰效果
 */
void xy_rgb_fx_fire(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    
    for (uint16_t i = 0; i < len; i++) {
        /* 从底部到顶部衰减 */
        uint8_t intensity = (uint8_t)(255 - i * 255 / len);
        
        /* 随机波动 */
        if (rand() % 256 < seg->intensity) {
            intensity = intensity * (150 + rand() % 106) / 255;
        }
        
        /* 火焰颜色：红 -> 橙 -> 黄 */
        rgb_color_t color;
        if (intensity > 128) {
            color = xy_color_blend((rgb_color_t){255, 0, 0}, (rgb_color_t){255, 255, 0}, intensity - 128);
        } else {
            color = xy_color_darken((rgb_color_t){255, 0, 0}, 128 - intensity);
        }
        
        xy_rgb_set_pixel(seg->start + i, color);
    }
}

/**
 * @brief 水流效果
 */
void xy_rgb_fx_water(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint16_t offset = (g_frame_count * seg->speed / 256) % len;
    
    for (uint16_t i = 0; i < len; i++) {
        uint16_t idx = (i + offset) % len;
        uint8_t brightness = (uint8_t)(128 + 127 * sinf(i * 3.14159f / len));
        rgb_color_t color = xy_color_darken(seg->color1, 255 - brightness);
        xy_rgb_set_pixel(seg->start + idx, color);
    }
}

/**
 * @brief 呼吸效果
 */
void xy_rgb_fx_breath(xy_rgb_segment_t *seg)
{
    uint8_t brightness = (uint8_t)(128 + 127 * sinf(g_frame_count * seg->speed / 4096.0f));
    
    for (uint16_t i = seg->start; i < seg->stop; i++) {
        rgb_color_t color = seg->color1;
        color.r = color.r * brightness / 255;
        color.g = color.g * brightness / 255;
        color.b = color.b * brightness / 255;
        xy_rgb_set_pixel(i, color);
    }
}

/**
 * @brief 流星效果
 */
void xy_rgb_fx_meteor(xy_rgb_segment_t *seg)
{
    uint16_t len = seg->stop - seg->start;
    uint16_t pos = (g_frame_count * seg->speed / 256) % (len + 5);
    
    xy_rgb_clear();
    
    if (pos < len + 5) {
        /* 流星头部 */
        if (pos < len) {
            xy_rgb_set_pixel(seg->start + pos, seg->color1);
        }
        
        /* 流星尾部 */
        for (int i = 1; i <= 5; i++) {
            if (pos >= i && pos - i < len) {
                rgb_color_t color = xy_color_darken(seg->color1, i * 50);
                xy_rgb_set_pixel(seg->start + pos - i, color);
            }
        }
    }
}

/* ==================== 音乐效果 ==================== */

/**
 * @brief 频谱效果
 */
void xy_rgb_fx_spectrum(xy_rgb_segment_t *seg, uint8_t *spectrum, uint8_t bands)
{
    if (!spectrum || bands == 0) {
        return;
    }
    
    uint16_t len = seg->stop - seg->start;
    uint16_t leds_per_band = len / bands;
    
    for (uint8_t b = 0; b < bands; b++) {
        uint8_t height = spectrum[b] * len / 256;
        
        for (uint16_t i = 0; i < leds_per_band && i < height; i++) {
            uint16_t idx = seg->start + b * leds_per_band + i;
            rgb_color_t color = xy_rainbow_color(b * 256 / bands);
            xy_rgb_set_pixel(idx, color);
        }
    }
}

/**
 * @brief 音量表效果
 */
void xy_rgb_fx_vu_meter(xy_rgb_segment_t *seg, uint8_t level)
{
    uint16_t len = seg->stop - seg->start;
    uint8_t height = level * len / 256;
    
    for (uint16_t i = 0; i < len; i++) {
        rgb_color_t color;
        
        if (i < height) {
            /* 根据高度选择颜色 */
            if (i < len * 2 / 3) {
                color = seg->color1;  /* 绿色 */
            } else if (i < len * 5 / 6) {
                color = seg->color2;  /* 黄色 */
            } else {
                color = seg->color3;  /* 红色 */
            }
            xy_rgb_set_pixel(seg->start + i, color);
        } else {
            xy_rgb_set_pixel(seg->start + i, seg->color2);
        }
    }
}
