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

static xy_led_driver_t *led_gui_driver_at(size_t index)
{
    if (index >= XY_LED_GUI_REGISTRY_SIZE) {
        return NULL;
    }

    return g_led_gui_registry[index].driver;
}

static void led_gui_set_pixel_for(size_t index, int16_t x, int16_t y, uint32_t color)
{
    xy_led_driver_t *driver = led_gui_driver_at(index);
    if (!driver || !driver->set_pixel || x < 0 || y < 0) {
        return;
    }

    driver->set_pixel((uint16_t)x, (uint16_t)y, color);
}

static uint32_t led_gui_get_pixel_for(size_t index, int16_t x, int16_t y)
{
    xy_led_driver_t *driver = led_gui_driver_at(index);
    if (!driver || !driver->get_pixel || x < 0 || y < 0) {
        return 0U;
    }

    return driver->get_pixel((uint16_t)x, (uint16_t)y);
}

static void led_gui_fill_rect_for(size_t index, int16_t x, int16_t y, int16_t w, int16_t h,
                                  uint32_t color)
{
    xy_led_driver_t *driver = led_gui_driver_at(index);
    if (!driver || !driver->set_pixel || w <= 0 || h <= 0) {
        return;
    }

    for (int16_t row = 0; row < h; ++row) {
        for (int16_t col = 0; col < w; ++col) {
            led_gui_set_pixel_for(index, (int16_t)(x + col), (int16_t)(y + row), color);
        }
    }
}

static void led_gui_flush_for(size_t index)
{
    xy_led_driver_t *driver = led_gui_driver_at(index);
    if (driver && driver->show) {
        driver->show();
    }
}

#define DEFINE_LED_GUI_SLOT(index)                                                                 \
    static void led_gui_set_pixel_##index(int16_t x, int16_t y, uint32_t color)                    \
    {                                                                                              \
        led_gui_set_pixel_for((index), x, y, color);                                               \
    }                                                                                              \
    static uint32_t led_gui_get_pixel_##index(int16_t x, int16_t y)                                \
    {                                                                                              \
        return led_gui_get_pixel_for((index), x, y);                                               \
    }                                                                                              \
    static void led_gui_fill_rect_##index(int16_t x, int16_t y, int16_t w, int16_t h,              \
                                          uint32_t color)                                          \
    {                                                                                              \
        led_gui_fill_rect_for((index), x, y, w, h, color);                                        \
    }                                                                                              \
    static void led_gui_flush_##index(void)                                                        \
    {                                                                                              \
        led_gui_flush_for((index));                                                                \
    }

DEFINE_LED_GUI_SLOT(0)
DEFINE_LED_GUI_SLOT(1)
DEFINE_LED_GUI_SLOT(2)
DEFINE_LED_GUI_SLOT(3)
DEFINE_LED_GUI_SLOT(4)
DEFINE_LED_GUI_SLOT(5)
DEFINE_LED_GUI_SLOT(6)
DEFINE_LED_GUI_SLOT(7)

static void (*const g_led_gui_set_pixel_slots[])(int16_t, int16_t, uint32_t) = {
    led_gui_set_pixel_0, led_gui_set_pixel_1, led_gui_set_pixel_2, led_gui_set_pixel_3,
    led_gui_set_pixel_4, led_gui_set_pixel_5, led_gui_set_pixel_6, led_gui_set_pixel_7,
};

static uint32_t (*const g_led_gui_get_pixel_slots[])(int16_t, int16_t) = {
    led_gui_get_pixel_0, led_gui_get_pixel_1, led_gui_get_pixel_2, led_gui_get_pixel_3,
    led_gui_get_pixel_4, led_gui_get_pixel_5, led_gui_get_pixel_6, led_gui_get_pixel_7,
};

static void (*const g_led_gui_fill_rect_slots[])(int16_t, int16_t, int16_t, int16_t, uint32_t) = {
    led_gui_fill_rect_0, led_gui_fill_rect_1, led_gui_fill_rect_2, led_gui_fill_rect_3,
    led_gui_fill_rect_4, led_gui_fill_rect_5, led_gui_fill_rect_6, led_gui_fill_rect_7,
};

static void (*const g_led_gui_flush_slots[])(void) = {
    led_gui_flush_0, led_gui_flush_1, led_gui_flush_2, led_gui_flush_3,
    led_gui_flush_4, led_gui_flush_5, led_gui_flush_6, led_gui_flush_7,
};

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
            g_led_gui_registry[index].display.set_pixel = g_led_gui_set_pixel_slots[index];
            g_led_gui_registry[index].display.get_pixel = g_led_gui_get_pixel_slots[index];
            g_led_gui_registry[index].display.fill_rect = g_led_gui_fill_rect_slots[index];
            g_led_gui_registry[index].display.flush = g_led_gui_flush_slots[index];
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

    size_t index = (size_t)(entry - g_led_gui_registry);
    entry->display.width = drv->width;
    entry->display.height = drv->height;
    entry->display.format = drv->bpp <= 1U ? XY_GUI_COLOR_MONO : XY_GUI_COLOR_RGB888;
    entry->display.set_pixel = g_led_gui_set_pixel_slots[index];
    entry->display.get_pixel = drv->get_pixel ? g_led_gui_get_pixel_slots[index] : NULL;
    entry->display.fill_rect = g_led_gui_fill_rect_slots[index];
    entry->display.flush = g_led_gui_flush_slots[index];
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
