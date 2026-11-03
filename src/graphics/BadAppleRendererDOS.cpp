#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdlib>

#include <dpmi.h>
#include <sys/nearptr.h>
#include <conio.h>

#define BAD_APPLE_FRAMEBUFFER_TYPE unsigned char
#define BAD_APPLE_COLOR_BLACK 0
#define BAD_APPLE_COLOR_WHITE 15
#define BAD_APPLE_COLOR_TRANSPARENT 4 // iykyk

#include "BadAppleRendererHeader.hpp"

BAD_APPLE_FRAMEBUFFER_TYPE *badAppleFramebuffer = NULL;

// VGA Mode 13h framebuffer
static unsigned char *vgaFramebuffer = NULL;
static bool nearptrEnabled = false;
static void SetVideoMode(int mode)
{
    __dpmi_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.x.ax = mode;
    __dpmi_int(0x10, &regs);
}

bool BadAppleRenderer::GameLoop()
{
    if (kbhit())
    {
        int key = getch();
        if (key == 27) // ESC
            return false;
    }

    return true;
}

void BadAppleRenderer::Exit()
{
    if (badAppleFramebuffer)
    {
        delete[] badAppleFramebuffer;
        badAppleFramebuffer = NULL;
    }

    // Restore DOS text mode.
    SetVideoMode(0x03);

    vgaFramebuffer = NULL;

    // Disable direct conventional memory access.
    if (nearptrEnabled)
    {
        __djgpp_nearptr_disable();
        nearptrEnabled = false;
    }
}

GfxInterface* BadAppleRenderer::Init()
{
    BadAppleRenderer* gfx = new BadAppleRenderer;

    // Enable direct access to conventional memory.
    if (!__djgpp_nearptr_enable())
    {
        std::cerr << "Cannot enable DJGPP near pointer access!\n";

        delete gfx;
        return NULL;
    }

    nearptrEnabled = true;

    // Enter VGA Mode 13h.
    SetVideoMode(0x13);

    // Map VGA memory at physical address 0xA0000.
    vgaFramebuffer =
        (unsigned char*)(0xA0000UL + __djgpp_conventional_base);

    // Configure renderer.
    g_GameWindow.ConfigureInit();

    GAME_WINDOW_WIDTH_REAL = 320;
    GAME_WINDOW_HEIGHT_REAL = 200;
    GAME_WINDOW_REFRESH_RATE = 15;

    g_GameWindow.ConfigureView();

    int width = GAME_WINDOW_WIDTH_REAL;
    int height = GAME_WINDOW_HEIGHT_REAL;

    // Allocate software framebuffer.
    badAppleFramebuffer =
        new BAD_APPLE_FRAMEBUFFER_TYPE[width * height];

    memset(badAppleFramebuffer, 0, width * height);

    // Initialize renderer.
    InitColorOpTableBadApple();

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

    gfx->noVertexBuffer =
        (g_Supervisor.cfg.opts & (1 << GCOS_DONT_USE_VERTEX_BUF)) != 0;

    gfx->noFog =
        (g_Supervisor.cfg.opts & (1 << GCOS_DONT_USE_FOG)) != 0;

    gfx->InitFlattenedMatrix();

    return gfx;
}

// --------------------------------------------------
// Swap buffers
// --------------------------------------------------

void BadAppleRenderer::SwapBuffers()
{
    if (!badAppleFramebuffer || !vgaFramebuffer)
        return;

    // Convert software framebuffer to VGA pixels.
    memcpy(vgaFramebuffer, badAppleFramebuffer, 64000);
}

#include "BadAppleRendererCore.hpp"