#include "Software.hpp"
#include "Supervisor.hpp"
#include "GameWindow.hpp"
#include "i18n.hpp"
#include <algorithm>
#include <stddef.h>
#include "utils.hpp"
#include "compat/Compat.hpp"
#include <math.h>

u8 alphaThreshold = 4;

GfxInterface *Software::Init()
{
    Software* gfx = new Software;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        delete gfx;
        return NULL;
    }

    #if SDL_MAJOR_VERSION == 1
    u32 flags = SDL_SWSURFACE;
    #else
    u32 flags = 0;
    #endif

    if (g_Supervisor.cfg.windowed == 0)
    {
        flags |= SDL_FULLSCREEN_COMPAT;
    }

    g_GameWindow.ConfigureInit();

#ifdef __ANDROID__
    GetWindowSize(
        &GAME_WINDOW_WIDTH_REAL,
        &GAME_WINDOW_HEIGHT_REAL,
        &GAME_WINDOW_REFRESH_RATE
    );
#endif
    g_GameWindow.ConfigureView();

    i32 width  = GAME_WINDOW_WIDTH_REAL;
    i32 height = GAME_WINDOW_HEIGHT_REAL;

    #if SDL_MAJOR_VERSION >= 2
    i32 x = SDL_WINDOWPOS_UNDEFINED_COMPAT;
    i32 y = SDL_WINDOWPOS_UNDEFINED_COMPAT;
    gfx->window = SDL_CreateWindowCompat(TH_WINDOW_TITLE, x, y, width, height, flags);

    if (gfx->window == NULL)
    {
        delete gfx;
        return NULL;
    }

    //SDL 2 are on 3th variable... i'm trying SDL_RENDERER_SOFTWARE
    //SDL 3 are on 4th variable... i'm trying "direct3d12"
    //set 4th var into NULL to get the fastest SDL rendering
    gfx->renderer = SDL_CreateRendererCompat(
        gfx->window, 
        -1, 
        SDL_RENDERER_ACCELERATED, 
        NULL
    );
    if (gfx->renderer == NULL)
    {
        delete gfx;
        return NULL;
    }
    SDL_Texture* framebufferTexture = SDL_CreateTexture(gfx->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GAME_WINDOW_WIDTH_REAL, GAME_WINDOW_HEIGHT_REAL);
    gfx->framebufferTexture = framebufferTexture;
    if (framebufferTexture == NULL)    {
        delete gfx;
        return NULL;
    }
    #else

    gfx->screen = SDL_SetVideoMode(
        width,
        height,
        32,
        flags
    );

    if (!gfx->screen)
    {
        delete gfx;
        return NULL;
    }
    #endif

    // Can't init on header, init on creation instead
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

    gfx->framebuffer = new u32[width * height];

    gfx->noVertexBuffer =
        (g_Supervisor.cfg.opts & (1 << GCOS_DONT_USE_VERTEX_BUF))!=0;

    gfx->noFog =
        (g_Supervisor.cfg.opts & (1 << GCOS_DONT_USE_FOG))!=0;

    return gfx;
}

bool Software::GameLoop(){
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
        {
            return false;
        }
    }
    return true;
}


void Software::Exit()
{
    #if SDL_MAJOR_VERSION >= 2
    if (this->renderer)
    {
        SDL_DestroyRenderer(this->renderer);
        this->renderer = NULL;
    }
    if (this->window)
    {
        SDL_DestroyWindow(this->window);
        this->window = NULL;
    }
    if (this->framebufferTexture)
    {
        SDL_DestroyTexture(this->framebufferTexture);
        this->framebufferTexture = NULL;
    }
    if (this->framebuffer)    {
        delete[] this->framebuffer;
        this->framebuffer = NULL;
    }
    #else
    delete[] framebuffer;
    framebuffer = NULL;

    SDL_Quit();
    #endif
}

void Software::SwapBuffers()
{
    #if SDL_MAJOR_VERSION >= 2
    SDL_UpdateTexture(framebufferTexture, NULL, framebuffer, GAME_WINDOW_WIDTH_REAL * sizeof(u32));
    SDL_RenderCopy(renderer, framebufferTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
    #else
    SDL_LockSurface(screen);
    const i32 width  = GAME_WINDOW_WIDTH_REAL;
    const i32 height = GAME_WINDOW_HEIGHT_REAL;
    for (i32 y = 0; y < height; y++)
    {
        memcpy(
            (u8*)screen->pixels + y * screen->pitch,
            framebuffer + y * width,
            width * sizeof(u32)
        );
    }
    SDL_UnlockSurface(screen);
    SDL_Flip(screen);
    #endif
}

#include "Software2DCore.hpp"