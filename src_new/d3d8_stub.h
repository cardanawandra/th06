#pragma once
// d3d8_stub.h - Minimal stubs for D3D8/DSound types used in thprac_th06.cpp
// In the SDL2 build, these are dummy types. The actual rendering uses OpenGL.

#include <cstdint>

// D3D8 stub types
struct IDirect3DDevice8;

// D3DSURFACE_DESC may already be defined by sdl2_compat.hpp
#ifndef D3D8_STUB_SURFACE_DESC
#define D3D8_STUB_SURFACE_DESC
#ifndef __SDL2_COMPAT_HPP__
struct D3DSURFACE_DESC {
    uint32_t Format;
    uint32_t Type;
    uint32_t Usage;
    uint32_t Pool;
    uint32_t Size;
    uint32_t MultiSampleType;
    uint32_t Width;
    uint32_t Height;
};
#endif
#endif

struct IDirect3DTexture8 {
    virtual void __Release() {}
    void Release() {}
    void GetLevelDesc(int /*level*/, D3DSURFACE_DESC* desc) {
        if (desc) {
            desc->Width = 0;
            desc->Height = 0;
        }
    }
};
typedef IDirect3DTexture8* LPDIRECT3DTEXTURE8;

// DirectSound stub types
struct IDirectSoundBuffer {
    void Stop() {}
    void Play(uint32_t, uint32_t, uint32_t) {}
};
