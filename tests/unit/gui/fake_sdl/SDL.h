#ifndef TEST_FAKE_SDL_H
#define TEST_FAKE_SDL_H

#include <stdint.h>

#define SDL_INIT_VIDEO 0x20U
#define SDL_WINDOWPOS_CENTERED 0
#define SDL_WINDOW_SHOWN 0x04U
#define SDL_RENDERER_SOFTWARE 0x01U
#define SDL_RENDERER_ACCELERATED 0x02U
#define SDL_PIXELFORMAT_ARGB8888 0x16362004U
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_QUIT 0x100U

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct {
    uint32_t type;
} SDL_Event;

int SDL_Init(uint32_t flags);
void SDL_Quit(void);
SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int width, int height, uint32_t flags);
SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, int index, uint32_t flags);
SDL_Texture *SDL_CreateTexture(SDL_Renderer *renderer, uint32_t format, int access, int width,
                               int height);
void SDL_DestroyTexture(SDL_Texture *texture);
void SDL_DestroyRenderer(SDL_Renderer *renderer);
void SDL_DestroyWindow(SDL_Window *window);
int SDL_UpdateTexture(SDL_Texture *texture, const void *rect, const void *pixels, int pitch);
int SDL_RenderClear(SDL_Renderer *renderer);
int SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture, const void *source,
                   const void *destination);
void SDL_RenderPresent(SDL_Renderer *renderer);
int SDL_PollEvent(SDL_Event *event);

#endif
