#pragma once

#include <SDL_video.h>

#include "GLFunc.hpp"
#include "inttypes.hpp"

// The internal resolution EoSD uses. 640x480. I can't think of any reason anyone sane
//   would want to change this
#define GAME_WINDOW_WIDTH (640)
#define GAME_WINDOW_HEIGHT (480)

// The actual resolution used for the output window and viewport scaling
//   At some point there should be a method to change this without recompiling but for now
//   this'll do

/*
#ifndef GAME_WINDOW_WIDTH_REAL
#define GAME_WINDOW_WIDTH_REAL (GAME_WINDOW_WIDTH)
#endif

#ifndef GAME_WINDOW_HEIGHT_REAL
#define GAME_WINDOW_HEIGHT_REAL (GAME_WINDOW_HEIGHT)
#endif

#define VIEWPORT_WIDTH GAME_WINDOW_WIDTH_REAL
#define VIEWPORT_OFF_X 0
#define VIEWPORT_HEIGHT GAME_WINDOW_HEIGHT_REAL
#define VIEWPORT_OFF_Y 0

// Aspect ratio wider than 4:3
#if (GAME_WINDOW_WIDTH_REAL * 3) > (GAME_WINDOW_HEIGHT_REAL * 4)
#undef VIEWPORT_WIDTH
#undef VIEWPORT_OFF_X
#define VIEWPORT_WIDTH ((u32)((GAME_WINDOW_HEIGHT_REAL / 3.0f) * 4.0f))
#define VIEWPORT_OFF_X ((GAME_WINDOW_WIDTH_REAL - VIEWPORT_WIDTH) / 2)
#elif (GAME_WINDOW_WIDTH_REAL * 3) < (GAME_WINDOW_HEIGHT_REAL * 4)
#undef VIEWPORT_HEIGHT
#undef VIEWPORT_OFF_Y
#define VIEWPORT_HEIGHT ((u32)((GAME_WINDOW_WIDTH_REAL / 4.0f) * 3.0f))
#define VIEWPORT_OFF_Y ((GAME_WINDOW_HEIGHT_REAL - VIEWPORT_HEIGHT) / 2)
#endif

#define WIDTH_RESOLUTION_SCALE (((f32)VIEWPORT_WIDTH) / GAME_WINDOW_WIDTH)
#define HEIGHT_RESOLUTION_SCALE (((f32)VIEWPORT_HEIGHT) / GAME_WINDOW_HEIGHT)
*/

enum RenderResult
{
    RENDER_RESULT_KEEP_RUNNING,
    RENDER_RESULT_EXIT_SUCCESS,
    RENDER_RESULT_EXIT_ERROR,
};

struct GameWindow
{
    RenderResult Render();
    static void Present();

    static void CreateGameWindow();
    static i32 InitD3dRendering();
    static void InitD3dDevice();

    SDL_Window *window;
    SDL_GLContext glContext;
    i32 isAppClosing;
    i32 lastActiveAppValue;
    i32 isAppActive;
    u8 curFrame;
    i32 screenSaveActive;
    i32 lowPowerActive;
    i32 powerOffActive;
    u32 renderBackendIndex;

    i32 GAME_WINDOW_WIDTH_REAL = GAME_WINDOW_WIDTH;
    i32 GAME_WINDOW_HEIGHT_REAL = GAME_WINDOW_HEIGHT;
    i32 VIEWPORT_WIDTH = GAME_WINDOW_WIDTH;
    i32 VIEWPORT_OFF_X = 0;
    i32 VIEWPORT_HEIGHT = GAME_WINDOW_HEIGHT;
    i32 VIEWPORT_OFF_Y = 0;
    f32 WIDTH_RESOLUTION_SCALE;
    f32 HEIGHT_RESOLUTION_SCALE;
    void CONFIGURE_VIEW(){
        this->VIEWPORT_WIDTH = GAME_WINDOW_WIDTH_REAL;
        this->VIEWPORT_HEIGHT = GAME_WINDOW_HEIGHT_REAL;
        if ((this->GAME_WINDOW_WIDTH_REAL * 3) > (this->GAME_WINDOW_HEIGHT_REAL * 4)){
            this->VIEWPORT_WIDTH=(u32)((this->GAME_WINDOW_HEIGHT_REAL / 3.0f) * 4.0f);
            this->VIEWPORT_OFF_X=((this->GAME_WINDOW_WIDTH_REAL - this->VIEWPORT_WIDTH) / 2);
        }else if((this->GAME_WINDOW_WIDTH_REAL * 3) < (this->GAME_WINDOW_HEIGHT_REAL * 4)){
            this->VIEWPORT_HEIGHT=(u32)((this->GAME_WINDOW_WIDTH_REAL / 4.0f) * 3.0f);
            this->VIEWPORT_OFF_Y=((this->GAME_WINDOW_HEIGHT_REAL - this->VIEWPORT_HEIGHT) / 2);
        }
        this->WIDTH_RESOLUTION_SCALE=((f32)this->VIEWPORT_WIDTH) / GAME_WINDOW_WIDTH;
        this->HEIGHT_RESOLUTION_SCALE=((f32)this->VIEWPORT_HEIGHT) / GAME_WINDOW_HEIGHT;
    }
};

extern GameWindow g_GameWindow;
extern i32 g_TickCountToEffectiveFramerate;
extern double g_LastFrameTime;
