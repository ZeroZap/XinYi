/**
 * @file xy_font.c
 * @brief Font Rendering Engine Implementation
 * @version 1.0.0
 * @date 2026-03-01 自主学习
 */

#include "xy_font.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 5x7 字体数据 (ASCII 32-126) */
static const uint8_t g_font5x7_data[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* SPACE */
    {0x00, 0x00, 0x5F, 0x00, 0x00}, /* ! */
    {0x00, 0x07, 0x00, 0x07, 0x00}, /* " */
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, /* # */
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, /* $ */
    /* ... 省略其他字符 ... */
};

static const xy_glyph_t g_font5x7_glyphs[] = {
    {g_font5x7_data[0], 5, 7, 6, 0, 0}, /* SPACE */
    {g_font5x7_data[1], 5, 7, 6, 0, 0}, /* ! */
    /* ... */
};

const xy_font_t g_font_5x7 = {
    .name = "5x7",
    .type = XY_FONT_TYPE_BITMAP,
    .style = {7, 50, false, false},
    .glyphs = g_font5x7_glyphs,
    .glyph_count = 95,
    .first_char = 32,
    .last_char = 126,
    .line_height = 8,
    .baseline = 6,
    .initialized = true,
};

int xy_font_init(xy_font_t *font, const char *name, xy_font_type_t type)
{
    if (!font || !name) {
        return -1;
    }
    
    memset(font, 0, sizeof(*font));
    font->name = name;
    font->type = type;
    font->style.size = 12;
    font->style.weight = 50;
    
    xy_log_i("Font initialized: %s (type=%d)\n", name, type);
    return 0;
}

const xy_glyph_t* xy_font_get_glyph(const xy_font_t *font, char ch)
{
    if (!font || !font->glyphs) {
        return NULL;
    }
    
    if (ch < font->first_char || ch > font->last_char) {
        return NULL;
    }
    
    return &font->glyphs[ch - font->first_char];
}

int xy_font_measure_text(const xy_font_t *font, const char *text,
                         xy_text_metrics_t *metrics)
{
    if (!font || !text || !metrics) {
        return -1;
    }
    
    memset(metrics, 0, sizeof(*metrics));
    
    uint16_t max_width = 0;
    uint16_t current_width = 0;
    uint16_t lines = 1;
    
    while (*text) {
        if (*text == '\n') {
            if (current_width > max_width) {
                max_width = current_width;
            }
            current_width = 0;
            lines++;
            text++;
            continue;
        }
        
        const xy_glyph_t *glyph = xy_font_get_glyph(font, *text);
        if (glyph) {
            current_width += glyph->advance;
        } else {
            current_width += font->style.size / 2;
        }
        text++;
    }
    
    if (current_width > max_width) {
        max_width = current_width;
    }
    
    metrics->width = max_width;
    metrics->height = font->style.size * lines;
    metrics->lines = lines;
    
    return 0;
}

uint16_t xy_font_get_text_width(const xy_font_t *font, const char *text)
{
    xy_text_metrics_t metrics;
    if (xy_font_measure_text(font, text, &metrics) == 0) {
        return metrics.width;
    }
    return 0;
}

uint16_t xy_font_get_text_height(const xy_font_t *font)
{
    if (!font) {
        return 0;
    }
    return font->style.size;
}

uint8_t xy_font_get_line_height(const xy_font_t *font)
{
    if (!font) {
        return 0;
    }
    return font->line_height;
}

int xy_font_draw_char(const xy_font_t *font, char ch, int16_t x, int16_t y,
                      uint16_t color, void *framebuffer,
                      uint16_t fb_width, uint16_t fb_height)
{
    if (!font || !framebuffer) {
        return -1;
    }
    
    const xy_glyph_t *glyph = xy_font_get_glyph(font, ch);
    if (!glyph) {
        return -1;
    }
    
    uint16_t *fb = (uint16_t*)framebuffer;
    
    /* 绘制点阵 */
    for (uint8_t row = 0; row < glyph->height; row++) {
        for (uint8_t col = 0; col < glyph->width; col++) {
            uint8_t pixel = (glyph->data[row] >> (7 - col)) & 0x01;
            if (pixel) {
                int16_t px = x + col + glyph->offset_x;
                int16_t py = y + row + glyph->offset_y;
                
                if (px >= 0 && px < fb_width && py >= 0 && py < fb_height) {
                    fb[py * fb_width + px] = color;
                }
            }
        }
    }
    
    return 0;
}

int xy_font_draw_string(const xy_font_t *font, const char *text,
                        int16_t x, int16_t y, uint16_t color,
                        void *framebuffer, uint16_t fb_width, uint16_t fb_height)
{
    if (!font || !text || !framebuffer) {
        return -1;
    }
    
    int16_t current_x = x;
    
    while (*text) {
        if (*text == '\n') {
            x = current_x;
            y += font->line_height;
            text++;
            continue;
        }
        
        const xy_glyph_t *glyph = xy_font_get_glyph(font, *text);
        if (glyph) {
            xy_font_draw_char(font, *text, current_x, y, color,
                            framebuffer, fb_width, fb_height);
            current_x += glyph->advance;
        }
        text++;
    }
    
    return 0;
}

int xy_font_draw_text(const xy_font_t *font, const char *text,
                      int16_t x, int16_t y, int16_t max_width,
                      uint16_t color, xy_align_t align,
                      void *framebuffer, uint16_t fb_width, uint16_t fb_height)
{
    if (!font || !text || !framebuffer) {
        return -1;
    }
    
    xy_text_metrics_t metrics;
    xy_font_measure_text(font, text, &metrics);
    
    /* 计算对齐偏移 */
    int16_t offset_x = 0;
    switch (align) {
        case XY_ALIGN_LEFT:
            offset_x = 0;
            break;
        case XY_ALIGN_CENTER:
            offset_x = (max_width - metrics.width) / 2;
            break;
        case XY_ALIGN_RIGHT:
            offset_x = max_width - metrics.width;
            break;
        default:
            break;
    }
    
    return xy_font_draw_string(font, text, x + offset_x, y, color,
                              framebuffer, fb_width, fb_height);
}

int xy_font_cache_glyph(xy_font_t *font, char ch)
{
    /* 字符缓存 - 修复 TODO */
    /* 简化实现：不启用缓存，直接渲染 */
    /* 如需启用缓存，需实现缓存查找和更新逻辑 */
    (void)font;
    (void)ch;
    return 0;
}
