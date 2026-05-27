#include "GameWindow.hpp"
#include "AnmManager.hpp"
#include "GameErrorContext.hpp"
#include "ScreenEffect.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "ZunMath.hpp"

#ifdef WIN98
#include "graphics/FixedFunctionDX2.hpp"
#endif

#include "graphics/FixedFunctionGLWIN32.hpp"
#include "graphics/FixedFunctionGL.hpp"
#include "graphics/WebGL.hpp"
#include "graphics/Software.hpp"

#include "i18n.hpp"
#include "utils.hpp"

#include <cstring>
// #include <iostream>
// void gamewindowdlog(string msg){
//     cout<<"gamewindow : "<<msg<<endl;
// }

GameWindow g_GameWindow;
GfxInterface *g_GfxBackend;
i32 g_TickCountToEffectiveFramerate;
f64 g_LastFrameTime;

#define FRAME_TIME (1000. / 60.)

static const struct
{
    const char *name;
    GfxInterface *(*TryInit)();
} s_RenderBackends[] = {
    #ifdef RENDER_WEBGL
    {"GL(ES) 2.0 / WebGL", WebGL::Create},
    #endif
    #ifdef RENDER_FIXED_FUNCTION_DX2
    {"Fixed function DX2", FixedFunctionDX2::Init},
    #endif
    #ifdef RENDER_FIXED_FUNCTION_GL_WIN32
    {"Fixed function GL(ES) WIN32", FixedFunctionGLWIN32::Init},
    #endif
    #ifdef RENDER_FIXED_FUNCTION_GL
    {"Fixed function GL(ES)", FixedFunctionGL::Init},
    #endif
    #ifdef RENDER_SOFTWARE
    {"Software fallback (VERY SLOW)", Software::Init},
    #endif
};

RenderResult GameWindow::Render()
{
    LOG_COMPAT("Render 1");
    i32 res;
    f64 slowdown;
    ZunViewport viewport;
    f64 delta;
    Uint32 curtime;

    LOG_COMPAT("Render 2");
    if (this->lastActiveAppValue == 0)
    {
        return RENDER_RESULT_KEEP_RUNNING;
    }

    if (this->curFrame == 0)
    {
RUN_CHAINS:
        if (g_Supervisor.cfg.frameskipConfig <= this->curFrame)
        {
            if (g_Supervisor.RedrawWholeFrame())
            {
                viewport.x = 0;
                viewport.y = 0;
                viewport.width = GAME_WINDOW_WIDTH;
                viewport.height = GAME_WINDOW_HEIGHT;
                viewport.minZ = 0.0;
                viewport.maxZ = 1.0;
                viewport.Set();

                LOG_COMPAT("Render 3");
                g_GfxBackend->SetClearColor(
                    ((g_Stage.skyFog.color >> 16) & 0xFF) / 255.0f,
                    ((g_Stage.skyFog.color >> 8) & 0xFF) / 255.0f,
                    (g_Stage.skyFog.color & 0xFF) / 255.0f,
                    ((g_Stage.skyFog.color >> 24) & 0xFF) / 255.0f
                );

                LOG_COMPAT("Render 4");
                g_GfxBackend->Clear(CLEAR_COLOR_BUFFER | CLEAR_DEPTH_BUFFER);

                LOG_COMPAT("Render 5");
                g_AnmManager->SetProjectionMode(PROJECTION_MODE_PERSPECTIVE);
                LOG_COMPAT("Render 6");
                g_Supervisor.viewport.Set();
            }

            LOG_COMPAT("Render 7\n");
            g_AnmManager->ClearVertexBuffer();
            g_AnmManager->flushesThisFrame = 0;
            g_Chain.RunDrawChain();
            LOG_COMPAT("Render 7a\n");
            g_AnmManager->SetCurrentTexture(0);
            LOG_COMPAT("Render 7b\n");
        }

        g_AnmManager->FlushVertexBuffer();
        g_Supervisor.viewport.x = 0;
        g_Supervisor.viewport.y = 0;
        g_Supervisor.viewport.width = GAME_WINDOW_WIDTH;
        g_Supervisor.viewport.height = GAME_WINDOW_HEIGHT;

        LOG_COMPAT("Render 8");
        g_AnmManager->SetProjectionMode(PROJECTION_MODE_PERSPECTIVE);
        g_Supervisor.viewport.Set();

        LOG_COMPAT("Render 9");
        res = g_Chain.RunCalcChain();
        LOG_COMPAT("Render 10");
        g_SoundPlayer.PlaySounds();

        if (res == 0)
            return RENDER_RESULT_EXIT_SUCCESS;

        if (res == -1)
            return RENDER_RESULT_EXIT_ERROR;

        this->curFrame++;
    }

    //TODO check windowed 
    // if(true) 
    if (g_Supervisor.cfg.windowed || g_Supervisor.ShouldRunAt60Fps())
    {
        if (this->curFrame != 0)
        {
            g_Supervisor.framerateMultiplier = 1.0;

            Uint32 slowdownTicks = SDL_GetTicks();
            slowdown = (f64)slowdownTicks;

            if (slowdown < g_LastFrameTime)
                g_LastFrameTime = slowdown;

            delta = fabs(slowdown - g_LastFrameTime);

            if (delta >= FRAME_TIME)
            {
                do
                {
                    g_LastFrameTime += FRAME_TIME;
                    delta -= FRAME_TIME;
                }
                while (delta >= FRAME_TIME);

                if (g_Supervisor.cfg.frameskipConfig < this->curFrame)
                    goto SKIP_PRESENT;

                goto RUN_CHAINS;
            }
        }
    }
    else
    {
        if (g_Supervisor.cfg.frameskipConfig >= this->curFrame)
        {
            Present();
            goto RUN_CHAINS;
        }

SKIP_PRESENT:
        Present();

        if (g_Supervisor.framerateMultiplier == 0.f)
        {
            if (2 <= g_TickCountToEffectiveFramerate)
            {
                curtime = SDL_GetTicks();

                if (curtime < g_Supervisor.lastFrameTime)
                    g_Supervisor.lastFrameTime = curtime;

                delta = curtime - g_Supervisor.lastFrameTime;
                delta = (delta * 60.0) / 2.0 / 1000.0;
                delta /= (g_Supervisor.cfg.frameskipConfig + 1);

                if (delta >= .865)
                    delta = 1.0;
                else if (delta >= .6)
                    delta = 0.8;
                else
                    delta = 0.5;

                g_Supervisor.effectiveFramerateMultiplier = delta;
                g_Supervisor.lastFrameTime = curtime;
                g_TickCountToEffectiveFramerate = 0;
            }
        }
        else
        {
            g_Supervisor.effectiveFramerateMultiplier =
                g_Supervisor.framerateMultiplier;
        }

        this->curFrame = 0;
        g_TickCountToEffectiveFramerate++;
    }

    LOG_COMPAT("Render Finish");
    return RENDER_RESULT_KEEP_RUNNING;
}

void GameWindow::Present()
{
    // In D3D, this was done after the present call, but SDL makes no guarantees
    // about the color buffer state immediately after a swap, so it has to be moved to be before it
    LOG_COMPAT("present TakeScreenshotIfRequested");
    g_AnmManager->TakeScreenshotIfRequested();
    if (g_Supervisor.unk198 != 0)
    {
        g_Supervisor.unk198--;
    }

    LOG_COMPAT("present g_GfxBackend->SwapBuffers");
    // SDL_GL_SWAP_COMPAT(g_GameWindow.screen);
    g_GfxBackend->SwapBuffers();

    LOG_COMPAT("present finish");
    return;
}

void GameWindow::CreateGameWindow()
{
    LOG_COMPAT("GameWindow::CreateGameWindow 1\n");
    SDL_Init(SDL_INIT_GAMECONTROLLER_COMPAT);

    // SDL1.2: try render backends (mostly just GL variants)
    for (u32 i = 0; i < ARRAY_SIZE(s_RenderBackends); i++)
    {
        LOG_COMPAT("Try Using renderer backend %s\n", s_RenderBackends[i].name);
        g_GfxBackend = s_RenderBackends[i].TryInit();
        if(g_GfxBackend) {
            LOG_COMPAT("Using renderer backend %s\n", s_RenderBackends[i].name);
            break;
        }else{
            LOG_COMPAT("Failed renderer backend %s\n", s_RenderBackends[i].name);
        }
    }

    g_GameWindow.lastActiveAppValue = 1;
}

i32 GameWindow::InitD3dRendering(void)
{
    LOG_COMPAT("InitD3dRendering 1\n");
    ZunVec3 eye;
    ZunVec3 at;
    ZunVec3 up;
    f32 half_width;
    f32 half_height;
    f32 aspect_ratio;
    f32 field_of_view_y;
    f32 camera_distance;

    LOG_COMPAT("InitD3dRendering 2\n");
    // OpenGL considers textures to be incomplete if the bound texture has no image defined
    // Incomplete textures result in texturing being turned off, but EoSD has places where it
    // uses the texturing engine to color fragments without using the texture itself. The dummy
    // texture is necessary to ensure the texture can't be considered incomplete in these cases.
    g_AnmManager->CreateTextureObject();
    g_AnmManager->dummyTextureHandle = g_AnmManager->currentTextureHandle;
    g_GfxBackend->SetTextureImage(1, 1, PIXEL_RGBA, PIXEL_UNSIGNED_BYTE, NULL);

    // g_AnmManager->gfxBackend = s_RenderBackends[g_GameWindow.renderBackendIndex].init();

    //    using_d3d_hal = 1;
    //    memset(&present_params, 0, sizeof(D3DPRESENT_PARAMETERS));
    //    g_Supervisor.d3dIface->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &display_mode);
    //TODO check windowed 2
    if (!g_Supervisor.cfg.windowed)
    {
        if ((((g_Supervisor.cfg.opts >> GCOS_FORCE_16BIT_COLOR_MODE) & 1) == 1))
        {
            //            present_params.BackBufferFormat = D3DFMT_R5G6B5;
            g_Supervisor.cfg.colorMode16bit = 1;
        }
        else if (g_Supervisor.cfg.colorMode16bit == 0xff)
        {
            //            if ((display_mode.Format == D3DFMT_X8R8G8B8) || (display_mode.Format == D3DFMT_A8R8G8B8))
            //            {
            //                present_params.BackBufferFormat = D3DFMT_X8R8G8B8;
            g_Supervisor.cfg.colorMode16bit = 0;
            LOG_COMPAT("InitD3dRendering 3\n");
            GameErrorContext::Log(&g_GameErrorContext, TH_ERR_SCREEN_INIT_32BITS);
            //            }
            //            else
            //            {
            //                present_params.BackBufferFormat = D3DFMT_R5G6B5;
            //                g_Supervisor.cfg.colorMode16bit = 1;
            //                GameErrorContext::Log(&g_GameErrorContext, TH_ERR_SCREEN_INIT_16BITS);
            //            }
        }
        //        else if (g_Supervisor.cfg.colorMode16bit == 0)
        //        {
        //            present_params.BackBufferFormat = D3DFMT_X8R8G8B8;
        //        }
        //        else
        //        {
        //            present_params.BackBufferFormat = D3DFMT_R5G6B5;
        //        }
        if (!((g_Supervisor.cfg.opts >> GCOS_FORCE_60FPS) & 1))
        {

            //            present_params.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        }
        else
        {
            //            present_params.FullScreen_RefreshRateInHz = 60;
            //            present_params.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
            //            GameErrorContext::Log(&g_GameErrorContext, TH_ERR_SET_REFRESH_RATE_60HZ);
        }

        LOG_COMPAT("InitD3dRendering 4\n");
        // SDL_GL_SET_SWAP_INTERVAL_COMPAT(1);

        //        if (g_Supervisor.cfg.frameskipConfig == 0)
        //        {
        //            present_params.SwapEffect = D3DSWAPEFFECT_FLIP;
        //        }
        //        else
        //        {
        //            present_params.SwapEffect = D3DSWAPEFFECT_COPY_VSYNC;
        //        }
    }
    //    else
    //    {
    //        present_params.BackBufferFormat = display_mode.Format;
    //        present_params.SwapEffect = D3DSWAPEFFECT_COPY;
    //        present_params.Windowed = 1;
    //    }
    //    present_params.BackBufferWidth = GAME_WINDOW_WIDTH;
    //    present_params.BackBufferHeight = GAME_WINDOW_HEIGHT;
    //    present_params.EnableAutoDepthStencil = true;
    //    present_params.AutoDepthStencilFormat = D3DFMT_D16;
    //    present_params.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

    // SDL_GL_SET_SWAP_INTERVAL_COMPAT(1);
    g_Supervisor.vsyncEnabled = 1;

    g_Supervisor.lockableBackbuffer = 1;
    //    memcpy(&g_Supervisor.presentParameters, &present_params, sizeof(D3DPRESENT_PARAMETERS));
    //    for (;;)
    //    {
    //        if (((g_Supervisor.cfg.opts >> GCOS_REFERENCE_RASTERIZER_MODE) & 1) != 0)
    //        {
    //            goto REFERENCE_RASTERIZER_MODE;
    //        }
    //        else
    //        {
    //            if (g_Supervisor.d3dIface->CreateDevice(0, D3DDEVTYPE_HAL, g_GameWindow.screen,
    //                                                    D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_params,
    //                                                    &g_Supervisor.d3dDevice) < 0)
    //            {
    //                GameErrorContext::Log(&g_GameErrorContext, TH_ERR_TL_HAL_UNAVAILABLE);
    //                if (g_Supervisor.d3dIface->CreateDevice(0, D3DDEVTYPE_HAL, g_GameWindow.screen,
    //                                                        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_params,
    //                                                        &g_Supervisor.d3dDevice) < 0)
    //                {
    //                    GameErrorContext::Log(&g_GameErrorContext, TH_ERR_HAL_UNAVAILABLE);
    //                REFERENCE_RASTERIZER_MODE:
    //                    if (g_Supervisor.d3dIface->CreateDevice(0, D3DDEVTYPE_REF, g_GameWindow.screen,
    //                                                            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_params,
    //                                                            &g_Supervisor.d3dDevice) < 0)
    //                    {
    //                        if (((g_Supervisor.cfg.opts >> GCOS_FORCE_60FPS) & 1) != 0 && !g_Supervisor.vsyncEnabled)
    //                        {
    //                            GameErrorContext::Log(&g_GameErrorContext,
    //                            TH_ERR_CANT_CHANGE_REFRESH_RATE_FORCE_VSYNC);
    //                            present_params.FullScreen_RefreshRateInHz = 0;
    //                            g_Supervisor.vsyncEnabled = 1;
    //                            present_params.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    //                            continue;
    //                        }
    //                        else
    //                        {
    //                            if (present_params.Flags == D3DPRESENTFLAG_LOCKABLE_BACKBUFFER)
    //                            {
    //                                GameErrorContext::Log(&g_GameErrorContext, TH_ERR_BACKBUFFER_NONLOCKED);
    //                                present_params.Flags = 0;
    //                                g_Supervisor.lockableBackbuffer = 0;
    //                                continue;
    //                            }
    //                            else
    //                            {
    //                                GameErrorContext::Fatal(&g_GameErrorContext, TH_ERR_D3D_INIT_FAILED);
    //                                if (g_Supervisor.d3dIface != NULL)
    //                                {
    //                                    g_Supervisor.d3dIface->Release();
    //                                    g_Supervisor.d3dIface = NULL;
    //                                }
    //                                return 1;
    //                            }
    //                        }
    //                    }
    //                    else
    //                    {
    //                        GameErrorContext::Log(&g_GameErrorContext, TH_USING_REF_MODE);
    //                        g_Supervisor.hasD3dHardwareVertexProcessing = 0;
    //                        using_d3d_hal = 0;
    //                    }
    //                }
    //                else
    //                {
    //                    GameErrorContext::Log(&g_GameErrorContext, TH_USING_HAL_MODE);
    //                    g_Supervisor.hasD3dHardwareVertexProcessing = 0;
    //                }
    //            }
    //            else
    //            {
    //                GameErrorContext::Log(&g_GameErrorContext, TH_USING_TL_HAL_MODE);
    //                g_Supervisor.hasD3dHardwareVertexProcessing = 1;
    //            }
    //            break;
    //        }
    //    }

    // Camera set up so that at z = 0.0, world coordinates map exactly to (quadrant 4) window coordinates

    LOG_COMPAT("InitD3dRendering 5\n");
    half_width = (float)GAME_WINDOW_WIDTH / 2.0;
    half_height = (float)GAME_WINDOW_HEIGHT / 2.0;
    aspect_ratio = (float)GAME_WINDOW_WIDTH / (float)GAME_WINDOW_HEIGHT;
    field_of_view_y = 0.52359879; // PI / 6.0f
    camera_distance = half_height / ZUN_TANF(field_of_view_y / 2.0f);
    up.x = 0.0;
    up.y = 1.0;
    up.z = 0.0;
    at.x = half_width;
    at.y = -half_height;
    at.z = 0.0;
    eye.x = half_width;
    eye.y = -half_height;
    eye.z = -camera_distance;
    //    D3DXMatrixLookAtLH(&g_Supervisor.viewMatrix, &eye, &at, &up);

    LOG_COMPAT("InitD3dRendering 6");
    ZunMatrix viewMatrix = createViewMatrix(eye, at, up);
    g_AnmManager->SetTransformMatrix(MATRIX_VIEW, viewMatrix);
    g_Supervisor.viewMatrix = viewMatrix;

    ZunMatrix perspectiveMatrix = perspectiveMatrixFromFOV(field_of_view_y, aspect_ratio, 100.0f, 10000.0f);
    g_AnmManager->SetTransformMatrix(MATRIX_PROJECTION, perspectiveMatrix);
    g_Supervisor.projectionMatrix = perspectiveMatrix;

    //    D3DXMatrixPerspectiveFovLH(&g_Supervisor.projectionMatrix, field_of_view_y, aspect_ratio, 100.0, 10000.0);
    //    g_Supervisor.d3dDevice->SetTransform(D3DTS_VIEW, &g_Supervisor.viewMatrix);
    //    g_Supervisor.d3dDevice->SetTransform(D3DTS_PROJECTION, &g_Supervisor.projectionMatrix);
    g_Supervisor.viewport.Get();

    //    g_Supervisor.d3dDevice->GetDeviceCaps(&g_Supervisor.d3dCaps);
    //    if (((((g_Supervisor.cfg.opts >> GCOS_USE_D3D_HW_TEXTURE_BLENDING) & 1) == 0) &&
    //         ((g_Supervisor.d3dCaps.TextureOpCaps & D3DTEXOPCAPS_ADD) == 0)))
    //    {
    //        GameErrorContext::Log(&g_GameErrorContext, TH_ERR_NO_SUPPORT_FOR_D3DTEXOPCAPS_ADD);
    //        g_Supervisor.cfg.opts = g_Supervisor.cfg.opts | (1 << GCOS_USE_D3D_HW_TEXTURE_BLENDING);
    //    }
    //    if (g_Supervisor.ShouldRunAt60Fps() &&
    //        ((g_Supervisor.d3dCaps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE) == 0))
    //    {
    //        GameErrorContext::Log(&g_GameErrorContext, TH_ERR_CANT_FORCE_60FPS_NO_ASYNC_FLIP);
    //        g_Supervisor.cfg.opts = g_Supervisor.cfg.opts & ~(1 << GCOS_FORCE_60FPS);
    //    }
    //    if ((((g_Supervisor.cfg.opts >> GCOS_FORCE_16BIT_COLOR_MODE) & 1) == 0) && (using_d3d_hal != 0))
    //    {
    //        if (g_Supervisor.d3dIface->CheckDeviceFormat(0, D3DDEVTYPE_HAL, present_params.BackBufferFormat, 0,
    //                                                     D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8) == 0)
    //        {
    //            g_Supervisor.colorMode16Bits = 1;
    //        }
    //        else
    //        {
    //            g_Supervisor.colorMode16Bits = 0;
    //            g_Supervisor.cfg.opts = g_Supervisor.cfg.opts | (1 << GCOS_FORCE_16BIT_COLOR_MODE);
    //            GameErrorContext::Log(&g_GameErrorContext, TH_ERR_D3DFMT_A8R8G8B8_UNSUPPORTED);
    //        }
    //    }
    LOG_COMPAT("InitD3dRendering 7\n");
    InitD3dDevice();
    ScreenEffect::SetViewport(0);
    g_GameWindow.isAppClosing = 0;
    g_Supervisor.lastFrameTime = 0;
    g_Supervisor.framerateMultiplier = 0.0;
    LOG_COMPAT("InitD3dRendering success");
    return 0;
}

void GameWindow::InitD3dDevice(void)
{
    AnmManager *anm1;
    AnmManager *anm2;
    AnmManager *anm3;
    AnmManager *anm4;

    g_GfxBackend->Enable(CAPS_BLEND);

    g_GfxBackend->SetBlendMode(BLEND_INV_SRC_ALPHA);
    
    if (((g_Supervisor.cfg.opts >> GCOS_TURN_OFF_DEPTH_TEST) & 1) == 0)
    {
        g_GfxBackend->Enable(CAPS_DEPTH_TEST);
        g_AnmManager->SetDepthMask(true);
        g_AnmManager->SetDepthFunc(DEPTH_FUNC_LEQUAL);
    }

    g_AnmManager->SetFogColor(0xFFA0A0A0);
    g_AnmManager->SetFogRange(1000.0f, 5000.0f);

    //    g_AnmManager->gfxBackend->Init();

    // All of these are set per texture object in OpenGL (and also most are defaults)
    //    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
    //    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    //    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    //    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
    //    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP);
    //    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    //    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    if (g_AnmManager != NULL)
    {
        anm1 = g_AnmManager;
        anm1->currentBlendMode = 0xff;
        anm4 = g_AnmManager;
        anm4->currentTextureHandle = 0;
    }
    g_Stage.skyFogNeedsSetup = 1;
    return;
}
