#pragma once

#ifndef NO_SDL
    #include <SDL.h>
    typedef int8_t i8;
    typedef uint8_t u8;
    typedef int16_t i16;
    typedef uint16_t u16;
    typedef int32_t i32;
    typedef uint32_t u32;
#else
    #if __cplusplus
    #include <climits>
    #else
    #include <limits.h>
    #endif

    #if defined(_MSC_VER)
    typedef signed __int32 i32;
    #elif INT_MIN == -2147483648l && INT_MAX == 2147483647l
    typedef signed int i32;
    #elif LONG_MIN == -2147483648l && LONG_MAX == 2147483647l
    typedef signed long i32;
    #elif SHORT_MIN == -2147483648l && SHORT_MAX == 2147483647l
    typedef signed short i32;
    #elif SCHAR_MIN == -2147483648l && SCHAR_MAX == 2147483647l
    typedef signed char i32;
    #else
    #error "No i32 type"
    #endif

    #if defined(_MSC_VER)
    typedef unsigned __int32 u32;
    #elif UINT_MAX == 4294967295ul
    typedef unsigned int u32;
    #elif ULONG_MAX == 4294967295ul
    typedef unsigned long u32;
    #elif USHORT_MAX == 4294967295ul
    typedef unsigned short u32;
    #elif UCHAR_MAX == 4294967295ul
    typedef unsigned char u32;
    #else
    #error "No u32 type"
    #endif

    #if defined(_MSC_VER)
    typedef signed __int16 i16;
    #elif INT_MIN == -32768l && INT_MAX == 32767l
    typedef signed int i16;
    #elif SHORT_MIN == -32768l && SHORT_MAX == 32767l
    typedef signed short i16;
    #elif SCHAR_MIN == -32768l && SCHAR_MAX == 32767l
    typedef signed char i16;
    #elif LONG_MIN == -32768l && LONG_MAX == 32767l
    typedef signed long i16;
    #else
    // #error "No i16 type" forced
    typedef signed short i16;
    #endif

    #if defined(_MSC_VER)
    typedef unsigned __int16 u16;
    #elif UINT_MAX == 65535ul
    typedef unsigned int u16;
    #elif USHORT_MAX == 65535ul
    typedef unsigned short u16;
    #elif UCHAR_MAX == 65535ul
    typedef unsigned char u16;
    #elif ULONG_MAX == 65535ul
    typedef unsigned long u16;
    #else
    // #error "No u16 type" forced
    typedef unsigned short u16;
    #endif

    #if defined(_MSC_VER)
    typedef signed __int8 i8;
    #elif INT_MIN == -128l && INT_MAX == 127l
    typedef signed int i8;
    #elif SCHAR_MIN == -128l && SCHAR_MAX == 127l
    typedef signed char i8;
    #elif SHORT_MIN == -128l && SHORT_MAX == 127l
    typedef signed short i8;
    #elif LONG_MIN == -128l && LONG_MAX == 127l
    typedef signed long i8;
    #else
    #error "No i8 type"
    #endif

    #if defined(_MSC_VER)
    typedef unsigned __int8 u8;
    #elif UINT_MAX == 255ul
    typedef unsigned int u8;
    #elif UCHAR_MAX == 255ul
    typedef unsigned char u8;
    #elif USHORT_MAX == 255ul
    typedef unsigned short u8;
    #elif ULONG_MAX == 255ul
    typedef unsigned long u8;
    #else
    #error "No u8 type"
    #endif
#endif
// #ifndef NO_SDL
//     #include <SDL.h>
// #else
//     #ifdef USE_CPP98

//         typedef signed char      int8_t;
//         typedef unsigned char    uint8_t;

//         typedef signed short     int16_t;
//         typedef unsigned short   uint16_t;

//         typedef signed long      int32_t;
//         typedef unsigned long    uint32_t;

//         typedef int              intptr_t;
//     #else
//         #include <stdint.h>
//     #endif
// #endif

// #if defined(_MSC_VER)
//     typedef unsigned __int64   uint64_t;
//     #if _MSC_VER < 1600
//         typedef int intptr_t;
//     #endif
// #elif defined(__WATCOMC__)
//     typedef unsigned __int64   uint64_t;
//     typedef int                intptr_t;
// #else
//     // typedef uint32_t   uint64_t;
// #endif

// typedef int8_t i8;
// typedef uint8_t u8;
// typedef int16_t i16;
// typedef uint16_t u16;
// typedef int32_t i32;
// typedef uint32_t u32;
#ifdef __ANDROID__
#include <stdint.h>
typedef intptr_t iptr;
#else
typedef int iptr;
#endif

typedef float f32;
typedef double f64;

#ifdef __ANDROID__
inline f32 uf32(const f32* ptr) {
    u32 temp = *(const u32*)ptr;
    __asm__ volatile ("":"+r"(temp));
    return __builtin_bit_cast(float, temp);
}
#else
#define uf32(a) *a
#endif