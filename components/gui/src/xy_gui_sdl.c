/**
 * @file xy_gui_sdl.c
 * @brief SDL2 display backend for PC simulation
 */

#include "xy_gui_sdl.h"

#include <SDL2/SDL.h>
#include <stdlib.h>

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t scale;
    const char *title;
    uint32_t *pixels;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    bool configured;
    bool running;
} xy_gui_sdl_context_t;

static xy_gui_sdl_context_t g_sdl;

static uint32_t rgb565_to_argb8888(uint16_t color)
{
    uint32_t red = (uint32_t)((color >> 11) & 0x1FU);
    uint32_t green = (uint32_t)((color >> 5) & 0x3FU);
    uint32_t blue = (uint32_t)(color & 0x1FU);

    red = (red << 3) | (red >> 2);
    green = (green << 2) | (green >> 4);
    blue = (blue << 3) | (blue >> 2);
    return 0xFF000000U | (red << 16) | (green << 8) | blue;
}

void xy_gui_sdl_deinit(void)
{
    bool had_resources = g_sdl.texture || g_sdl.renderer || g_sdl.window || g_sdl.pixels;

    if (g_sdl.texture) {
        SDL_DestroyTexture(g_sdl.texture);
    }
    if (g_sdl.renderer) {
        SDL_DestroyRenderer(g_sdl.renderer);
    }
    if (g_sdl.window) {
        SDL_DestroyWindow(g_sdl.window);
    }
    free(g_sdl.pixels);
    g_sdl.texture = NULL;
    g_sdl.renderer = NULL;
    g_sdl.window = NULL;
    g_sdl.pixels = NULL;
    g_sdl.running = false;
    g_sdl.configured = false;
    if (had_resources) {
        SDL_Quit();
    }
}

int xy_gui_sdl_configure(uint16_t width, uint16_t height, uint8_t scale, const char *title)
{
    if (width == 0U || height == 0U || scale == 0U || !title) {
        return XY_GUI_INVALID_PARAM;
    }
    if (g_sdl.window || g_sdl.pixels) {
        return XY_GUI_ERROR;
    }

    g_sdl.width = width;
    g_sdl.height = height;
    g_sdl.scale = scale;
    g_sdl.title = title;
    g_sdl.configured = true;
    return XY_GUI_OK;
}

static int sdl_init(void)
{
    size_t pixel_count;

    if (!g_sdl.configured) {
        return XY_GUI_INVALID_PARAM;
    }
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return XY_GUI_ERROR;
    }

    g_sdl.window = SDL_CreateWindow(g_sdl.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    (int)g_sdl.width * g_sdl.scale,
                                    (int)g_sdl.height * g_sdl.scale, SDL_WINDOW_SHOWN);
    if (!g_sdl.window) {
        xy_gui_sdl_deinit();
        return XY_GUI_ERROR;
    }
    g_sdl.renderer = SDL_CreateRenderer(g_sdl.window, -1, SDL_RENDERER_ACCELERATED);
    if (!g_sdl.renderer) {
        xy_gui_sdl_deinit();
        return XY_GUI_ERROR;
    }
    g_sdl.texture = SDL_CreateTexture(g_sdl.renderer, SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STREAMING, g_sdl.width, g_sdl.height);
    if (!g_sdl.texture) {
        xy_gui_sdl_deinit();
        return XY_GUI_ERROR;
    }

    pixel_count = (size_t)g_sdl.width * g_sdl.height;
    g_sdl.pixels = calloc(pixel_count, sizeof(*g_sdl.pixels));
    if (!g_sdl.pixels) {
        xy_gui_sdl_deinit();
        return XY_GUI_NO_MEM;
    }
    g_sdl.running = true;
    return XY_GUI_OK;
}

static int sdl_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
    if (!g_sdl.pixels || x < 0 || y < 0 || x >= g_sdl.width || y >= g_sdl.height) {
        return XY_GUI_INVALID_PARAM;
    }
    g_sdl.pixels[(size_t)y * g_sdl.width + (size_t)x] = rgb565_to_argb8888(color);
    return XY_GUI_OK;
}

static int sdl_fill_rect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
    if (!g_sdl.pixels || width < 0 || height < 0) {
        return XY_GUI_INVALID_PARAM;
    }
    for (int16_t row = 0; row < height; row++) {
        for (int16_t column = 0; column < width; column++) {
            int16_t pixel_x = (int16_t)(x + column);
            int16_t pixel_y = (int16_t)(y + row);
            if (pixel_x >= 0 && pixel_y >= 0 && pixel_x < g_sdl.width && pixel_y < g_sdl.height) {
                g_sdl.pixels[(size_t)pixel_y * g_sdl.width + (size_t)pixel_x] =
                    rgb565_to_argb8888(color);
            }
        }
    }
    return XY_GUI_OK;
}

static int sdl_flush(void)
{
    if (!g_sdl.pixels || !g_sdl.texture || !g_sdl.renderer) {
        return XY_GUI_INVALID_PARAM;
    }
    if (SDL_UpdateTexture(g_sdl.texture, NULL, g_sdl.pixels,
                          (int)(g_sdl.width * sizeof(*g_sdl.pixels))) != 0 ||
        SDL_RenderClear(g_sdl.renderer) != 0 || SDL_RenderCopy(g_sdl.renderer, g_sdl.texture, NULL, NULL) != 0) {
        return XY_GUI_ERROR;
    }
    SDL_RenderPresent(g_sdl.renderer);
    return XY_GUI_OK;
}

static xy_gui_disp_drv_t g_driver = {
    .init = sdl_init,
    .draw_pixel = sdl_draw_pixel,
    .fill_rect = sdl_fill_rect,
    .flush = sdl_flush,
};

xy_gui_disp_drv_t *xy_gui_sdl_get_driver(void)
{
    return &g_driver;
}

bool xy_gui_sdl_poll_events(void)
{
    SDL_Event event;

    if (!g_sdl.running) {
        return false;
    }
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            g_sdl.running = false;
        }
    }
    return g_sdl.running;
}