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

std::wstring output;
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

HWND editControl;

BOOL CALLBACK FindNotepadEditor(HWND hwnd, LPARAM)
{
    wchar_t className[256] = {};

    GetClassNameW(hwnd, className, 256);

    // Look for modern Notepad RichEdit controls
    if (wcscmp(className, L"RichEditD2DPT") == 0 ||
        wcscmp(className, L"Edit") == 0)
    {
        editControl = hwnd;
        return FALSE;
    }

    return TRUE;
}

GfxInterface* BadAppleRenderer::Init()
{
    BadAppleRenderer* gfx = new BadAppleRenderer;

    // Find Notepad window
    HWND notepadWindow = FindWindow(TEXT("Notepad"), nullptr);
    if (!notepadWindow) {
        printf("No Notepad window found.\n");
        delete gfx;
        return NULL;
    }

    // Call the edit notepad Windows API
    editControl = NULL;
    EnumChildWindows(
        notepadWindow,
        FindNotepadEditor,
        0
    );

    if (!editControl) {
        printf("Failed to find the edit control in Notepad.\n");
        delete gfx;
        return NULL;
    }

    // Process each frame
    printf("Processing video and sending frames to Notepad...\n");

    // Configure framebuffer
    g_GameWindow.ConfigureInit();

    GAME_WINDOW_WIDTH_REAL = GAME_WINDOW_WIDTH / 4;
    GAME_WINDOW_HEIGHT_REAL = GAME_WINDOW_HEIGHT / 4;
    GAME_WINDOW_REFRESH_RATE = 15;

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

    const int width = GAME_WINDOW_WIDTH_REAL;
    const int height = GAME_WINDOW_HEIGHT_REAL;

    for (int y = 0; y < GAME_WINDOW_HEIGHT_REAL; y += 2)
    {
        for (int x = 0; x < GAME_WINDOW_WIDTH_REAL; x++)
        {
            bool top = badAppleFramebuffer[y * GAME_WINDOW_WIDTH_REAL + x] == BAD_APPLE_COLOR_WHITE;
            bool bottom = (y + 1 < GAME_WINDOW_HEIGHT_REAL) &&
                badAppleFramebuffer[(y + 1) * GAME_WINDOW_WIDTH_REAL + x] == BAD_APPLE_COLOR_WHITE;

            output += top
                ? (bottom ? L"\u2588" : L"\u2580")
                : (bottom ? L"\u2584" : L" ");
        }

        output += L"\r\n";
    }

    SendMessageW(
        editControl,
        WM_SETTEXT,
        0,
        (LPARAM)output.c_str()
    );
}

#include "BadAppleRendererCore.hpp"