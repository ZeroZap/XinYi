/**
 * @file xy_gui_sdl.h
 * @brief SDL2 display backend for the explicit-context GUI core
 */

#ifndef XY_GUI_SDL_H
#define XY_GUI_SDL_H

#include "xy_gui.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Configure the singleton PC display before calling the driver's init callback. */
int xy_gui_sdl_configure(uint16_t width, uint16_t height, uint8_t scale, const char *title);

/** Return the display-driver callbacks consumed by xy_gui_init(). */
xy_gui_disp_drv_t *xy_gui_sdl_get_driver(void);

/** Pump pending SDL events and return false after a window-close event. */
bool xy_gui_sdl_poll_events(void);

/** Release all SDL resources. Safe to call repeatedly. */
void xy_gui_sdl_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_SDL_H */