#pragma once

#include "../inttypes.hpp"
struct STB_Rect
{
    i32 x;
    i32 y;
    i32 w;
    i32 h;
};

struct STB_Surface
{
    i32 w;
    i32 h;
    i32 channels;
    i32 pitch;
    bool owns_pixels;
    u8 *pixels;
};

struct STB_Color
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

static const u8 g_PixelChannels[6] = {0, 4, 4, 3, 3, 4};
