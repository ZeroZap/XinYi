/**
 * @file xy_gui.c
 * @brief Simple GUI Framework Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_gui.h"
#include <string.h>
#include <stdio.h>

/* ==================== GUI Core Implementation ==================== */

int xy_gui_init(xy_gui_t *gui, uint16_t width, uint16_t height, xy_gui_disp_drv_t *drv)
{
    int ret;

    if (!gui || !drv) {
        return XY_GUI_INVALID_PARAM;
    }
    
    memset(gui, 0, sizeof(*gui));
    gui->width = width;
    gui->height = height;
    gui->disp_drv = drv;
    gui->obj_count = 0;
    gui->bg_color = GUI_COLOR_WHITE;
    /* Initialize display driver */
    if (drv->init) {
        ret = drv->init();
        if (ret != XY_GUI_OK) {
            return ret;
        }
    }

    gui->initialized = true;
    return XY_GUI_OK;
}

int xy_gui_deinit(xy_gui_t *gui)
{
    if (!gui) {
        return XY_GUI_INVALID_PARAM;
    }
    
    gui->initialized = false;
    return XY_GUI_OK;
}

int xy_gui_clear(xy_gui_t *gui, uint16_t color)
{
    int ret;

    if (!gui || !gui->initialized) {
        return XY_GUI_INVALID_PARAM;
    }

    if (gui->disp_drv && gui->disp_drv->fill_rect) {
        ret = gui->disp_drv->fill_rect(0, 0, gui->width, gui->height, color);
        if (ret != XY_GUI_OK) {
            return ret;
        }
    }

    gui->bg_color = color;
    return XY_GUI_OK;
}

int xy_gui_flush(xy_gui_t *gui)
{
    if (!gui || !gui->initialized) {
        return XY_GUI_INVALID_PARAM;
    }

    if (gui->disp_drv && gui->disp_drv->flush) {
        return gui->disp_drv->flush();
    }

    return XY_GUI_OK;
}

int xy_gui_draw_pixel(xy_gui_t *gui, int16_t x, int16_t y, uint16_t color)
{
    if (!gui || !gui->initialized) {
        return XY_GUI_INVALID_PARAM;
    }
    
    if (x < 0 || x >= gui->width || y < 0 || y >= gui->height) {
        return XY_GUI_INVALID_PARAM;
    }

    if (gui->disp_drv && gui->disp_drv->draw_pixel) {
        return gui->disp_drv->draw_pixel(x, y, color);
    }

    return XY_GUI_OK;
}

/* Bresenham's line algorithm */
int xy_gui_draw_line(xy_gui_t *gui, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
    if (!gui || !gui->initialized) {
        return XY_GUI_INVALID_PARAM;
    }
    
    int16_t dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int16_t dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;
    int32_t timeout = ((int32_t)dx + (int32_t)dy + 1) * 2;
    
    while (timeout-- > 0) {
        xy_gui_draw_pixel(gui, x1, y1, color);
        
        if (x1 == x2 && y1 == y2) {
            break;
        }
        
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
    
    return XY_GUI_OK;
}

int xy_gui_draw_rect(xy_gui_t *gui, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (!gui || !gui->initialized) {
        return XY_GUI_INVALID_PARAM;
    }
    
    /* Draw four lines */
    xy_gui_draw_line(gui, x, y, x + w, y, color);           /* Top */
    xy_gui_draw_line(gui, x, y + h, x + w, y + h, color);   /* Bottom */
    xy_gui_draw_line(gui, x, y, x, y + h, color);           /* Left */
    xy_gui_draw_line(gui, x + w, y, x + w, y + h, color);   /* Right */
    
    return XY_GUI_OK;
}

int xy_gui_fill_rect(xy_gui_t *gui, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    int ret;

    if (!gui || !gui->initialized) {
        return XY_GUI_INVALID_PARAM;
    }

    if (gui->disp_drv && gui->disp_drv->fill_rect) {
        return gui->disp_drv->fill_rect(x, y, w, h, color);
    } else {
        /* Fallback: draw clipped pixels and stop at the first backend failure. */
        for (int16_t row = 0; row < h; row++) {
            for (int16_t col = 0; col < w; col++) {
                int16_t pixel_x = (int16_t)(x + col);
                int16_t pixel_y = (int16_t)(y + row);

                if (pixel_x < 0 || pixel_x >= gui->width || pixel_y < 0 ||
                    pixel_y >= gui->height) {
                    continue;
                }

                ret = xy_gui_draw_pixel(gui, pixel_x, pixel_y, color);
                if (ret != XY_GUI_OK) {
                    return ret;
                }
            }
        }
    }

    return XY_GUI_OK;
}

/* Simple 5x7 font */
static const uint8_t gui_font5x7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* space */
    {0x00, 0x00, 0x4F, 0x00, 0x00}, /* ! */
    {0x00, 0x07, 0x00, 0x07, 0x00}, /* " */
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, /* # */
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, /* $ */
    {0x23, 0x13, 0x08, 0x64, 0x62}, /* % */
    {0x36, 0x49, 0x55, 0x22, 0x50}, /* & */
    {0x00, 0x05, 0x03, 0x00, 0x00}, /* ' */
    {0x00, 0x1C, 0x22, 0x41, 0x00}, /* ( */
    {0x00, 0x41, 0x22, 0x1C, 0x00}, /* ) */
    {0x14, 0x08, 0x3E, 0x08, 0x14}, /* * */
    {0x08, 0x08, 0x3E, 0x08, 0x08}, /* + */
    {0x00, 0x50, 0x30, 0x00, 0x00}, /* , */
    {0x08, 0x08, 0x08, 0x08, 0x08}, /* - */
    {0x00, 0x60, 0x60, 0x00, 0x00}, /* . */
    {0x20, 0x10, 0x08, 0x04, 0x02}, /* / */
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* 0 */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* 1 */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* 2 */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* 3 */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* 4 */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* 5 */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* 6 */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* 7 */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* 8 */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* 9 */
    /* ... more characters ... */
};

int xy_gui_draw_char(xy_gui_t *gui, int16_t x, int16_t y, char c, uint16_t color)
{
    if (!gui || !gui->initialized) {
        return XY_GUI_INVALID_PARAM;
    }
    
    if (c < 32 || c > 126) {
        c = 32;
    }
    
    if (gui->disp_drv && gui->disp_drv->draw_char) {
        gui->disp_drv->draw_char(x, y, c, color);
        return XY_GUI_OK;
    }

    const uint8_t *char_data = gui_font5x7[c - 32];
    
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 7; j++) {
            if (char_data[i] & (1 << j)) {
                xy_gui_draw_pixel(gui, x + i, y + j, color);
            }
        }
    }
    
    return XY_GUI_OK;
}

int xy_gui_draw_string(xy_gui_t *gui, int16_t x, int16_t y, const char *str, uint16_t color)
{
    if (!gui || !str) {
        return XY_GUI_INVALID_PARAM;
    }
    
    while (*str) {
        xy_gui_draw_char(gui, x, y, *str, color);
        x += 6; /* 5px char + 1px space */
        str++;
    }
    
    return XY_GUI_OK;
}

/* ==================== Object Management ==================== */

xy_gui_obj_t *xy_gui_obj_create(xy_gui_t *gui, xy_gui_obj_type_t type, const xy_gui_rect_t *rect)
{
    if (!gui || !rect || gui->obj_count >= XY_GUI_MAX_OBJECTS) {
        return NULL;
    }
    
    xy_gui_obj_t *obj = &gui->objects[gui->obj_count];
    memset(obj, 0, sizeof(*obj));
    
    obj->type = type;
    obj->rect = *rect;
    obj->fg_color.color = GUI_COLOR_BLACK;
    obj->bg_color.color = GUI_COLOR_WHITE;
    obj->visible = true;
    obj->enabled = true;
    
    gui->obj_count++;
    
    return obj;
}

int xy_gui_obj_delete(xy_gui_t *gui, xy_gui_obj_t *obj)
{
    if (!gui || !obj) {
        return XY_GUI_INVALID_PARAM;
    }
    
    /* Find and remove object */
    for (uint8_t i = 0; i < gui->obj_count; i++) {
        if (&gui->objects[i] == obj) {
            /* Shift remaining objects */
            for (uint8_t j = i; j < gui->obj_count - 1; j++) {
                memcpy(&gui->objects[j], &gui->objects[j + 1], sizeof(*obj));
            }
            gui->obj_count--;
            return XY_GUI_OK;
        }
    }
    
    return XY_GUI_NOT_FOUND;
}

int xy_gui_obj_set_text(xy_gui_t *gui, xy_gui_obj_t *obj, const char *text)
{
    if (!gui || !obj) {
        return XY_GUI_INVALID_PARAM;
    }
    
    obj->text = text;
    return xy_gui_obj_redraw(gui, obj);
}

int xy_gui_obj_set_color(xy_gui_t *gui, xy_gui_obj_t *obj, uint16_t fg, uint16_t bg)
{
    if (!gui || !obj) {
        return XY_GUI_INVALID_PARAM;
    }
    
    obj->fg_color.color = fg;
    obj->bg_color.color = bg;
    return xy_gui_obj_redraw(gui, obj);
}

int xy_gui_obj_set_visible(xy_gui_t *gui, xy_gui_obj_t *obj, bool visible)
{
    if (!gui || !obj) {
        return XY_GUI_INVALID_PARAM;
    }
    
    obj->visible = visible;
    return xy_gui_obj_redraw(gui, obj);
}

int xy_gui_obj_redraw(xy_gui_t *gui, xy_gui_obj_t *obj)
{
    if (!gui || !obj || !obj->visible) {
        return XY_GUI_INVALID_PARAM;
    }
    
    switch (obj->type) {
        case XY_GUI_OBJ_LABEL:
            if (obj->text) {
                xy_gui_draw_string(gui, obj->rect.x, obj->rect.y, obj->text, obj->fg_color.color);
            }
            break;
            
        case XY_GUI_OBJ_BUTTON:
            xy_gui_fill_rect(gui, obj->rect.x, obj->rect.y, obj->rect.width, obj->rect.height, obj->bg_color.color);
            xy_gui_draw_rect(gui, obj->rect.x, obj->rect.y, obj->rect.width, obj->rect.height, obj->fg_color.color);
            if (obj->text) {
                xy_gui_draw_string(gui, obj->rect.x + 2, obj->rect.y + 2, obj->text, obj->fg_color.color);
            }
            break;
            
        case XY_GUI_OBJ_BAR:
            xy_gui_fill_rect(gui, obj->rect.x, obj->rect.y, obj->rect.width, obj->rect.height, obj->bg_color.color);
            /* Bar fill would be handled separately */
            break;
            
        default:
            break;
    }
    
    return XY_GUI_OK;
}
