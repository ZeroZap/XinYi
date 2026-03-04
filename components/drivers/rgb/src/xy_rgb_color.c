/**
 * @file xy_rgb_color.c
 * @brief RGB Color Utilities Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_rgb_color.h"
#include <math.h>
#include <stdlib.h>

/**
 * @brief RGB 转 HSV
 */
hsv_color_t xy_rgb_to_hsv(rgb_color_t rgb)
{
    hsv_color_t hsv;
    float r, g, b;
    float min, max, delta;
    
    r = rgb.r / 255.0f;
    g = rgb.g / 255.0f;
    b = rgb.b / 255.0f;
    
    min = fminf(r, fminf(g, b));
    max = fmaxf(r, fmaxf(g, b));
    delta = max - min;
    
    /* 计算 V */
    hsv.v = (uint8_t)(max * 255.0f);
    
    /* 计算 S */
    if (max > 0.0f) {
        hsv.s = (uint8_t)((delta / max) * 255.0f);
    } else {
        hsv.s = 0;
    }
    
    /* 计算 H */
    if (delta > 0.0f) {
        if (max == r) {
            hsv.h = (uint8_t)(60.0f * ((g - b) / delta) / 360.0f * 255.0f);
        } else if (max == g) {
            hsv.h = (uint8_t)(60.0f * (2.0f + (b - r) / delta) / 360.0f * 255.0f);
        } else {
            hsv.h = (uint8_t)(60.0f * (4.0f + (r - g) / delta) / 360.0f * 255.0f);
        }
    } else {
        hsv.h = 0;
    }
    
    return hsv;
}

/**
 * @brief HSV 转 RGB
 */
rgb_color_t xy_hsv_to_rgb(hsv_color_t hsv)
{
    rgb_color_t rgb;
    float h, s, v;
    float r, g, b;
    int i;
    float f, p, q, t;
    
    h = hsv.h / 255.0f * 360.0f;
    s = hsv.s / 255.0f;
    v = hsv.v / 255.0f;
    
    if (s == 0.0f) {
        /* 灰色 */
        r = g = b = v;
    } else {
        h = h / 60.0f;
        i = (int)h;
        f = h - i;
        p = v * (1.0f - s);
        q = v * (1.0f - s * f);
        t = v * (1.0f - s * (1.0f - f));
        
        switch (i % 6) {
            case 0:
                r = v; g = t; b = p;
                break;
            case 1:
                r = q; g = v; b = p;
                break;
            case 2:
                r = p; g = v; b = t;
                break;
            case 3:
                r = p; g = q; b = v;
                break;
            case 4:
                r = t; g = p; b = v;
                break;
            case 5:
                r = v; g = p; b = q;
                break;
            default:
                r = g = b = 0;
                break;
        }
    }
    
    rgb.r = (uint8_t)(r * 255.0f);
    rgb.g = (uint8_t)(g * 255.0f);
    rgb.b = (uint8_t)(b * 255.0f);
    
    return rgb;
}

/**
 * @brief 颜色混合
 */
rgb_color_t xy_color_blend(rgb_color_t color1, rgb_color_t color2, uint8_t factor)
{
    rgb_color_t result;
    
    result.r = (uint8_t)(((uint16_t)color1.r * (255 - factor) + (uint16_t)color2.r * factor) / 255);
    result.g = (uint8_t)(((uint16_t)color1.g * (255 - factor) + (uint16_t)color2.g * factor) / 255);
    result.b = (uint8_t)(((uint16_t)color1.b * (255 - factor) + (uint16_t)color2.b * factor) / 255);
    
    return result;
}

/**
 * @brief 颜色插值
 */
rgb_color_t xy_color_lerp(rgb_color_t color1, rgb_color_t color2, float t)
{
    rgb_color_t result;
    
    t = fmaxf(0.0f, fminf(1.0f, t));
    
    result.r = (uint8_t)(color1.r + (color2.r - color1.r) * t);
    result.g = (uint8_t)(color1.g + (color2.g - color1.g) * t);
    result.b = (uint8_t)(color1.b + (color2.b - color1.b) * t);
    
    return result;
}

/**
 * @brief 生成彩虹颜色
 */
rgb_color_t xy_rainbow_color(uint8_t hue)
{
    hsv_color_t hsv;
    hsv.h = hue;
    hsv.s = 255;
    hsv.v = 255;
    return xy_hsv_to_rgb(hsv);
}

/**
 * @brief 生成渐变颜色
 */
rgb_color_t xy_gradient_color(rgb_color_t start, rgb_color_t end, uint8_t pos)
{
    return xy_color_blend(start, end, pos);
}

/**
 * @brief 颜色 gamma 校正
 */
rgb_color_t xy_color_gamma(rgb_color_t color, float gamma)
{
    rgb_color_t result;
    float inv_gamma = 1.0f / gamma;
    
    result.r = (uint8_t)(powf(color.r / 255.0f, inv_gamma) * 255.0f);
    result.g = (uint8_t)(powf(color.g / 255.0f, inv_gamma) * 255.0f);
    result.b = (uint8_t)(powf(color.b / 255.0f, inv_gamma) * 255.0f);
    
    return result;
}

/**
 * @brief 限制颜色范围
 */
rgb_color_t xy_color_clamp(rgb_color_t color)
{
    /* 已经在 0-255 范围内 */
    return color;
}

/**
 * @brief 颜色取反
 */
rgb_color_t xy_color_invert(rgb_color_t color)
{
    rgb_color_t result;
    result.r = 255 - color.r;
    result.g = 255 - color.g;
    result.b = 255 - color.b;
    return result;
}

/**
 * @brief 颜色变亮
 */
rgb_color_t xy_color_brighten(rgb_color_t color, uint8_t amount)
{
    rgb_color_t result;
    result.r = (uint8_t)fminf(255, color.r + amount);
    result.g = (uint8_t)fminf(255, color.g + amount);
    result.b = (uint8_t)fminf(255, color.b + amount);
    return result;
}

/**
 * @brief 颜色变暗
 */
rgb_color_t xy_color_darken(rgb_color_t color, uint8_t amount)
{
    rgb_color_t result;
    result.r = (uint8_t)fmaxf(0, color.r - amount);
    result.g = (uint8_t)fmaxf(0, color.g - amount);
    result.b = (uint8_t)fmaxf(0, color.b - amount);
    return result;
}

/**
 * @brief 生成随机颜色
 */
rgb_color_t xy_color_random(void)
{
    rgb_color_t color;
    color.r = rand() % 256;
    color.g = rand() % 256;
    color.b = rand() % 256;
    return color;
}

/**
 * @brief 颜色相等比较
 */
bool xy_color_equal(rgb_color_t color1, rgb_color_t color2)
{
    return (color1.r == color2.r) && (color1.g == color2.g) && (color1.b == color2.b);
}

/**
 * @brief 计算颜色距离 (欧几里得距离)
 */
uint16_t xy_color_distance(rgb_color_t color1, rgb_color_t color2)
{
    int16_t dr = color1.r - color2.r;
    int16_t dg = color1.g - color2.g;
    int16_t db = color1.b - color2.b;
    
    return (uint16_t)sqrtf(dr * dr + dg * dg + db * db);
}
