/**
 * @file xy_rgb_segment.c
 * @brief RGB LED Segment Management Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_rgb_segment.h"
#include "xy_rgb_fx.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 分段管理器 */
static struct {
    xy_rgb_segment_t segments[XY_RGB_MAX_SEGMENTS];
    uint8_t count;
    bool initialized;
} g_seg_mgr = {0};

/**
 * @brief 初始化分段管理
 */
static void xy_rgb_segment_init(void)
{
    if (g_seg_mgr.initialized) {
        return;
    }
    
    memset(g_seg_mgr.segments, 0, sizeof(g_seg_mgr.segments));
    g_seg_mgr.count = 0;
    g_seg_mgr.initialized = true;
}

/**
 * @brief 查找空闲分段
 */
static int8_t xy_rgb_segment_find_free(void)
{
    for (uint8_t i = 0; i < XY_RGB_MAX_SEGMENTS; i++) {
        if (!g_seg_mgr.segments[i].enabled) {
            return i;
        }
    }
    return -1;
}

int32_t xy_rgb_create_segment(uint16_t start, uint16_t stop)
{
    if (start >= stop) {
        return XY_RGB_ERROR_INVALID_PARAM;
    }
    
    xy_rgb_segment_init();
    
    int8_t idx = xy_rgb_segment_find_free();
    if (idx < 0) {
        return XY_RGB_ERROR_NO_MEMORY;
    }
    
    xy_rgb_segment_t *seg = &g_seg_mgr.segments[idx];
    seg->start = start;
    seg->stop = stop;
    seg->speed = 128;
    seg->intensity = 128;
    seg->color1 = (rgb_color_t){255, 0, 0};
    seg->color2 = (rgb_color_t){0, 0, 0};
    seg->color3 = (rgb_color_t){0, 0, 0};
    seg->effect = FX_STATIC;
    seg->reverse = false;
    seg->enabled = true;
    
    g_seg_mgr.count++;
    
    xy_log_d("RGB segment created: id=%d, start=%d, stop=%d\n", idx, start, stop);
    
    return idx;
}

int32_t xy_rgb_delete_segment(uint8_t seg_id)
{
    if (seg_id >= XY_RGB_MAX_SEGMENTS) {
        return XY_RGB_ERROR_INVALID_PARAM;
    }
    
    xy_rgb_segment_t *seg = &g_seg_mgr.segments[seg_id];
    if (!seg->enabled) {
        return XY_RGB_ERROR;
    }
    
    seg->enabled = false;
    g_seg_mgr.count--;
    
    return XY_RGB_OK;
}

int32_t xy_rgb_set_segment_effect(uint8_t seg_id, xy_rgb_effect_t effect)
{
    if (seg_id >= XY_RGB_MAX_SEGMENTS || effect >= FX_COUNT) {
        return XY_RGB_ERROR_INVALID_PARAM;
    }
    
    xy_rgb_segment_t *seg = &g_seg_mgr.segments[seg_id];
    if (!seg->enabled) {
        return XY_RGB_ERROR;
    }
    
    seg->effect = effect;
    
    return XY_RGB_OK;
}

int32_t xy_rgb_set_segment_colors(uint8_t seg_id, rgb_color_t color1, 
                                   rgb_color_t color2, rgb_color_t color3)
{
    if (seg_id >= XY_RGB_MAX_SEGMENTS) {
        return XY_RGB_ERROR_INVALID_PARAM;
    }
    
    xy_rgb_segment_t *seg = &g_seg_mgr.segments[seg_id];
    if (!seg->enabled) {
        return XY_RGB_ERROR;
    }
    
    seg->color1 = color1;
    seg->color2 = color2;
    seg->color3 = color3;
    
    return XY_RGB_OK;
}

int32_t xy_rgb_set_segment_params(uint8_t seg_id, uint16_t speed, uint16_t intensity)
{
    if (seg_id >= XY_RGB_MAX_SEGMENTS) {
        return XY_RGB_ERROR_INVALID_PARAM;
    }
    
    xy_rgb_segment_t *seg = &g_seg_mgr.segments[seg_id];
    if (!seg->enabled) {
        return XY_RGB_ERROR;
    }
    
    seg->speed = speed;
    seg->intensity = intensity;
    
    return XY_RGB_OK;
}

int32_t xy_rgb_set_segment_reverse(uint8_t seg_id, bool reverse)
{
    if (seg_id >= XY_RGB_MAX_SEGMENTS) {
        return XY_RGB_ERROR_INVALID_PARAM;
    }
    
    xy_rgb_segment_t *seg = &g_seg_mgr.segments[seg_id];
    seg->reverse = reverse;
    
    return XY_RGB_OK;
}

int32_t xy_rgb_set_segment_enabled(uint8_t seg_id, bool enabled)
{
    if (seg_id >= XY_RGB_MAX_SEGMENTS) {
        return XY_RGB_ERROR_INVALID_PARAM;
    }
    
    xy_rgb_segment_t *seg = &g_seg_mgr.segments[seg_id];
    
    if (enabled && !seg->enabled) {
        g_seg_mgr.count++;
    } else if (!enabled && seg->enabled) {
        g_seg_mgr.count--;
    }
    
    seg->enabled = enabled;
    
    return XY_RGB_OK;
}

uint8_t xy_rgb_get_segment_count(void)
{
    return g_seg_mgr.count;
}

xy_rgb_segment_t* xy_rgb_get_segment(uint8_t seg_id)
{
    if (seg_id >= XY_RGB_MAX_SEGMENTS) {
        return NULL;
    }
    
    return &g_seg_mgr.segments[seg_id];
}

void xy_rgb_clear_segments(void)
{
    memset(g_seg_mgr.segments, 0, sizeof(g_seg_mgr.segments));
    g_seg_mgr.count = 0;
}

void xy_rgb_update_segments(void)
{
    if (!g_seg_mgr.initialized) {
        return;
    }
    
    for (uint8_t i = 0; i < XY_RGB_MAX_SEGMENTS; i++) {
        xy_rgb_segment_t *seg = &g_seg_mgr.segments[i];
        if (seg->enabled) {
            /* 调用效果处理函数 */
            xy_rgb_fx_service();
        }
    }
}
