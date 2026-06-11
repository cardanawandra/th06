#include <stdio.h>

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
    LOG_COMPAT("Starting");
    i32 renderResult = 0;


    LOG_COMPAT("Init Gamepath");
    GamePaths::Init();

    // if (utils::CheckForRunningGameInstance())
    // {
    //     g_GameErrorContext.Flush();

    //     return 1;
    // }

    LOG_COMPAT("Load CONF File");
    if (g_Supervisor.LoadConfig(TH_CONFIG_FILE) != ZUN_SUCCESS)
    {
#ifdef __ANDROID__
        // On Android, config file may not exist on first run.
        // LoadConfig sets defaults and tries to write — if write fails,
        // continue anyway with defaults.
        LOG_COMPAT("LoadConfig failed (first run?), continuing with defaults");
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
    LOG_COMPAT("Start the game");

restart:
    LOG_COMPAT("Create game window");
    GameWindow::CreateGameWindow();

    LOG_COMPAT("new AnmManager");
    g_AnmManager = new AnmManager();

    LOG_COMPAT("InitD3dRendering");
    if (GameWindow::InitD3dRendering() != ZUN_SUCCESS)
    {
        g_GameErrorContext.Flush();
        return 1;
    }

    LOG_COMPAT("InitializeDSound");
    g_SoundPlayer.InitializeDSound();
    LOG_COMPAT("GetJoystickCaps");
    Controller::GetJoystickCaps();
    LOG_COMPAT("ResetKeyboard");
    Controller::ResetKeyboard();

    LOG_COMPAT("Supervisor::RegisterChain");
    if (Supervisor::RegisterChain() != ZUN_SUCCESS)
    {
        goto stop;
    }
    if (!g_Supervisor.cfg.windowed)
    {
        HIDECURSOR_COMPAT();
    }

    g_GameWindow.curFrame = 0;

    LOG_COMPAT("Into loop game event");
    while (true)
    {
        if(!g_GfxBackend->GameLoop()){
            goto stop;
        }
        renderResult = g_GameWindow.Render();
        if (renderResult != 0)
        {
            break;
        }
    }


stop:
    LOG_COMPAT("stop the game");
    g_Chain.Release();
    g_SoundPlayer.Release();

    delete g_AnmManager;
    g_AnmManager = NULL;

    if(g_GfxBackend != NULL) delete g_GfxBackend;

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
            SHOWCURSOR_COMPAT();
        }
        goto restart;
    }

    FileSystem::WriteDataToFile(TH_CONFIG_FILE, &g_Supervisor.cfg, sizeof(g_Supervisor.cfg));

    SHOWCURSOR_COMPAT();
    g_GameErrorContext.Flush();
    COMPAT_Quit();
    return 0;
}
