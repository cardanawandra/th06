#include <windows.h>
#include <iostream>
#include <string>
#include <algorithm>
#define BAD_APPLE_FRAMEBUFFER_TYPE u8
#define BAD_APPLE_COLOR_BLACK 0x00
#define BAD_APPLE_COLOR_WHITE 0xff
#define BAD_APPLE_COLOR_TRANSPARENT 0xf0 //<- WHAT THE HELL

#include "BadAppleRendererHeader.hpp"
BAD_APPLE_FRAMEBUFFER_TYPE *badAppleFramebuffer;

std::string output;
bool BadAppleRenderer::GameLoop(){
    MSG msg;

    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            return false;
        }

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
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

    // Restore cursor
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_CURSOR_INFO cursor;
    cursor.dwSize = 25;
    cursor.bVisible = TRUE;

    SetConsoleCursorInfo(console, &cursor);

    // Reset console colors
    SetConsoleTextAttribute(
        GetStdHandle(STD_OUTPUT_HANDLE),
        FOREGROUND_RED |
        FOREGROUND_GREEN |
        FOREGROUND_BLUE |
        FOREGROUND_INTENSITY
    );

    // Clear console
    system("cls");
}

GfxInterface* BadAppleRenderer::Init()
{
    BadAppleRenderer* gfx = new BadAppleRenderer;

    // Initialize console
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleOutputCP(437);

    // Hide cursor
    CONSOLE_CURSOR_INFO cursor;
    cursor.dwSize = 1;
    cursor.bVisible = FALSE;

    SetConsoleCursorInfo(console, &cursor);

    // Configure framebuffer
    g_GameWindow.ConfigureInit();

    GAME_WINDOW_WIDTH_REAL = GAME_WINDOW_WIDTH / 4;
    GAME_WINDOW_HEIGHT_REAL = GAME_WINDOW_HEIGHT / 4;
    GAME_WINDOW_REFRESH_RATE = 30;

    g_GameWindow.ConfigureView();

    int width = GAME_WINDOW_WIDTH_REAL;
    int height = GAME_WINDOW_HEIGHT_REAL;

    badAppleFramebuffer =
        new BAD_APPLE_FRAMEBUFFER_TYPE[width * height];
    output.reserve((width + 1) * (height / 2));

    // Initialize renderer
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

void BadAppleRenderer::SwapBuffers()
{
    output.clear();

    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    for (int y = 0; y < GAME_WINDOW_HEIGHT_REAL; y += 2)
    {
        for (int x = 0; x < GAME_WINDOW_WIDTH_REAL; x++)
        {
            bool top = badAppleFramebuffer[y * GAME_WINDOW_WIDTH_REAL + x] == BAD_APPLE_COLOR_WHITE;
            bool bottom = (y + 1 < GAME_WINDOW_HEIGHT_REAL) &&
                badAppleFramebuffer[(y + 1) * GAME_WINDOW_WIDTH_REAL + x] == BAD_APPLE_COLOR_WHITE;

            output += top
                ? (bottom ? "\xDB" : "\xDF")
                : (bottom ? "\xDC" : " ");
        }

        output += '\n';
    }

    COORD pos = { 0, 0 };
    SetConsoleCursorPosition(console, pos);

    DWORD written;
    WriteConsoleA(console, output.c_str(), (DWORD)output.size(), &written, NULL);
}

#include "BadAppleRendererCore.hpp"