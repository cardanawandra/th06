#pragma once
#include <SDL.h>
#include <math.h>
#include <algorithm>
#define SDL_CONTROLLER_BUTTON_MAX 32

typedef struct {
    int bpp;
    Uint32 rmask, gmask, bmask, amask;
} PixelFormatSDL1;

const PixelFormatSDL1 SDL1_PIXELFORMAT_UNKNOWN = {0, 0, 0, 0, 0};
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    const PixelFormatSDL1 SDL1_PIXELFORMAT_RGBA32 = {
        32,
        0xFF000000, // R
        0x00FF0000, // G
        0x0000FF00, // B
        0x000000FF  // A
    };
    const PixelFormatSDL1 SDL1_PIXELFORMAT_RGBA5551 = {
        16,
        0xF800, // R (bits 15-11)
        0x07C0, // G (10-6)
        0x003E, // B (5-1)
        0x0001  // A (0)
    };
    const PixelFormatSDL1 SDL1_PIXELFORMAT_RGB24 = {
        24,
        0xFF0000,
        0x00FF00,
        0x0000FF,
        0
    };
#else
    const PixelFormatSDL1 SDL1_PIXELFORMAT_RGBA32 = {
        32,
        0x000000FF, // R
        0x0000FF00, // G
        0x00FF0000, // B
        0xFF000000  // A
    };
    const PixelFormatSDL1 SDL1_PIXELFORMAT_RGBA5551 = {
        16,
        0x001F, // R
        0x03E0, // G
        0x7C00, // B
        0x8000  // A
    };
    const PixelFormatSDL1 SDL1_PIXELFORMAT_RGB24 = {
        24,
        0x0000FF,
        0x00FF00,
        0xFF0000,
        0
    };
#endif

//this thing doesn't care about endian at all
const PixelFormatSDL1 SDL1_PIXELFORMAT_RGBA4444 = {
    16,
    0xF000, // R (15-12)
    0x0F00, // G (11-8)
    0x00F0, // B (7-4)
    0x000F  // A (3-0)
};
const PixelFormatSDL1 SDL1_PIXELFORMAT_RGB565 = {
    16,
    0xF800, // R (15-11)
    0x07E0, // G (10-5)
    0x001F, // B (4-0)
    0
};


static inline Sint64 GetRWSize(SDL_RWops* ctx)
{
    Sint64 pos = SDL_RWtell(ctx);
    Sint64 size = SDL_RWseek(ctx, 0, RW_SEEK_END);
    SDL_RWseek(ctx, pos, RW_SEEK_SET);
    return size;
}

static inline SDL_Surface* SDL_ConvertSurfaceFormat(SDL_Surface* src, const PixelFormatSDL1& fmt, int idk)
{
    if (!src) return NULL;

    SDL_Surface* temp = SDL_CreateRGBSurface(
        SDL_SWSURFACE,
        0,0,
        // src->w,
        // src->h,
        fmt.bpp,
        fmt.rmask,
        fmt.gmask,
        fmt.bmask,
        fmt.amask
    );

    if (!temp) return NULL;

    SDL_Surface* converted = SDL_ConvertSurface(src, temp->format, SDL_SWSURFACE);

    SDL_FreeSurface(temp);

    return converted;
}

// Bilinear stretch replacement for SDL_SoftStretchLinear
static inline int SDL_SoftStretchLinear(
    SDL_Surface* src,
    SDL_Rect* srcrect,
    SDL_Surface* dst,
    SDL_Rect* dstrect)
{
    if (!src || !dst) return -1;
    return SDL_SoftStretch(src,srcrect,dst,dstrect);
}
static inline int SDL_BlitScaled(
    SDL_Surface* src,
    SDL_Rect* srcrect,
    SDL_Surface* dst,
    SDL_Rect* dstrect)
{
    if (!src || !dst) return -1;

    SDL_Rect fullSrc = {0, 0, (Uint16)src->w, (Uint16)src->h};
    SDL_Rect fullDst = {0, 0, (Uint16)dst->w, (Uint16)dst->h};

    SDL_Rect* s = srcrect ? srcrect : &fullSrc;
    SDL_Rect* d = dstrect ? dstrect : &fullDst;

    // If sizes match normal blit (fast path)
    if (s->w == d->w && s->h == d->h)
    {
        return SDL_BlitSurface(src, s, dst, d);
    }

    return SDL_SoftStretchLinear(src, s, dst, d);
}