#include "unity.h"
#include "xy_gui_sdl.h"

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

struct SDL_Window {
    int marker;
};
struct SDL_Renderer {
    int marker;
};
struct SDL_Texture {
    int marker;
};

static struct SDL_Window g_window;
static struct SDL_Renderer g_renderer;
static struct SDL_Texture g_texture;
static uint32_t g_uploaded_pixels[12];
static int g_init_result;
static int g_create_failure;
static int g_update_result;
static int g_present_calls;
static bool g_quit_event;

void setUp(void)
{
    xy_gui_sdl_deinit();
    memset(g_uploaded_pixels, 0, sizeof(g_uploaded_pixels));
    g_init_result = 0;
    g_create_failure = 0;
    g_update_result = 0;
    g_present_calls = 0;
    g_quit_event = false;
}

void tearDown(void)
{
    xy_gui_sdl_deinit();
}

int SDL_Init(uint32_t flags)
{
    TEST_ASSERT_EQUAL_HEX32(SDL_INIT_VIDEO, flags);
    return g_init_result;
}

void SDL_Quit(void)
{
}

SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int width, int height, uint32_t flags)
{
    TEST_ASSERT_EQUAL_STRING("XinYi", title);
    TEST_ASSERT_EQUAL_INT(SDL_WINDOWPOS_CENTERED, x);
    TEST_ASSERT_EQUAL_INT(SDL_WINDOWPOS_CENTERED, y);
    TEST_ASSERT_EQUAL_INT(8, width);
    TEST_ASSERT_EQUAL_INT(6, height);
    TEST_ASSERT_EQUAL_HEX32(SDL_WINDOW_SHOWN, flags);
    return g_create_failure == 1 ? NULL : &g_window;
}

SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, int index, uint32_t flags)
{
    TEST_ASSERT_EQUAL_PTR(&g_window, window);
    TEST_ASSERT_EQUAL_INT(-1, index);
    TEST_ASSERT_EQUAL_HEX32(SDL_RENDERER_ACCELERATED, flags);
    return g_create_failure == 2 ? NULL : &g_renderer;
}

SDL_Texture *SDL_CreateTexture(SDL_Renderer *renderer, uint32_t format, int access, int width,
                               int height)
{
    TEST_ASSERT_EQUAL_PTR(&g_renderer, renderer);
    TEST_ASSERT_EQUAL_HEX32(SDL_PIXELFORMAT_ARGB8888, format);
    TEST_ASSERT_EQUAL_INT(SDL_TEXTUREACCESS_STREAMING, access);
    TEST_ASSERT_EQUAL_INT(4, width);
    TEST_ASSERT_EQUAL_INT(3, height);
    return g_create_failure == 3 ? NULL : &g_texture;
}

void SDL_DestroyTexture(SDL_Texture *texture) { TEST_ASSERT_EQUAL_PTR(&g_texture, texture); }
void SDL_DestroyRenderer(SDL_Renderer *renderer) { TEST_ASSERT_EQUAL_PTR(&g_renderer, renderer); }
void SDL_DestroyWindow(SDL_Window *window) { TEST_ASSERT_EQUAL_PTR(&g_window, window); }

int SDL_UpdateTexture(SDL_Texture *texture, const void *rect, const void *pixels, int pitch)
{
    TEST_ASSERT_EQUAL_PTR(&g_texture, texture);
    TEST_ASSERT_NULL(rect);
    TEST_ASSERT_EQUAL_INT(16, pitch);
    memcpy(g_uploaded_pixels, pixels, sizeof(g_uploaded_pixels));
    return g_update_result;
}

int SDL_RenderClear(SDL_Renderer *renderer)
{
    TEST_ASSERT_EQUAL_PTR(&g_renderer, renderer);
    return 0;
}

int SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture, const void *source,
                   const void *destination)
{
    TEST_ASSERT_EQUAL_PTR(&g_renderer, renderer);
    TEST_ASSERT_EQUAL_PTR(&g_texture, texture);
    TEST_ASSERT_NULL(source);
    TEST_ASSERT_NULL(destination);
    return 0;
}

void SDL_RenderPresent(SDL_Renderer *renderer)
{
    TEST_ASSERT_EQUAL_PTR(&g_renderer, renderer);
    g_present_calls++;
}

int SDL_PollEvent(SDL_Event *event)
{
    if (!g_quit_event) {
        return 0;
    }
    event->type = SDL_QUIT;
    g_quit_event = false;
    return 1;
}

static void test_configuration_guards_and_init_failure(void)
{
    xy_gui_disp_drv_t *driver = xy_gui_sdl_get_driver();

    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_sdl_configure(0, 3, 2, "XinYi"));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, driver->init());
    TEST_ASSERT_FALSE(xy_gui_sdl_poll_events());

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_sdl_configure(4, 3, 2, "XinYi"));
    g_init_result = -1;
    TEST_ASSERT_EQUAL_INT(XY_GUI_ERROR, driver->init());
    TEST_ASSERT_FALSE(xy_gui_sdl_poll_events());
}

static void test_driver_renders_rgb565_and_pumps_quit(void)
{
    xy_gui_disp_drv_t *driver = xy_gui_sdl_get_driver();

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_sdl_configure(4, 3, 2, "XinYi"));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, driver->init());
    TEST_ASSERT_EQUAL_INT(XY_GUI_ERROR, xy_gui_sdl_configure(4, 3, 2, "XinYi"));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, driver->draw_pixel(1, 1, XY_GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, driver->draw_pixel(4, 0, XY_GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, driver->fill_rect(-1, 0, 2, 2, XY_GUI_COLOR_GREEN));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, driver->flush());
    TEST_ASSERT_EQUAL_HEX32(0xFF00FF00U, g_uploaded_pixels[0]);
    TEST_ASSERT_EQUAL_HEX32(0xFFFF0000U, g_uploaded_pixels[5]);
    TEST_ASSERT_EQUAL_INT(1, g_present_calls);
    TEST_ASSERT_TRUE(xy_gui_sdl_poll_events());
    g_quit_event = true;
    TEST_ASSERT_FALSE(xy_gui_sdl_poll_events());
}

static void test_flush_propagates_texture_failure(void)
{
    xy_gui_disp_drv_t *driver = xy_gui_sdl_get_driver();

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_sdl_configure(4, 3, 2, "XinYi"));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, driver->init());
    g_update_result = -1;
    TEST_ASSERT_EQUAL_INT(XY_GUI_ERROR, driver->flush());
    TEST_ASSERT_EQUAL_INT(0, g_present_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_configuration_guards_and_init_failure);
    RUN_TEST(test_driver_renders_rgb565_and_pumps_quit);
    RUN_TEST(test_flush_propagates_texture_failure);
    return UNITY_END();
}