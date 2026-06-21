#include "Software.hpp"
#include "Supervisor.hpp"
#include "GameWindow.hpp"
#include "i18n.hpp"
#include <algorithm>
#include <stddef.h>
#include "utils.hpp"
#include "compat/Compat.hpp"
#include <math.h>
#include <windows.h>

u8 alphaThreshold = 4;

static HWND g_hWnd = NULL;

HWND hwnd;
HDC windowDC;
HDC memDC;
BITMAPINFO bmi;
HBITMAP hBitmap;
void* dibPixels = NULL;

LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(
        hwnd,
        msg,
        wParam,
        lParam);
}

GfxInterface* Software::Init()
{
    Software* gfx = new Software;

    g_GameWindow.ConfigureInit();
    GAME_WINDOW_WIDTH_REAL;
    GAME_WINDOW_HEIGHT_REAL;
    g_GameWindow.ConfigureView();
    int width  = GAME_WINDOW_WIDTH_REAL;
    int height = GAME_WINDOW_HEIGHT_REAL;

    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));

    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = "SoftwareRenderer";

    RegisterClass(&wc);

    DWORD style;

    if (g_Supervisor.cfg.windowed)
        style = WS_OVERLAPPEDWINDOW;
    else
        style = WS_POPUP;

    RECT rc;
    rc.left   = 0; 
    rc.top    = 0;
    rc.right  = width;
    rc.bottom = height;

    AdjustWindowRect(&rc, style, FALSE);

    g_hWnd = CreateWindow(
        "SoftwareRenderer",
        TH_WINDOW_TITLE,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    if (!g_hWnd)
    {
        delete gfx;
        return NULL;
    }

    hwnd = g_hWnd;

    ZeroMemory(&bmi, sizeof(BITMAPINFO));

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;   // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    windowDC = GetDC(hwnd);

    hBitmap = CreateDIBSection(
        windowDC,
        &bmi,
        DIB_RGB_COLORS,
        &dibPixels,
        NULL,
        0);

    memDC = CreateCompatibleDC(windowDC);

    SelectObject(memDC,hBitmap);

    if (hBitmap == NULL || memDC == NULL)
    {
        delete gfx;
        return NULL;
    }

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);

    gfx->boundTexture = NULL;
    gfx->clearDepth   = 1.0f;
    gfx->useTexCoord  = false;
    gfx->useDiffuse   = false;

    gfx->model.Identity();
    gfx->view.Identity();
    gfx->projection.Identity();
    gfx->textureMatrix.Identity();

    gfx->textures.reserve(1024);
    gfx->freeTextures.reserve(1024);

    gfx->framebuffer = (u32*)dibPixels;
    if (gfx->framebuffer == NULL)
    {
        delete gfx;
        return NULL;
    }

    gfx->noVertexBuffer =
        (g_Supervisor.cfg.opts & (1 << GCOS_DONT_USE_VERTEX_BUF)) != 0;

    gfx->noFog =
        (g_Supervisor.cfg.opts & (1 << GCOS_DONT_USE_FOG)) != 0;

    return gfx;
}

bool Software::GameLoop()
{
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            return false;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return true;
}

void Software::Exit()
{
    if (hBitmap)
    {
        DeleteObject(hBitmap);
        hBitmap = NULL;
    }
    if (memDC)
    {
        DeleteDC(memDC);
        memDC = NULL;
    }
    if (windowDC)
    {
        ReleaseDC(hwnd, windowDC);
        windowDC = NULL;
    }
    if (hwnd)
    {
        DestroyWindow(hwnd);
        hwnd = NULL;
    }
}

void Software::SwapBuffers()
{
    BitBlt(
        windowDC,
        0,
        0,
        GAME_WINDOW_WIDTH_REAL,
        GAME_WINDOW_HEIGHT_REAL,
        memDC,
        0,
        0,
        SRCCOPY);
}

#include "Software2DCore.hpp"