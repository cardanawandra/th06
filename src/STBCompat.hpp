#pragma once

struct STB_Rect
{
    int x;
    int y;
    int w;
    int h;
};

struct STB_Surface
{
    int w;
    int h;
    int channels;
    int pitch;
    bool owns_pixels;
    unsigned char *pixels;
};

struct STB_Color
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

static const u8 g_PixelChannels[6] = {0, 4, 4, 3, 3, 4};
