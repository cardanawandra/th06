#pragma once
#include <SDL.h>
#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef int intptr_t;
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