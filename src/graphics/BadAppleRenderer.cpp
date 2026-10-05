#include "BadAppleRendererHeader.hpp"

GfxInterface *BadAppleRenderer::Init()
{
    BadAppleRenderer* gfx = new BadAppleRenderer;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        delete gfx;
        return NULL;
    }

    #if SDL_MAJOR_VERSION == 1
    u32 flags = SDL_HWSURFACE;
    #else
    u32 flags = 0;
    #endif

    if (g_Supervisor.cfg.windowed == 0)
    {
        flags |= SDL_FULLSCREEN_COMPAT;
    }

    g_GameWindow.ConfigureInit();

    // todo : fix scaling
    GAME_WINDOW_WIDTH_REAL=GAME_WINDOW_WIDTH/2;
    GAME_WINDOW_HEIGHT_REAL=GAME_WINDOW_HEIGHT/2;
    GAME_WINDOW_REFRESH_RATE=30;
    g_GameWindow.ConfigureView();

    i32 width  = GAME_WINDOW_WIDTH_REAL;
    i32 height = GAME_WINDOW_HEIGHT_REAL;

    #if SDL_MAJOR_VERSION >= 2
    gfx->rgbaFramebuffer = new u32[width * height];
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
    #if SDL_MAJOR_VERSION == 2
    SDL_RenderSetLogicalSize(gfx->renderer, width, height);
    #elif SDL_MAJOR_VERSION == 3
    SDL_SetRenderLogicalPresentation(
        gfx->renderer,
        width,
        height,
        SDL_LOGICAL_PRESENTATION_LETTERBOX
    );
    #endif
    if (gfx->renderer == NULL)
    {
        delete gfx;
        return NULL;
    }
    SDL_Texture* framebufferTexture = SDL_CreateTexture(gfx->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GAME_WINDOW_WIDTH_REAL, GAME_WINDOW_HEIGHT_REAL);
    gfx->framebufferTexture = framebufferTexture;
    if (framebufferTexture == NULL){
        delete gfx;
        return NULL;
    }
    #else

    gfx->screen = SDL_SetVideoMode(
        width,
        height,
        8,
        flags
    );
    SDL_Color colors[256];
    memset(colors, 0, sizeof(colors));

    colors[0].r = 0;
    colors[0].g = 0;
    colors[0].b = 0;

    colors[1].r = 0;
    colors[1].g = 0;
    colors[1].b = 0;

    colors[2].r = 255;
    colors[2].g = 255;
    colors[2].b = 255;

    if (SDL_SetColors(gfx->screen, colors, 0, 256) == SDL_FALSE)
    {
        delete gfx;
        return NULL;
    }

    if (!gfx->screen)
    {
        delete gfx;
        return NULL;
    }
    #endif

    // Can't init on header, init on creation instead
    InitColorOpTableBadApple();
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

    gfx->framebuffer = new u8[width * height];

    gfx->noVertexBuffer =
        (g_Supervisor.cfg.opts & (1 << GCOS_DONT_USE_VERTEX_BUF))!=0;

    gfx->noFog =
        (g_Supervisor.cfg.opts & (1 << GCOS_DONT_USE_FOG))!=0;

    gfx->InitFlattenedMatrix();
    return gfx;
}

bool BadAppleRenderer::GameLoop(){
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


void BadAppleRenderer::Exit()
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

void BadAppleRenderer::SwapBuffers()
{
    const i32 width = GAME_WINDOW_WIDTH_REAL;
    const i32 height = GAME_WINDOW_HEIGHT_REAL;

    #if SDL_MAJOR_VERSION >= 2
    const i32 count = width * height;
    i32 i;
    for (i = 0; i < count; ++i)
    {
        switch (framebuffer[i])
        {
        case 2:
            rgbaFramebuffer[i] = 0xFFFFFFFF; // White
            break;

        case 0:
        case 1:
        default:
            rgbaFramebuffer[i] = 0xFF000000; // Black
            break;
        }
    }

    SDL_UpdateTexture(framebufferTexture, NULL, rgbaFramebuffer, GAME_WINDOW_WIDTH_REAL * sizeof(u32));
    SDL_RenderCopy(renderer, framebufferTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
    #else
    SDL_LockSurface(screen);
    for (i32 y = 0; y < height; y++)
    {
        memcpy(
            (u8*)screen->pixels + y * screen->pitch,
            framebuffer + y * width,
            width
        );
    }
    SDL_UnlockSurface(screen);
    SDL_Flip(screen);
    #endif
}

#include "BadAppleRendererCore.hpp"