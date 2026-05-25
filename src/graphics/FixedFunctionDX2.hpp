#pragma once
#define RENDER_FIXED_FUNCTION_DX2 true

#include "GfxInterface.hpp"
#include <windows.h>

#ifndef DIRECTDRAW_VERSION
#   define DIRECTDRAW_VERSION 0x0300   // IDirectDraw2
#endif
#ifndef DIRECT3D_VERSION
#   define DIRECT3D_VERSION   0x0300   // IDirect3D2 / IDirect3DDevice2
#endif
#include <ddraw.h>
#include <d3d.h>

#include <SDL.h>
#include <SDL_syswm.h>
#include <vector>

// ---------------------------------------------------------------------------
// Packed vertex written into DX2 execute buffers (D3DTLVERTEX layout)
// ---------------------------------------------------------------------------
struct DX2Vertex
{
    D3DVALUE x, y, z, rhw;
    D3DCOLOR color;
    D3DCOLOR specular;
    D3DVALUE tu, tv;
};

// ---------------------------------------------------------------------------
// FixedFunctionDX2
// ---------------------------------------------------------------------------
struct FixedFunctionDX2 : GfxInterface
{
    static GfxInterface *Init();
    static void          SetContextFlags();

    virtual void Exit();
    virtual ~FixedFunctionDX2() { Exit(); }

    virtual void SetFogRange(f32 nearPlane, f32 farPlane);
    virtual void SetFogColor(ZunColor color);
    virtual void ToggleVertexAttribute(u8 attr, bool enable);
    virtual void SetAttributePointer(VertexAttributeArrays attr, size_t stride, void *ptr);
    virtual void SetColorOp(TextureOpComponent component, ColorOp op);
    virtual void SetTextureFactor(ZunColor factor);
    virtual void SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix);
    virtual void SetTextureFilter();
    virtual void GetViewport(u32 *viewport);
    virtual void GetDepthRange(f32 *depthRange);
    virtual void SetViewport(i32 x, i32 y, i32 width, i32 height);
    virtual void SetDepthRange(f32 nearPlane, f32 farPlane);
    virtual void Enable(Capabilities cap);
    virtual bool HasError();
    virtual void SetBlendMode(BlendMode mode);
    virtual void SetDepthMask(bool enable);
    virtual void SetDepthFunc(DepthFunc func);
    virtual void SetClearDepth(f32 depth);
    virtual void SetClearColor(f32 r, f32 g, f32 b, f32 a);
    virtual void Clear(u32 clearBits);
    virtual GfxTextureHandle CreateTexture();
    virtual void             BindTexture(GfxTextureHandle handle);
    virtual void             DeleteTexture(GfxTextureHandle handle);
    virtual void             SetTextureImage(u32 width, u32 height,
                                             PixelFormat fmt, PixelDataType type,
                                             const void *data);
    virtual void             SetTextureSubImage(i32 xoffset, i32 yoffset,
                                                i32 width, i32 height,
                                                const void *data);
    virtual void ReadPixels(i32 x, i32 y, i32 width, i32 height,
                            const void *pixels);
    virtual void Draw(PrimitiveType type, i32 start, i32 count);
    virtual void SwapBuffers();

private:
    void _ExecuteDraw(D3DPRIMITIVETYPE primType, i32 start, i32 count);
    void _PackVertices(DX2Vertex *dst, i32 start, i32 count);
    static void _FillPixelFormat(DDPIXELFORMAT &pf, PixelFormat fmt, PixelDataType type);

    // DirectDraw / Direct3D COM objects
    IDirectDraw2       *dd;
    IDirectDrawSurface *primarySurf;
    IDirectDrawSurface *backSurf;
    IDirectDrawSurface *zbufSurf;
    IDirect3D2         *d3d;
    IDirect3DDevice2   *device;
    IDirect3DViewport2 *vp;
    IDirect3DMaterial2 *clearMat;

    HWND hwnd;

    // Texture table
    struct TextureEntry
    {
        IDirectDrawSurface *surf;
        IDirect3DTexture2  *tex;
        u32                 width;
        u32                 height;
        PixelFormat         fmt;
        PixelDataType       dataType;
        bool                valid;
    };
    std::vector<TextureEntry> textures;
    i32                       boundTexture;

    // Shadowed CPU-side attribute pointers (mirrors glVertexPointer etc.)
    // enum instead of constexpr for C++98 compatibility
    enum { kMaxAttribs = 8 };
    struct AttrState { bool enabled; size_t stride; void *ptr; };
    AttrState attribs[kMaxAttribs];

    // Cached state
    f32     clearDepth;
    DWORD   clearColorDWORD;
    i32     vpX, vpY, vpWidth, vpHeight;
    f32     vpNear, vpFar;
    HRESULT lastResult;

    // Execute buffer scratch space (grown as needed, reused each frame)
    IDirect3DExecuteBuffer *execBuf;
    DWORD                   execBufSize;
};