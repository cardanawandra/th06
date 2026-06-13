#pragma once
#include "compat/STBCompat.hpp"
#include "inttypes.hpp"

#define STORAGE_INIT()
#define GET_EXTERNAL_STORAGE_PATH() NULL
#define CALLCOMPAT 
#ifdef _MSC_VER
    #define SNPRINTF _snprintf
#else
    #define SNPRINTF snprintf
#endif
#define LOG_COMPAT printf

#define INT16_MAX_COMPAT 32767
#define JOYSTICK_COMPATButton i16

#define COMPAT_NumJoysticks() 0
#define COMPAT_CONTROLLER_BUTTON_A              0
#define COMPAT_CONTROLLER_BUTTON_B              1
#define COMPAT_CONTROLLER_BUTTON_X              2
#define COMPAT_CONTROLLER_BUTTON_Y              3
#define COMPAT_CONTROLLER_BUTTON_LEFTSHOULDER   4
#define COMPAT_CONTROLLER_BUTTON_RIGHTSHOULDER  5
#define COMPAT_CONTROLLER_BUTTON_BACK           6
#define COMPAT_CONTROLLER_BUTTON_START          7
#define COMPAT_CONTROLLER_BUTTON_LEFTSTICK      8
#define COMPAT_CONTROLLER_BUTTON_RIGHTSTICK     9
#define COMPAT_CONTROLLER_BUTTON_DPAD_UP        10
#define COMPAT_CONTROLLER_BUTTON_DPAD_DOWN      11
#define COMPAT_CONTROLLER_BUTTON_DPAD_LEFT      12
#define COMPAT_CONTROLLER_BUTTON_DPAD_RIGHT     13
#define KEYBOARD_KEY_PRESSED(button, x) keyboardState[x] ? button : 0

#define AUDIO_STREAM_COMPAT bool
#define AUDIO_DEVICE_ID_COMPAT int
#define COMPAT_AudioSpec bool
#define COMPAT_Thread char
#define RWOPS_COMPAT int

#define TRY_RESOLVE_FUNCTION(name) this->name = ::name;

#ifdef NO_SDL

    #ifndef USE_CPP98
        #define BYTEORDER      1
        #ifdef BYTEORDER_BIG_ENDIAN
            #undef BYTEORDER
            #define BYTEORDER 2
        #endif
        #define FLOATWORDORDER BYTEORDER
        #ifdef FLOATWORDORDER_BIG_ENDIAN
            #undef FLOATWORDORDER
            #define FLOATWORDORDER 2
        #endif
        #define LIL_ENDIAN     1
        #define BIG_ENDIAN     2
    #else
        #include <stdint.h>
        #include <bit>
        #define BYTEORDER      std::endian::native
        #define FLOATWORDORDER BYTEORDER
        #define LIL_ENDIAN     std::endian::little
        #define BIG_ENDIAN     std::endian::big
    #endif

    #if BYTEORDER == LIL_ENDIAN
        #define COMPAT_Swap16(a) a
        #define COMPAT_Swap32(a) a
        #define COMPAT_Swap64(a) a
    #else
        inline u16 COMPAT_Swap16(u16 v)
        {
        #if defined(_MSC_VER) && _MSC_VER >= 1400
            return _byteswap_ushort(v);
        #elif defined(__GNUC__) || defined(__clang__)
            return __builtin_bswap16(v);
        #else
            return (u16)((v >> 8) | (v << 8));
        #endif
        }

        inline u32 COMPAT_Swap32(u32 v)
        {
        #if defined(_MSC_VER) && _MSC_VER >= 1400
            return _byteswap_ulong(v);
        #elif defined(__GNUC__) || defined(__clang__)
            return __builtin_bswap32(v);
        #else
            return ((v & 0x000000FFUL) << 24) |
                ((v & 0x0000FF00UL) <<  8) |
                ((v & 0x00FF0000UL) >>  8) |
                ((v & 0xFF000000UL) >> 24);
        #endif
        }

        inline u64 COMPAT_Swap64(u64 v)
        {
        #if defined(_MSC_VER) && _MSC_VER >= 1400
            return _byteswap_uint64(v);
        #elif defined(__GNUC__) || defined(__clang__)
            return __builtin_bswap64(v);
        #else
            return ((u64)COMPAT_Swap32((u32)v) << 32) |
                    (u64)COMPAT_Swap32((u32)(v >> 32));
        #endif
        }

    #endif

    #define COMPAT_CONTROLLER_BUTTON_MAX 15 // adjust to your own enum
    #define JOYSTICK_COMPAT char
    #define JOYSTICK_COMPATOpen(a) NULL
    #define JOYSTICK_COMPATClose()
    #define COMPAT_Quit() ((void)0)
    #define HIDECURSOR_COMPAT()
    #define SHOWCURSOR_COMPAT()

    #include <time.h>
    inline u32 compat_get_ticks()
    {
        static clock_t start = clock();

        return static_cast<u32>(
            (clock() - start) * 1000 / CLOCKS_PER_SEC
        );
    }
    #define GET_TICKS() compat_get_ticks()

    #define COMPAT_TimerCallback int
    #define COMPAT_TimerID int
    
    #ifdef USE_SFML
        #include "compat/SFMLCompat.hpp"
    #endif
#else

    #include "compat/SDLCompat.hpp"
#endif

//DISABLE SDL LOG DEBUGGER (SET 1 for disable)
#if 1
#undef LOG_COMPAT
#define LOG_COMPAT
#endif
