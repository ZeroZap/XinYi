#include "xy_gui_ssd1306_adapter.h"

#include <stddef.h>
#include <string.h>

#ifndef XY_GUI_SSD1306_ADAPTER_SLOTS
#define XY_GUI_SSD1306_ADAPTER_SLOTS 4U
#endif

typedef struct {
    xy_oled_ssd1306_t *oled;
} xy_gui_ssd1306_slot_t;

static xy_gui_ssd1306_slot_t g_ssd1306_slots[XY_GUI_SSD1306_ADAPTER_SLOTS];

static bool ssd1306_color_is_on(uint16_t color)
{
    return color != XY_GUI_COLOR_BLACK;
}

static xy_oled_ssd1306_t *ssd1306_oled_at(size_t index)
{
    if (index >= XY_GUI_SSD1306_ADAPTER_SLOTS) {
        return NULL;
    }

    return g_ssd1306_slots[index].oled;
}

static int ssd1306_init_for(size_t index)
{
    return ssd1306_oled_at(index) ? XY_GUI_OK : XY_GUI_INVALID_PARAM;
}

static int ssd1306_draw_pixel_for(size_t index, int16_t x, int16_t y, uint16_t color)
{
    xy_oled_ssd1306_t *oled = ssd1306_oled_at(index);
    if (!oled) {
        return XY_GUI_INVALID_PARAM;
    }

    xy_oled_ssd1306_draw_pixel(oled, x, y, ssd1306_color_is_on(color));
    return XY_GUI_OK;
}

static int ssd1306_fill_rect_for(size_t index, int16_t x, int16_t y, int16_t w, int16_t h,
                                 uint16_t color)
{
    xy_oled_ssd1306_t *oled = ssd1306_oled_at(index);
    if (!oled) {
        return XY_GUI_INVALID_PARAM;
    }

    if (w <= 0 || h <= 0) {
        return XY_GUI_OK;
    }

    for (int16_t row = 0; row < h; ++row) {
        for (int16_t col = 0; col < w; ++col) {
            xy_oled_ssd1306_draw_pixel(oled, (int16_t)(x + col), (int16_t)(y + row),
                                       ssd1306_color_is_on(color));
        }
    }

    return XY_GUI_OK;
}

static int ssd1306_flush_for(size_t index)
{
    xy_oled_ssd1306_t *oled = ssd1306_oled_at(index);
    if (!oled) {
        return XY_GUI_INVALID_PARAM;
    }

    xy_oled_ssd1306_refresh(oled);
    return XY_GUI_OK;
}

#define DEFINE_SSD1306_GUI_SLOT(index)                                                             \
    static int ssd1306_init_##index(void)                                                           \
    {                                                                                              \
        return ssd1306_init_for((index));                                                          \
    }                                                                                              \
    static int ssd1306_draw_pixel_##index(int16_t x, int16_t y, uint16_t color)                    \
    {                                                                                              \
        return ssd1306_draw_pixel_for((index), x, y, color);                                      \
    }                                                                                              \
    static int ssd1306_fill_rect_##index(int16_t x, int16_t y, int16_t w, int16_t h,               \
                                         uint16_t color)                                           \
    {                                                                                              \
        return ssd1306_fill_rect_for((index), x, y, w, h, color);                                 \
    }                                                                                              \
    static int ssd1306_flush_##index(void)                                                         \
    {                                                                                              \
        return ssd1306_flush_for((index));                                                         \
    }

DEFINE_SSD1306_GUI_SLOT(0)
DEFINE_SSD1306_GUI_SLOT(1)
DEFINE_SSD1306_GUI_SLOT(2)
DEFINE_SSD1306_GUI_SLOT(3)

static int (*const g_ssd1306_init_slots[])(void) = {
    ssd1306_init_0,
    ssd1306_init_1,
    ssd1306_init_2,
    ssd1306_init_3,
};

static int (*const g_ssd1306_draw_pixel_slots[])(int16_t, int16_t, uint16_t) = {
    ssd1306_draw_pixel_0,
    ssd1306_draw_pixel_1,
    ssd1306_draw_pixel_2,
    ssd1306_draw_pixel_3,
};

static int (*const g_ssd1306_fill_rect_slots[])(int16_t, int16_t, int16_t, int16_t, uint16_t) = {
    ssd1306_fill_rect_0,
    ssd1306_fill_rect_1,
    ssd1306_fill_rect_2,
    ssd1306_fill_rect_3,
};

static int (*const g_ssd1306_flush_slots[])(void) = {
    ssd1306_flush_0,
    ssd1306_flush_1,
    ssd1306_flush_2,
    ssd1306_flush_3,
};

static size_t ssd1306_find_or_alloc_slot(xy_oled_ssd1306_t *oled)
{
    for (size_t index = 0; index < XY_GUI_SSD1306_ADAPTER_SLOTS; ++index) {
        if (g_ssd1306_slots[index].oled == oled) {
            return index;
        }
    }

    for (size_t index = 0; index < XY_GUI_SSD1306_ADAPTER_SLOTS; ++index) {
        if (!g_ssd1306_slots[index].oled) {
            g_ssd1306_slots[index].oled = oled;
            return index;
        }
    }

    return XY_GUI_SSD1306_ADAPTER_SLOTS;
}

int xy_gui_ssd1306_bind(xy_gui_disp_drv_t *out_drv, xy_oled_ssd1306_t *oled)
{
    if (!out_drv || !oled) {
        return XY_GUI_INVALID_PARAM;
    }

    size_t index = ssd1306_find_or_alloc_slot(oled);
    if (index >= XY_GUI_SSD1306_ADAPTER_SLOTS) {
        memset(out_drv, 0, sizeof(*out_drv));
        return XY_GUI_NO_MEM;
    }

    memset(out_drv, 0, sizeof(*out_drv));
    out_drv->init = g_ssd1306_init_slots[index];
    out_drv->draw_pixel = g_ssd1306_draw_pixel_slots[index];
    out_drv->fill_rect = g_ssd1306_fill_rect_slots[index];
    out_drv->flush = g_ssd1306_flush_slots[index];

    return XY_GUI_OK;
}
