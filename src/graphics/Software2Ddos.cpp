#include "Software.hpp"
#include "Supervisor.hpp"
#include "GameWindow.hpp"
#include "i18n.hpp"
#include <algorithm>
#include <stddef.h>
#include "utils.hpp"
#include "compat/Compat.hpp"
#include <math.h>
#include <go32.h>
#include <dpmi.h>
#include <sys/farptr.h>
#include <conio.h>

u8 alphaThreshold = 4;

u8 ColorOpTable[3][256][256];
u8 ColorAddTable[256][256];
u8 ColorMulTable[256][256];
u8 ColorClamp510[511];
u8 ColorDA[2][256];
void InitColorOpTable()
{
    i16 i;
    for (i=0;i < 256; i++){
        ColorDA[BLEND_INV_SRC_ALPHA][i] = 255-i;
        ColorDA[BLEND_ONE][i] = 255;
    }
    //clamp 255+255
    for (i=0;i<=510;i++)
        ColorClamp510[i]=(i>255)?255:i;
    u16 factor, value;
    for (factor = 0; factor < 256; ++factor)
    {
        for (value = 0; value < 256; ++value)
        {
            //ColorMulTable
            ColorMulTable[factor][value] = (factor * value + 128) >> 8;

            // Modulate
            ColorOpTable[COLOR_OP_MODULATE][factor][value] =
                u8((value * factor) >> 8);

            // Add
            ColorOpTable[COLOR_OP_ADD][factor][value] = ColorClamp510[factor*value];

            // replace (bruh)
            ColorOpTable[COLOR_OP_REPLACE][factor][value] = value;
        }
    }
}

GfxInterface *Software::Init()
{
    Software *gfx = new Software;

    const int width  = GAME_WINDOW_WIDTH_REAL;
    const int height = GAME_WINDOW_HEIGHT_REAL;

    //
    // Set graphics mode
    //

    __dpmi_regs regs;

    // VGA Mode 13h (320x200x256)
    regs.x.ax = 0x0013;
    __dpmi_int(0x10, &regs);

    gfx->boundTexture = NULL;
    gfx->clearDepth = 1.0f;
    gfx->useTexCoord = false;
    gfx->useDiffuse = false;

    gfx->model.Identity();
    gfx->view.Identity();
    gfx->projection.Identity();
    gfx->textureMatrix.Identity();

    gfx->textures.reserve(1024);
    gfx->freeTextures.reserve(1024);

    gfx->framebuffer = new u32[width * height];

    gfx->noVertexBuffer =
        (g_Supervisor.cfg.opts &
        (1 << GCOS_DONT_USE_VERTEX_BUF)) != 0;

    gfx->noFog =
        (g_Supervisor.cfg.opts &
        (1 << GCOS_DONT_USE_FOG)) != 0;

    InitColorOpTable();

    return gfx;
}

bool Software::GameLoop()
{
    if (kbhit())
    {
        int key = getch();

        // ESC quits
        if (key == 27)
            return false;
    }

    return true;
}


void Software::Exit()
{
    __dpmi_regs regs;

    // Restore 80x25 text mode
    regs.x.ax = 0x0003;
    __dpmi_int(0x10, &regs);

    delete[] framebuffer;
    framebuffer = NULL;
}

static unsigned char vga[320 * 200];

void Software::SwapBuffers()
{
    for (int y = 0; y < 200; y++)
    {
        int sy = y * GAME_WINDOW_HEIGHT / 200;

        for (int x = 0; x < 320; x++)
        {
            int sx = x * GAME_WINDOW_WIDTH / 320;

            u32 c = framebuffer[sy * GAME_WINDOW_WIDTH + sx];

            unsigned char r = (c >> 16) & 255;
            unsigned char g = (c >> 8) & 255;
            unsigned char b = c & 255;

            // crude grayscale
            vga[y * 320 + x] = (r + g + b) / 12;
        }
    }

    dosmemput(vga, 320 * 200, 0xA0000);
}
// void Software::SwapBuffers()
// {
//     dosmemput(framebuffer, 320 * 200, 0xA0000);
// }

#include "Software2DCore.hpp"