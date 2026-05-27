#include "TextHelper.hpp"
#include "GameErrorContext.hpp"
#include "GameWindow.hpp"
#include "Supervisor.hpp"
#include "i18n.hpp"

#include "thirdparty/sjis_converter.h"

#include <algorithm>
#include <cstring>
#include "GamePaths.hpp"
#define STB_TRUETYPE_IMPLEMENTATION
#include "thirdparty/stb_truetype.h"

static stbtt_fontinfo g_Font;
static unsigned char *g_FontBuffer = NULL;
static float g_FontScale = 1.0f;

bool textNotExist;

TextHelper::TextHelper()
{
    //    this->format = (D3DFORMAT)-1;
    //    this->width = 0;
    //    this->height = 0;
    //    this->hdc = 0;
    //    this->gdiObj2 = 0;
    //    this->gdiObj = 0;
    //    this->buffer = NULL;
}

TextHelper::~TextHelper()
{
    if (g_FontBuffer != NULL)
    {
        free(g_FontBuffer);
        g_FontBuffer = NULL;
    }
}

#define TEXT_BUFFER_HEIGHT 64

// Extended to initialize all globals for text helper
ZunResult TextHelper::CreateTextBuffer()
{
    SDL_PIXEL_FORMAT_COMPAT_LOAD();
    // Primary font is MSゴシック, which is nonfree and has to be taken from a Windows install
    // Fallback is Noto Sans Regular (JP) which is redistributable
    #ifdef __ANDROID__
    char path[512];
    char path2[512];
    snprintf(path, sizeof(path), "%s%s",
        GamePaths::GetUserPath(), "th06.ttc");
    snprintf(path2, sizeof(path2), "%s%s",
        GamePaths::GetUserPath(), "th06.ttf");
    #else
    const char* path="th06.ttc";
    const char* path2="th06.ttf";
    #endif

    FILE* fp = fopen(path, "rb");

    if (fp == NULL)
    {
        fp = fopen(path2, "rb");
    }

    if (fp == NULL)
    {
        // LOG_COMPAT("%s\n", TTF_GetError());
        // GameErrorContext::Fatal(&g_GameErrorContext, TH_ERR_FONTS_NOT_FOUND);
        textNotExist = true;
        return ZUN_SUCCESS;
    }

    fseek(fp, 0, SEEK_END);

    size_t fileSize = ftell(fp);

    fseek(fp, 0, SEEK_SET);

    g_FontBuffer = (unsigned char*)malloc(fileSize);

    if (g_FontBuffer == NULL)
    {
        fclose(fp);

        textNotExist = true;

        return ZUN_SUCCESS;
    }

    fread(
        g_FontBuffer,
        1,
        fileSize,
        fp);

    fclose(fp);

    int fontOffset =
        stbtt_GetFontOffsetForIndex(
            g_FontBuffer,
            0);

    if (!stbtt_InitFont(
            &g_Font,
            g_FontBuffer,
            fontOffset))
    {
        free(g_FontBuffer);

        g_FontBuffer = NULL;

        textNotExist = true;

        return ZUN_SUCCESS;
    }

    g_FontScale =
        stbtt_ScaleForPixelHeight(
            &g_Font,
            30.0f);

    g_TextBufferSurface = SDL_CREATE_RGB_SURFACE_COMPAT(
        GAME_WINDOW_WIDTH, TEXT_BUFFER_HEIGHT,
        SDL_PIXELFORMAT_RGBA32
    );

    SDL_SetSurfaceBlendMode(g_TextBufferSurface, SDL_BLENDMODE_NONE);

    return ZUN_SUCCESS;
}

bool TextHelper::InvertAlpha(i32 x, i32 y, i32 spriteWidth, i32 fontHeight)
{
    if(textNotExist) return true;
    u8 *bufferCursor;
    i32 gradientArea;
    i32 i = 0;

    gradientArea = spriteWidth * fontHeight;

    SDL_LockSurface(g_TextBufferSurface);

    // In D3D EoSD this function mostly inverts the alpha, but on A1R5G5B5 surfaces specifically it also
    //   creates a gradient. D3D EoSD will always attempt to create an A1R5G5B5 surface for the text buffer,
    //   will only attempt use other formats as a fallback, and in those cases the text will be bugged anyway.
    //   As part of the port from GDI to SDL_ttf, we've converted the text buffer surface to always be RGBA32
    //   and no longer need the alpha inversion, but we still want that gradient to be applied

    for (bufferCursor = (u8 *)g_TextBufferSurface->pixels; i < gradientArea; i++, bufferCursor += 4)
    {
        if (bufferCursor[3]) // A
        {
            bufferCursor[0] = bufferCursor[0] - bufferCursor[0] * i / gradientArea / 2; // R
            bufferCursor[1] = bufferCursor[1] - bufferCursor[1] * i / gradientArea / 2; // G
            bufferCursor[2] = bufferCursor[2] - bufferCursor[2] * i / gradientArea / 4; // B
        }
    }

    SDL_UnlockSurface(g_TextBufferSurface);

    return true;
}

// Text strings in asset files are encoded using Shift_JIS. This allows RenderTextToTexture to handle both UTF-8 and
// Shift_JIS. This also does not check for overlong encoding, but that shouldn't matter
bool isUTF8Encoded(const char *string)
{
    if(textNotExist) return true;
#define UTF8_1BYTE_MASK 0x80
#define UTF8_2BYTE_MASK 0xE0
#define UTF8_3BYTE_MASK 0xF0
#define UTF8_4BYTE_MASK 0xF8

#define UTF8_2NDBYTE_MASK 0xC0

// 0xxx xxxx
#define UTF8_1BYTE_PREFIX 0x00
// 110x xxxx
#define UTF8_2BYTE_PREFIX 0xC0
// 1110 xxxx
#define UTF8_3BYTE_PREFIX 0xE0
// 1111 0xxx
#define UTF8_4BYTE_PREFIX 0xF0

// 10xx xxxx
#define UTF8_2NDBYTE_PREFIX 0x80

    bool isMultiByteParse = false;
    int codepointLen = 0;

    while (*string != '\0')
    {
        unsigned char c = *(unsigned char *)string;

        if (!isMultiByteParse)
        {
            if ((c & UTF8_1BYTE_MASK) != UTF8_1BYTE_PREFIX)
            {
                isMultiByteParse = true;

                if ((c & UTF8_2BYTE_MASK) == UTF8_2BYTE_PREFIX)
                    codepointLen = 1;
                else if ((c & UTF8_3BYTE_MASK) == UTF8_3BYTE_PREFIX)
                    codepointLen = 2;
                else if ((c & UTF8_4BYTE_MASK) == UTF8_4BYTE_PREFIX)
                    codepointLen = 3;
                else
                    return false;
            }
        }
        else
        {
            if ((c & UTF8_2NDBYTE_MASK) != UTF8_2NDBYTE_PREFIX)
                return false;

            if (--codepointLen == 0)
                isMultiByteParse = false;
        }

        string++;
    }

    return true;

#undef UTF8_1BYTE_MASK
#undef UTF8_2BYTE_MASK
#undef UTF8_3BYTE_MASK
#undef UTF8_4BYTE_MASK

#undef UTF8_2NDBYTE_MASK

#undef UTF8_1BYTE_PREFIX
#undef UTF8_2BYTE_PREFIX
#undef UTF8_3BYTE_PREFIX
#undef UTF8_4BYTE_PREFIX

#undef UTF8_2NDBYTE_PREFIX
}

void SurfaceOverwriteBlend(SDL_Surface *srcSurface, SDL_Surface *dstSurface, u32 x)
{
    if(textNotExist) return;
    // Source surface is A8R8G8B8
    // Dest surface is RGBA32
    // We want to overwrite dest unless source has alpha 0

    SDL_LockSurface(srcSurface);
    SDL_LockSurface(dstSurface);

    u32 *srcData = (u32 *)srcSurface->pixels;
    u8 *dstData = (u8 *)dstSurface->pixels;

    for (int i = 0; i < srcSurface->h; i++)
    {
        for (int j = 0; j < srcSurface->w; j++)
        {
            if ((srcData[j] & 0xFF000000) != 0)
            {
                dstData[i * dstSurface->pitch + (x + j) * 4] = (srcData[j] >> 16) & 0xFF;
                dstData[i * dstSurface->pitch + (x + j) * 4 + 1] = (srcData[j] >> 8) & 0xFF;
                dstData[i * dstSurface->pitch + (x + j) * 4 + 2] = srcData[j] & 0xFF;
                dstData[i * dstSurface->pitch + (x + j) * 4 + 3] = (srcData[j] >> 24) & 0xFF;
            }
        }

        srcData += srcSurface->pitch / 4;
    }

    SDL_UnlockSurface(dstSurface);
    SDL_UnlockSurface(srcSurface);
}

static const char* UTF8_Decode(
    const char* s,
    int* outCodepoint)
{
    unsigned char c =
        (unsigned char)s[0];

    if (c < 0x80)
    {
        *outCodepoint = c;
        return s + 1;
    }

    if ((c >> 5) == 0x6)
    {
        *outCodepoint =
            ((c & 0x1F) << 6) |
            (s[1] & 0x3F);

        return s + 2;
    }

    if ((c >> 4) == 0xE)
    {
        *outCodepoint =
            ((c & 0x0F) << 12) |
            ((s[1] & 0x3F) << 6) |
            (s[2] & 0x3F);

        return s + 3;
    }

    if ((c >> 3) == 0x1E)
    {
        *outCodepoint =
            ((c & 0x07) << 18) |
            ((s[1] & 0x3F) << 12) |
            ((s[2] & 0x3F) << 6) |
            (s[3] & 0x3F);

        return s + 4;
    }

    *outCodepoint = '?';

    return s + 1;
}

void TextHelper::RenderTextToTexture(i32 xPos, i32 yPos, i32 spriteWidth, i32 spriteHeight, i32 fontHeight,
                                     i32 fontWidth, ZunColor textColor, ZunColor shadowColor, const char *string,
                                     TextureData *outTexture)
{
    if(textNotExist) return;
    
    char convertedText[1024] = {0};
    SDL_Rect finalCopyDst;
    SDL_Rect finalCopySrc;
    SDL_Rect shadowRect;
    SDL_Rect textRect;

    if (!isUTF8Encoded(string))
    {
        char *utf8 = sjis2utf8(string);
        strncpy(convertedText, utf8,sizeof(convertedText));
        free(utf8);
    }
    else
    {
        strncpy(convertedText, string,sizeof(convertedText));
    }

    // TTF_SetFontSize(g_Font, fontHeight * 2);

    finalCopySrc.x = 0;
    finalCopySrc.y = 0;
    finalCopySrc.w = spriteWidth * 2 - 2;
    finalCopySrc.h = fontHeight * 2 - 2;

    SDL_FillRect(g_TextBufferSurface, &finalCopySrc, 0);

    if (shadowColor != COLOR_WHITE)
    {
        SDL_Surface *shadowText;

        // Render shadow.

        int surfaceW = 1024;
        int surfaceH = 128;

        shadowText = SDL_CREATE_RGB_SURFACE_COMPAT(
            surfaceW,
            surfaceH,
            SDL_PIXELFORMAT_RGBA32
        );

        if (shadowText != NULL)
        {
            SDL_FillRect(shadowText, NULL, 0);

            SDL_LockSurface(shadowText);

            Uint32 *pixels = (Uint32 *)shadowText->pixels;

            int ascent;
            int descent;
            int lineGap;

            stbtt_GetFontVMetrics(
                &g_Font,
                &ascent,
                &descent,
                &lineGap
            );

            int baseline =
                (int)(ascent * g_FontScale);

            int penX = 0;

            const char *ptr = convertedText;

            while (*ptr)
            {
                int codepoint;

                ptr = UTF8_Decode(
                    ptr,
                    &codepoint);

                int advanceWidth;
                int leftBearing;

                stbtt_GetCodepointHMetrics(
                    &g_Font,
                    codepoint,
                    &advanceWidth,
                    &leftBearing
                );

                int glyphW;
                int glyphH;

                int xoff;
                int yoff;

                unsigned char *bitmap =
                    stbtt_GetCodepointBitmap(
                        &g_Font,
                        0,
                        g_FontScale,
                        codepoint,
                        &glyphW,
                        &glyphH,
                        &xoff,
                        &yoff
                    );

                if (bitmap != NULL)
                {
                    for (int y = 0; y < glyphH; y++)
                    {
                        for (int x = 0; x < glyphW; x++)
                        {
                            unsigned char a =
                                bitmap[y * glyphW + x];

                            if (a == 0)
                                continue;

                            int dstX =
                                penX + x + xoff;

                            int dstY =
                                baseline + y + yoff;

                            if (dstX < 0 || dstY < 0 ||
                                dstX >= surfaceW ||
                                dstY >= surfaceH)
                            {
                                continue;
                            }

                            SDL_Color sdlShadowColor =
                                SDL_TEXT_COLOR_COMPAT(shadowColor);

                            pixels[
                                dstY * surfaceW + dstX
                            ] = SDL_MAP_RGBA_COMPAT(
                                shadowText->format,
                                sdlShadowColor.r,
                                sdlShadowColor.g,
                                sdlShadowColor.b,
                                a
                            );
                        }
                    }

                    stbtt_FreeBitmap(bitmap, NULL);
                }

                penX +=
                    (int)(advanceWidth * g_FontScale);
            }

            SDL_UnlockSurface(shadowText);

            shadowRect.x = xPos * 2 + 3;
            shadowRect.y = 2;
            shadowRect.w = shadowText->w;
            shadowRect.h = shadowText->h;

            SDL_BlitSurface(shadowText, NULL, g_TextBufferSurface, &shadowRect);

            SDL_FreeSurface(shadowText);
        }
    }

    SDL_Surface *regularText;

    int surfaceW = 1024;
    int surfaceH = 128;

    regularText = SDL_CREATE_RGB_SURFACE_COMPAT(
        surfaceW,
        surfaceH,
        SDL_PIXELFORMAT_RGBA32
    );

    if (regularText != NULL)
    {
        SDL_FillRect(regularText, NULL, 0);

        SDL_LockSurface(regularText);

        Uint32 *pixels = (Uint32 *)regularText->pixels;

        int ascent;
        int descent;
        int lineGap;

        stbtt_GetFontVMetrics(
            &g_Font,
            &ascent,
            &descent,
            &lineGap
        );

        int baseline =
            (int)(ascent * g_FontScale);

        int penX = 0;

        const char *ptr = convertedText;

        while (*ptr)
        {
            int codepoint;

            ptr = UTF8_Decode(
                ptr,
                &codepoint);

            int advanceWidth;
            int leftBearing;

            stbtt_GetCodepointHMetrics(
                &g_Font,
                codepoint,
                &advanceWidth,
                &leftBearing
            );

            int glyphW;
            int glyphH;

            int xoff;
            int yoff;

            unsigned char *bitmap =
                stbtt_GetCodepointBitmap(
                    &g_Font,
                    0,
                    g_FontScale,
                    codepoint,
                    &glyphW,
                    &glyphH,
                    &xoff,
                    &yoff
                );

            if (bitmap != NULL)
            {
                for (int y = 0; y < glyphH; y++)
                {
                    for (int x = 0; x < glyphW; x++)
                    {
                        unsigned char a =
                            bitmap[y * glyphW + x];

                        if (a == 0)
                            continue;

                        int dstX =
                            penX + x + xoff;

                        int dstY =
                            baseline + y + yoff;

                        if (dstX < 0 || dstY < 0 ||
                            dstX >= surfaceW ||
                            dstY >= surfaceH)
                        {
                            continue;
                        }

                        pixels[
                            dstY * surfaceW + dstX
                        ] = SDL_MAP_RGBA_COMPAT(
                            regularText->format,
                            (textColor >> 16) & 0xFF,
                            (textColor >> 8) & 0xFF,
                            textColor & 0xFF,
                            a
                        );
                    }
                }

                stbtt_FreeBitmap(bitmap, NULL);
            }

            penX +=
                (int)(advanceWidth * g_FontScale);
        }

        SDL_UnlockSurface(regularText);
    }

    if (regularText != NULL)
    {
        textRect.x = xPos * 2;
        textRect.y = 0;
        textRect.w = regularText->w;
        textRect.h = regularText->h;

        SurfaceOverwriteBlend(regularText, g_TextBufferSurface, xPos * 2);

        SDL_FreeSurface(regularText);
    }

    // Once we get an API abstraction layer for surface operations, this needs to change
    //   We really shouldn't be clobbering the texture format
    if (!outTexture->textureData || outTexture->format != TEX_FMT_A8R8G8B8)
    {
        free(outTexture->textureData);
        outTexture->textureData = (u8 *)malloc(outTexture->width * outTexture->height * 4);
        memset(outTexture->textureData, 0, outTexture->width * outTexture->height * 4);
    }

    outTexture->format = TEX_FMT_A8R8G8B8;
    // SDL_Surface *textureSurface = SDL_CreateRGBSurfaceWithFormatFrom(
    //     outTexture->textureData, outTexture->width, outTexture->height, SDL_BITSPERPIXEL(SDL_PIXELFORMAT_RGBA32),
    //     outTexture->width * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_RGBA32), SDL_PIXELFORMAT_RGBA32);

    SDL_Surface *textureSurface = SDL_CREATE_RGB_SURFACE_FROM_COMPAT(
        outTexture->textureData,
        outTexture->width,
        outTexture->height,
        32,
        outTexture->width * 4,
        SDL_PIXELFORMAT_RGBA32
    );

    InvertAlpha(0, 0, spriteWidth * 2, fontHeight * 2 + 6);

    finalCopyDst.x = 0;
    finalCopyDst.y = yPos;
    finalCopyDst.w = spriteWidth;
    finalCopyDst.h = 16;

    if (SDL_SoftStretch(g_TextBufferSurface, &finalCopySrc, textureSurface, &finalCopyDst) < 0)
    {
        // SDL_Log("SDL_BlitScaled failed! Error: %s", SDL_GetError());
    }

    g_AnmManager->SetCurrentTexture(outTexture->handle);

    g_GfxBackend->SetTextureImage(outTexture->width, outTexture->height, PIXEL_RGBA, PIXEL_UNSIGNED_BYTE,
                                        outTexture->textureData);

    SDL_FreeSurface(textureSurface);
    

    return;
}

// Extended to free all globals for text helper
void TextHelper::ReleaseTextBuffer()
{
    if(textNotExist) return;

    if (g_FontBuffer != NULL)
    {
        free(g_FontBuffer);
        g_FontBuffer = NULL;
    }

    if (g_TextBufferSurface != NULL)
    {
        SDL_FreeSurface(g_TextBufferSurface);
        g_TextBufferSurface = NULL;
    }

    return;
}