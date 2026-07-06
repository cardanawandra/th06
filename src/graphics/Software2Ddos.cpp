#include "Software2DHeader.hpp"
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include <conio.h>
#include <pc.h>
// struct Color
// {
//     u8 r, g, b;
// };

// Color palette[256];
// void InitPallete()
// {
//     outportb(0x3C8, 0);

//     for (int i = 0; i < 256; i++)
//     {
//         outportb(0x3C9, palette[i].r >> 2);
//         outportb(0x3C9, palette[i].g >> 2);
//         outportb(0x3C9, palette[i].b >> 2);
//     }
// }
GfxInterface *Software::Init()
{
    Software* gfx = new Software;

    // __dpmi_regs r;
    // r.x.ax = 0x0013;          // VGA 320x200x256
    // __dpmi_int(0x10, &r);
    GAME_WINDOW_WIDTH_REAL/=4;
    GAME_WINDOW_HEIGHT_REAL/=4;
    GAME_WINDOW_REFRESH_RATE/=4;


    gfx->framebuffer = new u32[GAME_WINDOW_WIDTH_REAL * GAME_WINDOW_HEIGHT_REAL];

    // InitColorOpTable();

    gfx->boundTexture=NULL;
    gfx->clearDepth = 1.0f;
    gfx->useTexCoord = false;
    gfx->useDiffuse = false;

    gfx->model.Identity();
    gfx->view.Identity();
    gfx->projection.Identity();
    gfx->textureMatrix.Identity();

    gfx->textures.reserve(1024);
    gfx->freeTextures.reserve(1024);

    gfx->noVertexBuffer =
        (g_Supervisor.cfg.opts & (1 << GCOS_DONT_USE_VERTEX_BUF))!=0;

    gfx->noFog =
        (g_Supervisor.cfg.opts & (1 << GCOS_DONT_USE_FOG))!=0;

    gfx->InitFlattenedMatrix();
    // InitPallete();
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
    if (framebuffer)
    {
        delete[] framebuffer;
        framebuffer = nullptr;
    }

    __dpmi_regs r;
    r.x.ax = 0x0003;          // Restore text mode
    __dpmi_int(0x10, &r);

    delete this;
}

// u8 RGBToPalette(u8 r, u8 g, u8 b)
// {
//     int best = 0;
//     int bestDist = 0x7FFFFFFF;

//     for (u8 i = 0; i < 256; i++)
//     {
//         int dr = r - palette[i].r;
//         int dg = g - palette[i].g;
//         int db = b - palette[i].b;

//         int dist = dr * dr + dg * dg + db * db;

//         if (dist < bestDist)
//         {
//             bestDist = dist;
//             best = i;
//         }
//     }

//     return (u8)best;
// }

static const char shades[] = " .:-=+*#%@";

void Software::SwapBuffers()
{
    // clrscr();

    // for (int cy = 0; cy < 25; cy++)
    // {
    //     gotoxy(1, cy + 1);

    //     for (int cx = 0; cx < 80; cx++)
    //     {
    //         int sum = 0;

    //         // Sample a 4x8 block
    //         for (int y = 0; y < 8; y++)
    //         {
    //             for (int x = 0; x < 4; x++)
    //             {
    //                 u32 c = framebuffer[(cy * 8 + y) * 320 + (cx * 4 + x)];

    //                 int r = (c >> 16) & 255;
    //                 int g = (c >> 8) & 255;
    //                 int b = c & 255;

    //                 sum += (r + g + b) / 3;
    //             }
    //         }

    //         int avg = sum / 32;

    //         int idx = avg * 9 / 255;
    //         putchar(shades[idx]);
    //     }
    // }
}

// void Software::SwapBuffers()
// {
//     static u8 vgaBuffer[320 * 200];

//     for (int i = 0; i < 320 * 200; i++)
//     {
//         u32 argb = framebuffer[i];

//         u8 r = (argb >> 16) & 0xFF;
//         u8 g = (argb >> 8)  & 0xFF;
//         u8 b = (argb >> 0)  & 0xFF;

//         vgaBuffer[i] = RGBToPalette(r, g, b);
//     }

//     dosmemput(vgaBuffer, 320 * 200, 0xA0000);
// }

#include "Software2DCore.hpp"