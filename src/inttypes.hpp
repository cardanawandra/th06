#pragma once
#ifndef NO_SDL
    #include <SDL.h>
#else
    #ifdef USE_CPP98

        typedef signed char      int8_t;
        typedef unsigned char    uint8_t;

        typedef signed short     int16_t;
        typedef unsigned short   uint16_t;

        typedef signed long      int32_t;
        typedef unsigned long    uint32_t;

        typedef int                intptr_t;
    #else
        #include <stdint.h>
    #endif
#endif

#if defined(_MSC_VER)
    typedef unsigned __int64   uint64_t;
#elif defined(__WATCOMC__)
    typedef unsigned __int64   uint64_t;
    typedef int                intptr_t;
#else
    // typedef uint32_t   uint64_t;
#endif

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef uint64_t u64;
typedef intptr_t iptr;

typedef float f32;
typedef double f64;

#ifdef __ANDROID__
inline f32 uf32(const f32* ptr) {
    u32 temp = *(const u32*)ptr;
    __asm__ volatile ("":"+r"(temp));
    return __builtin_bit_cast(float, temp);
}
#else
inline f32 uf32(const f32* ptr) {
    return *ptr;
}
#endif