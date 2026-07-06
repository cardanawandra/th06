#pragma once

// Header originally created by Zero318
//   Any bad parts were tacked on by me

#include "compat/Compat.hpp"
#if BYTEORDER == LIL_ENDIAN
template<typename T>
struct LE
{
    T value;

    operator T() const
    {
        return value;
    }

    LE& operator=(T v)
    {
        value = v;
        return *this;
    }
};
#else
#include <cstring>
#include "inttypes.hpp"
#include <type_traits>

static_assert(
    BYTEORDER == LIL_ENDIAN ||
    BYTEORDER == BIG_ENDIAN
    , "System endian must be either big or little!"
);

#ifndef FLOATWORDORDER
#define FLOATWORDORDER BYTEORDER
#endif

static_assert(
    FLOATWORDORDER == LIL_ENDIAN ||
    FLOATWORDORDER == BIG_ENDIAN
    , "Float endian must be either big or little!"
);

template <typename T>
struct UIForSize
{
    // fallback should never be used if you only pass u8/u16/u32
    typedef u32 type;
};

// u8
template <>
struct UIForSize<u8>
{
    typedef u8 type;
};

// u16
template <>
struct UIForSize<u16>
{
    typedef u16 type;
};

// u32
template <>
struct UIForSize<u32>
{
    typedef u32 type;
};

template <>
struct UIForSize<f32>
{
    typedef u32 type;
};

// GCC-ARM without aligned access and GCC-SuperH both fail to inline a fixed-size unaligned memcpy,
//    giving horrid codegen, but on just about every other platform, memcpy gets inlined and
//    produces equal or better codegen than a manual implementation. So this check exists
//    Is this hyperspecific? Yes, but I spent way too much time looking at godbolt for this
//    header in general and I'll be damned if it's going to have suboptimal codegen
#if (defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)) && \
    ((defined(__arm__) && !defined(__ARM_FEATURE_UNALIGNED)) || \
     (defined(__sh__)))
    #define DO_MANUAL_MEMCPY 1
#else
    #define DO_MANUAL_MEMCPY 0
#endif

#if !DO_MANUAL_MEMCPY
template <typename T>
static inline typename UIForSize<T>::type read_to_ui_unaligned(const void *value)
{
    typename UIForSize<T>::type ret;
    std::memcpy(&ret, value, sizeof(T));
    return ret;
}

template <typename U>
static inline void write_from_ui_unaligned(void *dst, const U &src)
{
    std::memcpy(dst, &src, sizeof(U));
}
#else

// Since we have to go byte-by-byte anyway we do the accesses here as little endian.
//   Aside from dodging the byteswap, this is also necessary because if GCC thinks we're
//   doing a memcpy it'll "helpfully" outline it to the libc's memcpy :|

template <typename T>
static inline UIForSize<T> read_to_ui_unaligned(void *value) {
    UIForSize<T> ret = 0;

    for(i32 i = sizeof(T) - 1; i >= 0; i--) {
        ret <<= 8;
        ret |= ((char *) value)[i];
    }

    return ret;
}

template <typename T>
static inline void write_from_ui_unaligned(void *dst, const T &src) {
    T n = src;

    for(size_t i = 0; i < sizeof(T); i++) {
        ((char *)dst)[i] = n & 0xFF;
        n >>= 8;
    }
}
#endif

template <typename T>
static inline typename UIForSize<T>::type bit_cast_from_size(const T &a)
{
    typename UIForSize<T>::type ret;
    std::memcpy(&ret, &a, sizeof(T));
    return ret;
}

template <typename T>
static inline T bit_cast_to_size(typename UIForSize<T>::type value)
{
    T ret;
    std::memcpy(&ret, &value, sizeof(T));
    return ret;
}

static inline u8  ZunByteswap(u8 in)  { return in; }
static inline u16 ZunByteswap(u16 in) { return COMPAT_Swap16(in); }
static inline u32 ZunByteswap(u32 in) { return COMPAT_Swap32(in); }

template <typename T>
struct LE
{
    T raw;

    inline operator T() const
    {
        typename UIForSize<T>::type ui =
            read_to_ui_unaligned<T>(&raw);

        if ((std::is_floating_point<T>::value ? FLOATWORDORDER : BYTEORDER) == BIG_ENDIAN &&
            !DO_MANUAL_MEMCPY)
        {
            ui = ZunByteswap(ui);
        }

        return bit_cast_to_size<T>(ui);
    }

    inline LE& operator=(const T& a)
    {
        typename UIForSize<T>::type ui =
            bit_cast_from_size<T>(a);

        if ((std::is_floating_point<T>::value ? FLOATWORDORDER : BYTEORDER) == BIG_ENDIAN &&
            !DO_MANUAL_MEMCPY)
        {
            ui = ZunByteswap(ui);
        }

        write_from_ui_unaligned(&raw, ui);
        return *this;
    }
};
#undef DO_MANUAL_MEMCPY
#endif