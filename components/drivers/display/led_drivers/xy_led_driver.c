#include "xy_led_driver.h"

#include <stddef.h>

#ifndef XY_LED_GUI_REGISTRY_SIZE
#define XY_LED_GUI_REGISTRY_SIZE 8U
#endif

typedef struct {
    xy_led_driver_t *driver;
    xy_gui_display_t display;
    bool enabled;
} xy_led_gui_entry_t;

static xy_led_gui_entry_t g_led_gui_registry[XY_LED_GUI_REGISTRY_SIZE];
static xy_led_driver_t *g_active_gui_driver;

static void led_gui_set_pixel(int16_t x, int16_t y, uint32_t color)
{
    if (!g_active_gui_driver || !g_active_gui_driver->set_pixel || x < 0 || y < 0) {
        return;
    }

    g_active_gui_driver->set_pixel((uint16_t)x, (uint16_t)y, color);
}

static uint32_t led_gui_get_pixel(int16_t x, int16_t y)
{
    if (!g_active_gui_driver || !g_active_gui_driver->get_pixel || x < 0 || y < 0) {
        return 0U;
    }

    return g_active_gui_driver->get_pixel((uint16_t)x, (uint16_t)y);
}

static void led_gui_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color)
{
    if (!g_active_gui_driver || !g_active_gui_driver->set_pixel || w <= 0 || h <= 0) {
        return;
    }

    for (int16_t row = 0; row < h; ++row) {
        for (int16_t col = 0; col < w; ++col) {
            led_gui_set_pixel((int16_t)(x + col), (int16_t)(y + row), color);
        }
    }
}

static void led_gui_flush(void)
{
    if (g_active_gui_driver && g_active_gui_driver->show) {
        g_active_gui_driver->show();
    }
}

static xy_led_gui_entry_t *led_gui_find(xy_led_driver_t *drv)
{
    if (!drv) {
        return NULL;
    }

    for (size_t index = 0; index < XY_LED_GUI_REGISTRY_SIZE; ++index) {
        if (g_led_gui_registry[index].driver == drv) {
            return &g_led_gui_registry[index];
        }
    }

    return NULL;
}

static xy_led_gui_entry_t *led_gui_alloc(xy_led_driver_t *drv)
{
    xy_led_gui_entry_t *entry = led_gui_find(drv);
    if (entry) {
        return entry;
    }

    for (size_t index = 0; index < XY_LED_GUI_REGISTRY_SIZE; ++index) {
        if (!g_led_gui_registry[index].driver) {
            g_led_gui_registry[index].driver = drv;
            return &g_led_gui_registry[index];
        }
    }

    return NULL;
}

int xy_led_register_gui(xy_led_driver_t *drv)
{
    if (!drv || !drv->set_pixel || !drv->show || drv->width == 0U || drv->height == 0U) {
        return -1;
    }

    xy_led_gui_entry_t *entry = led_gui_alloc(drv);
    if (!entry) {
        return -1;
    }

    entry->display.width = drv->width;
    entry->display.height = drv->height;
    entry->display.format = drv->bpp <= 1U ? XY_GUI_COLOR_MONO : XY_GUI_COLOR_RGB888;
    entry->display.set_pixel = led_gui_set_pixel;
    entry->display.get_pixel = drv->get_pixel ? led_gui_get_pixel : NULL;
    entry->display.fill_rect = led_gui_fill_rect;
    entry->display.flush = led_gui_flush;
    entry->display.user_data = drv;
    entry->enabled = true;

    return 0;
}

xy_gui_display_t *xy_led_get_gui_interface(xy_led_driver_t *drv)
{
    xy_led_gui_entry_t *entry = led_gui_find(drv);
    if (!entry || !entry->enabled) {
        return NULL;
    }

    g_active_gui_driver = drv;
    return &entry->display;
}

void xy_led_enable_gui(xy_led_driver_t *drv, bool enable)
{
    xy_led_gui_entry_t *entry = led_gui_find(drv);
    if (entry) {
        entry->enabled = enable;
    }
}

bool xy_led_is_gui_enabled(xy_led_driver_t *drv)
{
    xy_led_gui_entry_t *entry = led_gui_find(drv);
    return entry ? entry->enabled : false;
}
