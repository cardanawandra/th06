#include <stdio.h>
#include <SDL.h>

#include "AnmManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GamePaths.hpp"
#include "GameWindow.hpp"
#include "MidiOutput.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "TextHelper.hpp"
#include "ZunResult.hpp"
#include "i18n.hpp"
#include "utils.hpp"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#include <iostream>
FILE _iob[3] = { *stdin, *stdout, *stderr }; 
extern "C" FILE* __cdecl __iob_func(void)
{
    return _iob;
}
#endif

int main(int argc, char *argv[])
{
    SDL_LOG_COMPAT("Starting");
    i32 renderResult = 0;

#ifdef __ANDROID__
    // On Android, SDL must be initialized before GamePaths::Init()
    // because SDL_AndroidGetInternalStoragePath() requires SDL_Init.
    if (SDL_Init(0) < 0)
    {
        return 1;
    }
#endif

    SDL_LOG_COMPAT("Init Gamepath");
    GamePaths::Init();

    // if (utils::CheckForRunningGameInstance())
    // {
    //     g_GameErrorContext.Flush();

    //     return 1;
    // }

    SDL_LOG_COMPAT("Load CONF File");
    if (g_Supervisor.LoadConfig(TH_CONFIG_FILE) != ZUN_SUCCESS)
    {
#ifdef __ANDROID__
        // On Android, config file may not exist on first run.
        // LoadConfig sets defaults and tries to write — if write fails,
        // continue anyway with defaults.
        SDL_LOG_COMPAT("LoadConfig failed (first run?), continuing with defaults");
#else
        g_GameErrorContext.Flush();
        return -1;
#endif
    }

    // if (GameWindow::InitD3dInterface())
    // {
    //     g_GameErrorContext.Flush();
    //     return 1;
    // }
    SDL_LOG_COMPAT("Start the game");

restart:
    SDL_LOG_COMPAT("Create game window");
    GameWindow::CreateGameWindow();

    SDL_LOG_COMPAT("new AnmManager");
    g_AnmManager = new AnmManager();

    SDL_LOG_COMPAT("InitD3dRendering");
    if (GameWindow::InitD3dRendering())
    {
        g_GameErrorContext.Flush();
        return 1;
    }

    SDL_LOG_COMPAT("InitializeDSound");
    g_SoundPlayer.InitializeDSound();
    SDL_LOG_COMPAT("GetJoystickCaps");
    Controller::GetJoystickCaps();
    SDL_LOG_COMPAT("ResetKeyboard");
    Controller::ResetKeyboard();

    SDL_LOG_COMPAT("Supervisor::RegisterChain");
    if (Supervisor::RegisterChain() != ZUN_SUCCESS)
    {
        goto stop;
    }
    if (!g_Supervisor.cfg.windowed)
    {
        SDL_HIDECURSOR_COMPAT();
    }

    g_GameWindow.curFrame = 0;

    SDL_LOG_COMPAT("Into loop game event");
    while (true)
    {
        SDL_Event e;

        SDL_LOG_COMPAT("Into poolevent loop");
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                goto stop;
            }
        }

        SDL_LOG_COMPAT("g_GameWindow.Render");
        renderResult = g_GameWindow.Render();
        if (renderResult != 0)
        {
            break;
        }

        //        SDL_Delay(1000.0f / 60.0f);

        //        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        //        {
        //            TranslateMessage(&msg);
        //            DispatchMessage(&msg);
        //        }
        //        else
        //        {
        //            testCoopLevelRes = g_Supervisor.d3dDevice->TestCooperativeLevel();
        //            if (testCoopLevelRes == D3D_OK)
        //            {
        //                renderResult = g_GameWindow.Render();
        //                if (renderResult != 0)
        //                {
        //                    goto stop;
        //                }
        //            }
        //            else if (testCoopLevelRes == D3DERR_DEVICENOTRESET)
        //            {
        //                g_AnmManager->ReleaseSurfaces();
        //                testResetRes = g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
        //                if (testResetRes != 0)
        //                {
        //                    goto stop;
        //                }
        //                GameWindow::InitD3dDevice();
        //                g_Supervisor.unk198 = 3;
        //            }
        //        }
    }


stop:
    SDL_LOG_COMPAT("stop the game");
    g_Chain.Release();
    g_SoundPlayer.Release();

    delete g_AnmManager;
    g_AnmManager = NULL;

    // Clean up GL resources while the context is still valid.
    // THPrac::THPracGuiShutdown();
    // {
    //     SDL_GLContext ctx = g_Renderer ? g_Renderer->glContext : nullptr;
    //     if (g_Renderer)
    //         g_Renderer->Release();
    //     if (ctx)
    //         SDL_GL_DeleteContext(ctx);
    // }

    SDL_DESTROY_WINDOW_COMPAT(g_GameWindow.screen);
    SDL_GL_DELETE_CONTEXT_COMPAT(g_GameWindow.glContext);

    if (renderResult == 2)
    {
        // Clean up resources that leak across restart cycles.
        // We cannot call Supervisor::DeletedCallback() here because
        // ReleasePbg3() has a built-in double-free (calls Release() then
        // delete which calls Release() again) that crashes on modern heaps.
        // PBG3 archives are re-released internally by LoadPbg3() on reload,
        // so only these three resources actually leak:
        if (g_Supervisor.midiOutput != NULL)
        {
            g_Supervisor.midiOutput->StopPlayback();
            delete g_Supervisor.midiOutput;
            g_Supervisor.midiOutput = NULL;
        }
        TextHelper::ReleaseTextBuffer();
        // Controller::CloseSDLController();

        g_GameErrorContext.ResetContext();

        GameErrorContext::Log(&g_GameErrorContext, TH_ERR_OPTION_CHANGED_RESTART);

        if (!g_Supervisor.cfg.windowed)
        {
            SDL_SHOWCURSOR_COMPAT();
        }
        goto restart;
    }

    FileSystem::WriteDataToFile(TH_CONFIG_FILE, &g_Supervisor.cfg, sizeof(g_Supervisor.cfg));

    SDL_SHOWCURSOR_COMPAT();
    g_GameErrorContext.Flush();
    SDL_Quit();
    return 0;
}
