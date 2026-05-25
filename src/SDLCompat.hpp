#pragma once
#include <math.h>
#include <algorithm>
#include <SDL.h>
#include "inttypes.hpp"
#include "ZunColor.hpp"

#ifdef _MSC_VER
    #define SNPRINTF _snprintf
#else
    #define SNPRINTF snprintf
#endif

#if SDL_MAJOR_VERSION >= 3
    // SDL 3.x specific code
    #include <SDL_gamepad.h>
    #define SDL_JOYSTICK_COMPAT SDL_Gamepad
    #define SDL_JOYSTICK_COMPATButton SDL_GamepadButton
    #define SDL_JOYSTICK_COMPATGetJoystick(a) SDL_GetGamepadJoystick(a)
    #define SDL_JOYSTICK_COMPATHasAxis(a,b) SDL_GamepadHasAxis(a,b)
    #define SDL_JOYSTICK_COMPATGetAxis(a,b) SDL_GetGamepadAxis(a,b)
    #define SDL_JOYSTICK_COMPATGetButton(a,b) SDL_GetGamepadButton(a,b)
    #define SDL_JOYSTICK_COMPATOpen(a) SDL_OpenGamepad(a)
    #define SDL_JOYSTICK_COMPATClose(a) SDL_CloseGamepad(a)
    
    #define SDL_CONTROLLER_BUTTON_MAX_COMPAT SDL_GAMEPAD_BUTTON_COUNT

    #define SDL_SHOWCURSOR_COMPAT() SDL_ShowCursor()
    #define SDL_HIDECURSOR_COMPAT() SDL_HideCursor()

    #define SDL_CreateWindowCompat(title, x, y, width, height, flags) SDL_CreateWindow(title, width, height, flags)
    #define SDL_CreateRendererCompat(a,b,c,d) SDL_CreateRenderer(a,d)
    #define SDL_SetHintCompat(a,b) SDL_SetHint(#a,b)

    #define SDL_GL_MAKE_CURRENT_COMPAT_SUCCESS true

    inline void GetWindowSize(int *w, int *h)
    {
        const SDL_DisplayMode* mode =
            SDL_GetCurrentDisplayMode(0);

        if (mode)
        {
            *w = mode->w;
            *h = mode->h;
        }
    }

    #define SDL_RWOPS_COMPAT               SDL_IOStream

    #define SDL_RWFROMFILE_COMPAT          SDL_IOFromFile
    #define SDL_RWFROMCONSTMEM_COMPAT SDL_IOFromConstMem

    #define SDL_RWCLOSE_COMPAT             SDL_CloseIO

    static inline size_t SDL_RWREAD_COMPAT(
        SDL_IOStream *context,
        void *ptr,
        size_t size,
        size_t maxnum)
    {
        size_t bytesRequested = size * maxnum;
        size_t bytesRead =
            SDL_ReadIO(context, ptr, bytesRequested);

        return bytesRead / size;
    }

    #define SDL_RWSEEK_COMPAT              SDL_SeekIO
    #define SDL_RWTELL_COMPAT              SDL_TellIO

    // #define SDL_START_TEXT_INPUT_COMPAT(a) SDL_StartTextInput(a)
    // #define SDL_STOP_TEXT_INPUT_COMPAT(a) SDL_StopTextInput(a)
    #define SDL_START_TEXT_INPUT_COMPAT()
    #define SDL_STOP_TEXT_INPUT_COMPAT()

    #define SDL_PIXEL_FORMAT_COMPAT SDL_PixelFormat
    #define SDL_CREATE_RGB_SURFACE_FROM_COMPAT(a,b,c,d,e,f) SDL_CreateSurfaceFrom(b,c,f,a,e)
    #define SDL_CREATE_RGB_SURFACE_COMPAT(a,b,c) SDL_CreateSurface(a,b,c)
    #define SDL_CONVERT_SURFACE_FORMAT_COMPAT(a,b,c) SDL_ConvertSurface(a,b)

    inline int SDL_NumJoysticks() {
        int count;
        SDL_GetJoysticks(&count);
        return count;
    }
    #define IMG_Load_RW IMG_Load_IO
    #define SDL_SoftStretch(a,b,c,d) SDL_BlitSurfaceScaled(a,b,c,d,SDL_SCALEMODE_LINEAR)

    #define TTF_RenderUTF8_Blended(a,b,c) TTF_RenderText_Blended(a,b,strlen(b),c)
    
    inline u16 SDL_READLE16_COMPAT(SDL_IOStream *src){
        u16 value;
        SDL_ReadU16LE(src, &value);
        return value;
    }
    inline u32 SDL_READLE32_COMPAT(SDL_IOStream *src){
        u32 value;
        SDL_ReadU32LE(src, &value);
        return value;
    }
    #define SDL_AUDIO_STREAM_COMPAT SDL_AudioStream*
    #define SDL_CREATE_AUDIO_STREAM_COMPAT SDL_CreateAudioStream
    #define SDL_QUEUE_AUDIO_COMPAT(a,b,c,d) SDL_PutAudioStreamData(b,c,d)
    #define SDL_OPEN_AUDIO_COMPAT(a,b) SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,a)
    #define SDL_BIND_AUDIO_STREAM_COMPAT SDL_BindAudioStream

    //Replacement
    typedef struct SDL_AudioCVT
    {
        int needed;
        SDL_AudioFormat src_format;
        SDL_AudioFormat dst_format;
        double rate_incr;
        Uint8 *buf;
        int len;
        int len_cvt;
        int len_mult;
        double len_ratio;
        int filter_index;
    } SDL_AudioCVT;
    #define SDL_RESUME_AUDIO_COMPAT(a,b) SDL_ResumeAudioDevice(a);SDL_ResumeAudioStreamDevice(b) 
    #define SDL_PAUSE_AUDIO_COMPAT(b) SDL_PauseAudioDevice(b)
    #define SDL_DESTROY_AUDIO_STREAM SDL_DestroyAudioStream
#else
    #include <SDL_rwops.h>
    #define SDL_RWOPS_COMPAT SDL_RWops
    #define SDL_RWFROMFILE_COMPAT SDL_RWFromFile
    #define SDL_RWFROMCONSTMEM_COMPAT SDL_RWFromConstMem
    #define SDL_RWCLOSE_COMPAT SDL_RWclose
    #define SDL_RWREAD_COMPAT SDL_RWread
    #define SDL_RWSEEK_COMPAT SDL_RWseek
    #define SDL_RWTELL_COMPAT SDL_RWtell
    #define SDL_GL_MAKE_CURRENT_COMPAT_SUCCESS 0

    #define SDL_READLE32_COMPAT SDL_ReadLE32
    #define SDL_READLE16_COMPAT SDL_ReadLE16

    #define SDL_SHOWCURSOR_COMPAT() SDL_ShowCursor(SDL_ENABLE)
    #define SDL_HIDECURSOR_COMPAT() SDL_ShowCursor(SDL_DISABLE)

    #define SDL_CONTROLLER_BUTTON_A                0
    #define SDL_CONTROLLER_BUTTON_B                1
    #define SDL_CONTROLLER_BUTTON_X                2
    #define SDL_CONTROLLER_BUTTON_Y                3
    #define SDL_CONTROLLER_BUTTON_LEFTSHOULDER     4
    #define SDL_CONTROLLER_BUTTON_RIGHTSHOULDER    5
    #define SDL_CONTROLLER_BUTTON_BACK             6
    #define SDL_CONTROLLER_BUTTON_START            7
    #define SDL_CONTROLLER_BUTTON_LEFTSTICK        8
    #define SDL_CONTROLLER_BUTTON_RIGHTSTICK       9

    #define SDL_CONTROLLER_BUTTON_DPAD_UP          10
    #define SDL_CONTROLLER_BUTTON_DPAD_DOWN        11
    #define SDL_CONTROLLER_BUTTON_DPAD_LEFT        12
    #define SDL_CONTROLLER_BUTTON_DPAD_RIGHT       13

    #define SDL_AUDIO_STREAM_COMPAT bool
    #define SDL_CREATE_AUDIO_STREAM_COMPAT(a,b) true
    #define SDL_BIND_AUDIO_STREAM_COMPAT(a,b) true
    #define SDL_DESTROY_AUDIO_STREAM(a)
#endif

#if SDL_MAJOR_VERSION == 2
    // SDL 2.x only specific code
    #include <SDL_gamecontroller.h>
    #define SDL_JOYSTICK_COMPAT SDL_GameController
    #define SDL_JOYSTICK_COMPATButton SDL_GameControllerButton
    #define SDL_JOYSTICK_COMPATGetJoystick(a) SDL_GameControllerGetJoystick(a)
    #define SDL_JOYSTICK_COMPATHasAxis(a,b) SDL_GameControllerHasAxis(a,b)
    #define SDL_JOYSTICK_COMPATGetAxis(a,b) SDL_GameControllerGetAxis(a,b)
    #define SDL_JOYSTICK_COMPATGetButton(a,b) SDL_GameControllerGetButton(a,b)
    #define SDL_JOYSTICK_COMPATOpen(a) SDL_GameControllerOpen(a)
    #define SDL_JOYSTICK_COMPATClose(a) SDL_GameControllerClose(a)    

    #define SDL_CONTROLLER_BUTTON_MAX_COMPAT SDL_CONTROLLER_BUTTON_MAX
    #define SDL_CreateWindowCompat(title, x, y, width, height, flags) SDL_CreateWindow(title, x, y, width, height, flags)
    #define SDL_CreateRendererCompat(a,b,c,d) SDL_CreateRenderer(a,b,c)
    #define SDL_SetHintCompat(a,b) SDL_SetHint(a,b)

    #define SDL_START_TEXT_INPUT_COMPAT() SDL_StartTextInput()
    #define SDL_STOP_TEXT_INPUT_COMPAT() SDL_StopTextInput()
    #define SDL_PIXEL_FORMAT_COMPAT SDL_PixelFormatEnum
    #define SDL_CREATE_RGB_SURFACE_COMPAT(a,b,c) SDL_CreateRGBSurfaceWithFormat(0,a,b,32,c)
    #define SDL_CREATE_RGB_SURFACE_FROM_COMPAT(a,b,c,d,e,f) SDL_CreateRGBSurfaceWithFormatFrom(a,b,c,d,e,f)
    #define SDL_CONVERT_SURFACE_FORMAT_COMPAT SDL_ConvertSurfaceFormat

    inline void GetWindowSize(int *w, int *h){
        SDL_DisplayMode mode;
        if (SDL_GetCurrentDisplayMode(0, &mode) == 0) {
            *w = mode.w;
            *h = mode.h;
        }
    }
    #define SDL_OPEN_AUDIO_COMPAT(a,b) SDL_OpenAudioDevice(NULL, 0,a,b,0)
    #define SDL_QUEUE_AUDIO_COMPAT(a,b,c,d) SDL_QueueAudio(a,c,d)
    #define SDL_RESUME_AUDIO_COMPAT(a,b) SDL_PauseAudioDevice(a,0)
    #define SDL_PAUSE_AUDIO_COMPAT(b) SDL_PauseAudioDevice(b,1)
#endif

#if SDL_MAJOR_VERSION >= 2
    // SDL 2.x specific code
    #define SDL_LOG_COMPAT SDL_Log
    #define TRY_RESOLVE_FUNCTION(func) this->func = (decltype(this->func))SDL_GL_GetProcAddress(#func);
    #define TRY_RESOLVE_FUNCTION_GLES(func) this->func##_ptr = (decltype(this->func##_ptr))SDL_GL_GetProcAddress(#func);


    #define SDL_SURFACE_COMPAT SDL_Window
    #define SDL_GL_CONTEXT_COMPAT SDL_GLContext
    #define SDL_WINDOWPOS_UNDEFINED_COMPAT SDL_WINDOWPOS_UNDEFINED
    #define SDL_GL_SWAP_COMPAT(x) SDL_GL_SwapWindow(x)
    #define SDL_GL_SET_SWAP_INTERVAL_COMPAT(x) SDL_GL_SetSwapInterval(x)

    #define SDL_INIT_GAMECONTROLLER_COMPAT SDL_INIT_GAMECONTROLLER
    #define SDL_GET_KEYSTATE_COMPAT() SDL_GetKeyboardState(NULL)
    #define SDL_PUMP_EVENTS_COMPAT()

    #define INT16_MAX_COMPAT INT16_MAX

    #define KEY_UP             SDL_SCANCODE_UP
    #define KEY_DOWN           SDL_SCANCODE_DOWN
    #define KEY_LEFT           SDL_SCANCODE_LEFT
    #define KEY_RIGHT          SDL_SCANCODE_RIGHT

    #define KEY_KP8            SDL_SCANCODE_KP_8
    #define KEY_KP2            SDL_SCANCODE_KP_2
    #define KEY_KP4            SDL_SCANCODE_KP_4
    #define KEY_KP6            SDL_SCANCODE_KP_6

    #define KEY_KP7            SDL_SCANCODE_KP_7
    #define KEY_KP9            SDL_SCANCODE_KP_9
    #define KEY_KP1            SDL_SCANCODE_KP_1
    #define KEY_KP3            SDL_SCANCODE_KP_3

    #define KEY_HOME           SDL_SCANCODE_HOME

    #define KEY_Z              SDL_SCANCODE_Z
    #define KEY_X              SDL_SCANCODE_X

    #define KEY_LSHIFT         SDL_SCANCODE_LSHIFT
    #define KEY_RSHIFT         SDL_SCANCODE_RSHIFT

    #define KEY_ESCAPE         SDL_SCANCODE_ESCAPE

    #define KEY_LCTRL          SDL_SCANCODE_LCTRL
    #define KEY_RCTRL          SDL_SCANCODE_RCTRL

    #define KEY_Q              SDL_SCANCODE_Q
    #define KEY_S              SDL_SCANCODE_S
    #define KEY_RETURN         SDL_SCANCODE_RETURN
    #define WINDOW_FLAGS_COMPAT SDL_WINDOW_OPENGL


    #define SDL_GL_MAKE_CURRENT_COMPAT(a,b) SDL_GL_MakeCurrent(a,b)
    #define SDL_GL_CREATE_CONTEXT_COMPAT(screen) SDL_GL_CreateContext(screen)
    #define SDL_DESTROY_WINDOW_COMPAT(a) SDL_DestroyWindow(a)
    #define SDL_GL_DELETE_CONTEXT_COMPAT(a) SDL_GL_DeleteContext(a)
    #define SDL_FULLSCREEN_COMPAT SDL_WINDOW_FULLSCREEN
    #define SDL_CREATE_THREAD_COMPAT(a,b,c) SDL_CreateThread(a,b,c)

    #define SDL_PIXEL_FORMAT_COMPAT_LOAD()

    #define SDL_RW_SIZE_COMPAT SDL_RWsize

    inline SDL_Color SDL_TEXT_COLOR_COMPAT(ZunColor shadowColor){
        SDL_Color sdlShadowColor;
        sdlShadowColor.a = 0xFF;
        sdlShadowColor.b = (shadowColor >> 16) & 0xFF;
        sdlShadowColor.g = (shadowColor >> 8) & 0xFF;
        sdlShadowColor.r = shadowColor & 0xFF;
        return sdlShadowColor;
    }
    #define SDL_AUDIO_DEVICE_ID_COMPAT SDL_AudioDeviceID
    #define SDL_OPEN_AUDIO_COMPAT_ERROR 0
    #define SDL_CLOSE_AUDIO_COMPAT(c) SDL_CloseAudioDevice(c)

#endif

#if SDL_MAJOR_VERSION == 1
    // SDL 1.x specific code
    #include <SDL_joystick.h>
    #define SDL_INIT_GAMECONTROLLER_COMPAT SDL_INIT_JOYSTICK
    #define SDL_LOG_COMPAT printf
    #ifdef _WIN32
        #define TRY_RESOLVE_FUNCTION(name) this->name = ::name;
    #else
        #define TRY_RESOLVE_FUNCTION(name) *(void**)&this->name = SDL_GL_GetProcAddress(#name);
    #endif
    #define TRY_RESOLVE_FUNCTION_GLES(bla)

    #define SDL_SURFACE_COMPAT SDL_Surface
    #define SDL_GL_CONTEXT_COMPAT SDL_Surface*
    #define SDL_WINDOWPOS_UNDEFINED_COMPAT 0
    #define SDL_GL_SWAP_COMPAT(x) SDL_GL_SwapBuffers()
    #define SDL_GL_SET_SWAP_INTERVAL_COMPAT(x) SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, x)

    #define SDL_JOYSTICK_COMPAT SDL_Joystick
    #define SDL_JOYSTICK_COMPATButton i16
    #define SDL_JOYSTICK_COMPATGetJoystick(a) a
    #define SDL_CONTROLLER_AXIS_LEFTX 0
    #define SDL_CONTROLLER_AXIS_LEFTY 0
    #define SDL_CONTROLLER_AXIS_RIGHTX 0
    #define SDL_CONTROLLER_AXIS_RIGHTY 0
    #define SDL_JOYSTICK_COMPATHasAxis(a,b) 1
    #define SDL_JOYSTICK_COMPATGetAxis(a,b) SDL_JoystickGetAxis(a,b)
    #define SDL_JOYSTICK_COMPATGetButton(a,b) SDL_JoystickGetButton(a,b)
    #define SDL_JOYSTICK_COMPATOpen(a) SDL_JoystickOpen(a)
    #define SDL_JOYSTICK_COMPATClose(a) SDL_JoystickClose(a)    

    #define SDL_CONTROLLER_BUTTON_MAX_COMPAT 32
    #define SDL_GET_KEYSTATE_COMPAT() SDL_GetKeyState(NULL)
    #define SDL_PUMP_EVENTS_COMPAT() SDL_PumpEvents()

    #define SDL_START_TEXT_INPUT_COMPAT()
    #define SDL_STOP_TEXT_INPUT_COMPAT()

    #define INT16_MAX_COMPAT 32767

    #define KEY_UP             SDLK_UP
    #define KEY_DOWN           SDLK_DOWN
    #define KEY_LEFT           SDLK_LEFT
    #define KEY_RIGHT          SDLK_RIGHT

    #define KEY_KP8            SDLK_KP8
    #define KEY_KP2            SDLK_KP2
    #define KEY_KP4            SDLK_KP4
    #define KEY_KP6            SDLK_KP6

    #define KEY_KP7            SDLK_KP7
    #define KEY_KP9            SDLK_KP9
    #define KEY_KP1            SDLK_KP1
    #define KEY_KP3            SDLK_KP3

    #define KEY_HOME           SDLK_HOME

    #define KEY_Z              SDLK_z
    #define KEY_X              SDLK_x

    #define KEY_LSHIFT         SDLK_LSHIFT
    #define KEY_RSHIFT         SDLK_RSHIFT

    #define KEY_ESCAPE         SDLK_ESCAPE

    #define KEY_LCTRL          SDLK_LCTRL
    #define KEY_RCTRL          SDLK_RCTRL

    #define KEY_Q              SDLK_q
    #define KEY_S              SDLK_s
    #define KEY_RETURN         SDLK_RETURN
    #define WINDOW_FLAGS_COMPAT SDL_OPENGL | SDL_DOUBLEBUF
    #define TH_WINDOWPOS_UNDEFINED 0
    #define SDL_GL_CREATE_CONTEXT_COMPAT(a) a
    #define SDL_GL_MAKE_CURRENT_COMPAT(a,b) 0
    #define SDL_DESTROY_WINDOW_COMPAT(a) 1
    #define SDL_GL_DELETE_CONTEXT_COMPAT(a) 1
    #define SDL_FULLSCREEN_COMPAT SDL_FULLSCREEN
    #define SDL_CREATE_THREAD_COMPAT(a,b,c) SDL_CreateThread(a,c)
    #define SDL_CreateWindowCompat(title, x, y, width, height, flags) SDL_SetVideoMode(width, height, 32, flags); SDL_WM_SetCaption(title, "hello_icon")
    #define SDL_CONVERT_SURFACE_FORMAT_COMPAT(a,b,c) SDL_ConvertSurface(a,&b,c)
    #define SDL_CREATE_RGB_SURFACE_COMPAT(a,b,c) SDL_CreateRGBSurface(SDL_HWSURFACE,a,b,c.BitsPerPixel,c.Rmask,c.Gmask,c.Bmask,c.Amask)
    #define SDL_CREATE_RGB_SURFACE_FROM_COMPAT(a,b,c,d,e,f) SDL_CreateRGBSurfaceFrom(a,b,c,d,e,f.Rmask,f.Gmask,f.Bmask,f.Amask)

    inline void GetWindowSize(int *w, int *h){
        const SDL_VideoInfo* info = SDL_GetVideoInfo();
        if (info)
        {
            *w  = info->current_w;
            *h = info->current_h;
        }
    }

    #define SDL_PIXEL_FORMAT_COMPAT SDL_PixelFormat
    #define WIN98
    #if defined(_MSC_VER) && (_MSC_VER >= 1600)
    #undef WIN98
    #endif

    typedef struct {
        int BPP;
        Uint32 Rmask, Gmask, Bmask, Amask;
    } SDL_PIXEL_FORMAT_COMPAT_CACHED;


    const SDL_PIXEL_FORMAT_COMPAT_CACHED SDL_PIXELFORMAT_UNKNOWN_CACHED = {0, 0, 0, 0, 0};
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        const SDL_PIXEL_FORMAT_COMPAT_CACHED SDL_PIXELFORMAT_RGBA32_CACHED = {
            32,
            0xFF000000, // R
            0x00FF0000, // G
            0x0000FF00, // B
            0x000000FF  // A
        };
        const SDL_PIXEL_FORMAT_COMPAT_CACHED SDL_PIXELFORMAT_RGBA5551_CACHED = {
            16,
            0xF800, // R (bits 15-11)
            0x07C0, // G (10-6)
            0x003E, // B (5-1)
            0x0001  // A (0)
        };
        const SDL_PIXEL_FORMAT_COMPAT_CACHED SDL_PIXELFORMAT_RGB24_CACHED = {
            24,
            0xFF0000,
            0x00FF00,
            0x0000FF,
            0
        };
    #else
        const SDL_PIXEL_FORMAT_COMPAT_CACHED SDL_PIXELFORMAT_RGBA32_CACHED = {
            32,
            0x000000FF, // R
            0x0000FF00, // G
            0x00FF0000, // B
            0xFF000000  // A
        };
        const SDL_PIXEL_FORMAT_COMPAT_CACHED SDL_PIXELFORMAT_RGBA5551_CACHED = {
            16,
            0x001F, // R
            0x03E0, // G
            0x7C00, // B
            0x8000  // A
        };
        const SDL_PIXEL_FORMAT_COMPAT_CACHED SDL_PIXELFORMAT_RGB24_CACHED = {
            24,
            0x0000FF,
            0x00FF00,
            0xFF0000,
            0
        };
    #endif

    //this thing doesn't care about endian at all
    const SDL_PIXEL_FORMAT_COMPAT_CACHED SDL_PIXELFORMAT_RGBA4444_CACHED = {
        16,
        0xF000, // R (15-12)
        0x0F00, // G (11-8)
        0x00F0, // B (7-4)
        0x000F  // A (3-0)
    };
    const SDL_PIXEL_FORMAT_COMPAT_CACHED SDL_PIXELFORMAT_RGB565_CACHED = {
        16,
        0xF800, // R (15-11)
        0x07E0, // G (10-5)
        0x001F, // B (4-0)
        0
    };
    static SDL_PIXEL_FORMAT_COMPAT SDL_PIXELFORMAT_UNKNOWN;
    static SDL_PIXEL_FORMAT_COMPAT SDL_PIXELFORMAT_RGBA32;
    static SDL_PIXEL_FORMAT_COMPAT SDL_PIXELFORMAT_RGBA5551;
    static SDL_PIXEL_FORMAT_COMPAT SDL_PIXELFORMAT_RGB24;
    static SDL_PIXEL_FORMAT_COMPAT SDL_PIXELFORMAT_RGBA4444;
    static SDL_PIXEL_FORMAT_COMPAT SDL_PIXELFORMAT_RGB565;

    inline void InitPixelFormat(
        SDL_PixelFormat* dst,
        SDL_PIXEL_FORMAT_COMPAT_CACHED fmt)
    {
        SDL_Surface *tmp =
        SDL_CreateRGBSurface(
            SDL_HWSURFACE,
            0,0,
            // src->w,
            // src->h,
            fmt.BPP,
            fmt.Rmask,
            fmt.Gmask,
            fmt.Bmask,
            fmt.Amask
        );
        *dst = *tmp->format;

        // SDL_FreeSurface(tmp);
    }

    inline void SDL_PIXEL_FORMAT_COMPAT_LOAD(){
        InitPixelFormat(
            &SDL_PIXELFORMAT_RGBA32,
            SDL_PIXELFORMAT_RGBA32_CACHED);

        InitPixelFormat(
            &SDL_PIXELFORMAT_RGBA5551,
            SDL_PIXELFORMAT_RGBA5551_CACHED);

        InitPixelFormat(
            &SDL_PIXELFORMAT_RGB24,
            SDL_PIXELFORMAT_RGB24_CACHED);

        InitPixelFormat(
            &SDL_PIXELFORMAT_RGBA4444,
            SDL_PIXELFORMAT_RGBA4444_CACHED);

        InitPixelFormat(
            &SDL_PIXELFORMAT_RGB565,
            SDL_PIXELFORMAT_RGB565_CACHED);
    }

    inline SDL_Color SDL_TEXT_COLOR_COMPAT(ZunColor shadowColor){
        SDL_Color sdlShadowColor;
        sdlShadowColor.b = (shadowColor >> 16) & 0xFF;
        sdlShadowColor.g = (shadowColor >> 8) & 0xFF;
        sdlShadowColor.r = shadowColor & 0xFF;
        return sdlShadowColor;
    }

    #define SDL_AUDIO_DEVICE_ID_COMPAT int
    static inline Sint64 SDL_RW_SIZE_COMPAT(SDL_RWOPS_COMPAT* ctx)
    {
        Sint64 pos = SDL_RWTELL_COMPAT(ctx);
        Sint64 size = SDL_RWSEEK_COMPAT(ctx, 0, RW_SEEK_END);
        SDL_RWSEEK_COMPAT(ctx, pos, RW_SEEK_SET);
        return size;
    }
    #define SDL_OPEN_AUDIO_COMPAT(a,b) SDL_OpenAudio(a,b)
    #define SDL_OPEN_AUDIO_COMPAT_ERROR -1
    #define SDL_RESUME_AUDIO_COMPAT(a,b) SDL_PauseAudio(0)
    #define SDL_PAUSE_AUDIO_COMPAT(a) SDL_PauseAudio(1)
    #define SDL_CLOSE_AUDIO_COMPAT(a) SDL_CloseAudio()
    #define SDL_QUEUE_AUDIO_COMPAT(a,b,c,d)


    //Replacement

    #define SDL_SetSurfaceBlendMode(a,b)
#endif

//DISABLE SDL LOG DEBUGGER (SET 1 for disable)
#if 1
#undef SDL_LOG_COMPAT
#define SDL_LOG_COMPAT
#endif