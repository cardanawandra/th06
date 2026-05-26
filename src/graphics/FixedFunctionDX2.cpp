#include <SDL.h>
#include "FixedFunctionDX2.hpp"
#include "Supervisor.hpp"
#include "GameWindow.hpp"
#include "i18n.hpp"
#include "SDLCompat.hpp"

// ---------------------------------------------------------------------------
// Internal helpers (file-scope, not exposed in the header)
// ---------------------------------------------------------------------------

#define DX2_CHECK(hr) do { lastResult = (hr); } while(0)

static D3DPRIMITIVETYPE MapPrimType(PrimitiveType t)
{
    switch (t)
    {
        case PRIM_TRIANGLE_STRIP: return D3DPT_TRIANGLESTRIP;
        case PRIM_TRIANGLES:      return D3DPT_TRIANGLELIST;
        default:                  return D3DPT_TRIANGLELIST;
    }
}

static void ApplyBlendMode(IDirect3DDevice2 *dev, BlendMode mode)
{
    if (mode == BLEND_INV_SRC_ALPHA)
    {
        dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        dev->SetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA);
        dev->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    }
    else
    {
        dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        dev->SetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA);
        dev->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
    }
}

// ---------------------------------------------------------------------------
// SetContextFlags
// ---------------------------------------------------------------------------
void FixedFunctionDX2::SetContextFlags()
{
    // No-op: no GL context attributes to set for DirectDraw.
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
GfxInterface *FixedFunctionDX2::Init()
{
    LOG_COMPAT("FixedFunctionDX2::Init 1\n");

    FixedFunctionDX2 *gfx = new FixedFunctionDX2();

    gfx->boundTexture = -1;
    gfx->clearDepth   = 1.0f;
    gfx->vpNear       = 0.0f;
    gfx->vpFar        = 1.0f;
    gfx->lastResult   = DD_OK;

    // =========================================================
    // WIN32 WINDOW (C++98 SAFE)
    // =========================================================
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));

    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "DX2Window";

    RegisterClass(&wc);

    // NO SDL PROCESSING
    g_GameWindow.CONFIGURE_INIT();
    g_GameWindow.CONFIGURE_VIEW();
    int width  = g_GameWindow.GAME_WINDOW_WIDTH_REAL;
    int height = g_GameWindow.GAME_WINDOW_HEIGHT_REAL;

    HWND hwnd = CreateWindowEx(
        0,
        "DX2Window",
        TH_WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        NULL,
        NULL,
        wc.hInstance,
        NULL
    );

    if (!hwnd)
    {
        LOG_COMPAT("CreateWindowEx failed\n");
        delete gfx;
        return NULL;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    gfx->hwnd = hwnd;

    if (!IsWindow(gfx->hwnd))
    {
        LOG_COMPAT("Invalid HWND\n");
        delete gfx;
        return NULL;
    }

    Sleep(20); // DX2 stability delay

    // =========================================================
    // DIRECTDRAW2
    // =========================================================
    IDirectDraw *dd1 = NULL;
    HRESULT hr;

    hr = DirectDrawCreate(NULL, &dd1, NULL);
    if (FAILED(hr))
    {
        LOG_COMPAT("DirectDrawCreate failed : 0x%08X\n", hr);
        delete gfx;
        return NULL;
    }

    hr = dd1->QueryInterface(IID_IDirectDraw2, (void**)&gfx->dd);
    dd1->Release();

    if (FAILED(hr))
    {
        LOG_COMPAT("QueryInterface IDirectDraw2 failed : 0x%08X\n", hr);
        delete gfx;
        return NULL;
    }

    hr = gfx->dd->SetCooperativeLevel(
        gfx->hwnd,
        DDSCL_EXCLUSIVE |
        DDSCL_FULLSCREEN |
        DDSCL_ALLOWREBOOT
    );
    if (FAILED(hr))
    {
        LOG_COMPAT("SetCooperativeLevel failed : 0x%08X\n", hr);
        delete gfx;
        return NULL;
    }

    // =========================================================
    // PRIMARY SURFACE
    // =========================================================
    DDSURFACEDESC ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));

    ddsd.dwSize = sizeof(ddsd);

    ddsd.dwFlags =
        DDSD_CAPS |
        DDSD_BACKBUFFERCOUNT;

    ddsd.ddsCaps.dwCaps =
        DDSCAPS_PRIMARYSURFACE |
        DDSCAPS_FLIP |
        DDSCAPS_COMPLEX |
        DDSCAPS_3DDEVICE;

    ddsd.dwBackBufferCount = 1;
    
    hr = gfx->dd->CreateSurface(&ddsd, &gfx->primarySurf, NULL);
    if (FAILED(hr))
    {
        LOG_COMPAT("Primary surface failed : 0x%08X\n", hr);
        delete gfx;
        return NULL;
    }

    // =========================================================
    // BACKBUFFER
    // =========================================================
    DDSURFACEDESC back;
    ZeroMemory(&back, sizeof(back));

    back.dwSize = sizeof(DDSURFACEDESC);
    back.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    back.dwWidth  = width;
    back.dwHeight = height;

    back.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    hr = gfx->dd->CreateSurface(&back, &gfx->backSurf, NULL);
    if (FAILED(hr))
    {
        LOG_COMPAT("Backbuffer failed : 0x%08X\n", hr);
        delete gfx;
        return NULL;
    }

    // =========================================================
    // Z BUFFER
    // =========================================================
    DDSURFACEDESC zsd;
    ZeroMemory(&zsd, sizeof(zsd));

    zsd.dwSize = sizeof(DDSURFACEDESC);
    zsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_ZBUFFERBITDEPTH;

    zsd.dwWidth  = width;
    zsd.dwHeight = height;
    zsd.dwZBufferBitDepth = 16;

    zsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_SYSTEMMEMORY;

    hr = gfx->dd->CreateSurface(&zsd, &gfx->zbufSurf, NULL);
    if (FAILED(hr))
    {
        LOG_COMPAT("Zbuffer failed : 0x%08X\n", hr);
        delete gfx;
        return NULL;
    }

    gfx->backSurf->AddAttachedSurface(gfx->zbufSurf);

    // =========================================================
    // DIRECT3D2
    // =========================================================
    hr = gfx->dd->QueryInterface(IID_IDirect3D2, (void**)&gfx->d3d);
    if (FAILED(hr))
    {
        LOG_COMPAT("QueryInterface IDirect3D2 failed : 0x%08X\n", hr);
        delete gfx;
        return NULL;
    }

    hr = gfx->d3d->CreateDevice(IID_IDirect3DHALDevice,
                                gfx->primarySurf,
                                &gfx->device);

    if (FAILED(hr))
    {
        LOG_COMPAT("HAL device failed : 0x%08X\n", hr);

        hr = gfx->d3d->CreateDevice(IID_IDirect3DRGBDevice,
                                    gfx->primarySurf,
                                    &gfx->device);

        if (FAILED(hr))
        {
            LOG_COMPAT("RGB fallback failed : 0x%08X\n", hr);
            delete gfx;
            return NULL;
        }
    }

    // =========================================================
    // VIEWPORT
    // =========================================================
    hr = gfx->d3d->CreateViewport(&gfx->vp, NULL);
    if (FAILED(hr))
    {
        LOG_COMPAT("CreateViewport failed : 0x%08X\n", hr);
        delete gfx;
        return NULL;
    }

    gfx->device->AddViewport(gfx->vp);
    gfx->device->SetCurrentViewport(gfx->vp);

    D3DVIEWPORT2 vp;
    ZeroMemory(&vp, sizeof(vp));

    vp.dwSize   = sizeof(D3DVIEWPORT2);
    vp.dwX      = 0;
    vp.dwY      = 0;
    vp.dwWidth  = width;
    vp.dwHeight = height;
    vp.dvMinZ   = 0.0f;
    vp.dvMaxZ   = 1.0f;

    gfx->vp->SetViewport2(&vp);

    // =========================================================
    // STATES
    // =========================================================
    gfx->device->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
    gfx->device->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    gfx->device->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    gfx->device->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);

    LOG_COMPAT("FixedFunctionDX2::Init OK\n");
    return gfx;
}

// ---------------------------------------------------------------------------
// Exit
// ---------------------------------------------------------------------------
void FixedFunctionDX2::Exit()
{
    if (execBuf) { execBuf->Release(); execBuf = NULL; }

    for (i32 i = 0; i < static_cast<i32>(textures.size()); ++i)
    {
        if (textures[i].tex)  { textures[i].tex->Release();  textures[i].tex  = NULL; }
        if (textures[i].surf) { textures[i].surf->Release(); textures[i].surf = NULL; }
    }
    textures.clear();

    if (clearMat)    { clearMat->Release();              clearMat    = NULL; }
    if (vp)          { device->DeleteViewport(vp);
                       vp->Release();                    vp          = NULL; }
    if (device)      { device->Release();                device      = NULL; }
    if (d3d)         { d3d->Release();                   d3d         = NULL; }
    if (zbufSurf)    { zbufSurf->Release();              zbufSurf    = NULL; }
    if (backSurf)    { backSurf->Release();              backSurf    = NULL; }
    if (primarySurf) { primarySurf->Release();           primarySurf = NULL; }
    if (dd)          { dd->Release();                    dd          = NULL; }
}

// ---------------------------------------------------------------------------
// Fog
// ---------------------------------------------------------------------------
void FixedFunctionDX2::SetFogRange(f32 nearPlane, f32 farPlane)
{
    // device->SetRenderState(D3DRENDERSTATE_FOGSTART,
    //                        *reinterpret_cast<DWORD *>(&nearPlane));
    // device->SetRenderState(D3DRENDERSTATE_FOGEND,
    //                        *reinterpret_cast<DWORD *>(&farPlane));
}

void FixedFunctionDX2::SetFogColor(ZunColor color)
{
    device->SetRenderState(D3DRENDERSTATE_FOGCOLOR, static_cast<DWORD>(color));
}

// ---------------------------------------------------------------------------
// Vertex attributes
// ---------------------------------------------------------------------------
void FixedFunctionDX2::ToggleVertexAttribute(u8 attr, bool enable)
{
    if (attr & VERTEX_ATTR_TEX_COORD)
    {
        if (enable)
            device->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATEALPHA);
        else
            device->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_COPY);

        attribs[VERTEX_ARRAY_TEX_COORD].enabled = enable;
    }

    if (attr & VERTEX_ATTR_DIFFUSE)
    {
        attribs[VERTEX_ARRAY_DIFFUSE].enabled = enable;
    }
}

void FixedFunctionDX2::SetAttributePointer(VertexAttributeArrays attr,
                                            size_t stride, void *ptr)
{
    int idx = static_cast<int>(attr);
    if (idx < kMaxAttribs)
    {
        attribs[idx].stride = stride;
        attribs[idx].ptr    = ptr;
    }
}

// ---------------------------------------------------------------------------
// Texture combiners
// ---------------------------------------------------------------------------
void FixedFunctionDX2::SetColorOp(TextureOpComponent component, ColorOp op)
{
    if (component > COMPONENT_ALPHA || op > COLOR_OP_REPLACE)
        return;

    switch (op)
    {
        case COLOR_OP_MODULATE:
            device->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATEALPHA);
            break;
        case COLOR_OP_ADD:
            device->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_ADD);
            break;
        case COLOR_OP_REPLACE:
            device->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_DECAL);
            break;
    }
}

void FixedFunctionDX2::SetTextureFactor(ZunColor factor)
{
    (void)factor;
}

// ---------------------------------------------------------------------------
// Transforms
// ---------------------------------------------------------------------------
void FixedFunctionDX2::SetTransformMatrix(TransformMatrix type,
                                           const ZunMatrix &matrix)
{
    D3DMATRIX m;
    memcpy(&m, &matrix, sizeof(m));

    switch (type)
    {
        case MATRIX_MODEL:      device->SetTransform(D3DTRANSFORMSTATE_WORLD,      &m); break;
        case MATRIX_VIEW:       device->SetTransform(D3DTRANSFORMSTATE_VIEW,       &m); break;
        case MATRIX_PROJECTION: device->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &m); break;
        default: break;
    }
}

// ---------------------------------------------------------------------------
// Texture filter
// ---------------------------------------------------------------------------
void FixedFunctionDX2::SetTextureFilter()
{
    device->SetRenderState(D3DRENDERSTATE_TEXTUREMAG, D3DFILTER_LINEAR);
    device->SetRenderState(D3DRENDERSTATE_TEXTUREMIN, D3DFILTER_LINEAR);
}

// ---------------------------------------------------------------------------
// Viewport / depth range
// ---------------------------------------------------------------------------
void FixedFunctionDX2::GetViewport(u32 *viewport)
{
    viewport[0] = static_cast<u32>(vpX);
    viewport[1] = static_cast<u32>(vpY);
    viewport[2] = static_cast<u32>(vpWidth);
    viewport[3] = static_cast<u32>(vpHeight);
}

void FixedFunctionDX2::GetDepthRange(f32 *depthRange)
{
    depthRange[0] = vpNear;
    depthRange[1] = vpFar;
}

void FixedFunctionDX2::SetViewport(i32 x, i32 y, i32 width, i32 height)
{
    vpX = x; vpY = y; vpWidth = width; vpHeight = height;

    D3DVIEWPORT2 d3dvp;
    memset(&d3dvp, 0, sizeof(d3dvp));
    d3dvp.dwSize      = sizeof(d3dvp);
    d3dvp.dwX         = static_cast<DWORD>(x);
    d3dvp.dwY         = static_cast<DWORD>(y);
    d3dvp.dwWidth     = static_cast<DWORD>(width);
    d3dvp.dwHeight    = static_cast<DWORD>(height);
    d3dvp.dvClipX     = -1.0f;  d3dvp.dvClipWidth  = 2.0f;
    d3dvp.dvClipY     =  1.0f;  d3dvp.dvClipHeight = 2.0f;
    d3dvp.dvMinZ      = vpNear;
    d3dvp.dvMaxZ      = vpFar;
    DX2_CHECK(vp->SetViewport2(&d3dvp));
}

void FixedFunctionDX2::SetDepthRange(f32 nearPlane, f32 farPlane)
{
    vpNear = nearPlane;
    vpFar  = farPlane;
    SetViewport(vpX, vpY, vpWidth, vpHeight);
}

// ---------------------------------------------------------------------------
// Capabilities / render states
// ---------------------------------------------------------------------------
void FixedFunctionDX2::Enable(Capabilities cap)
{
    switch (cap)
    {
        case CAPS_BLEND:
            device->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
            break;
        case CAPS_DEPTH_TEST:
            device->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
            break;
    }
}

bool FixedFunctionDX2::HasError()
{
    return FAILED(lastResult);
}

void FixedFunctionDX2::SetBlendMode(BlendMode mode)
{
    ApplyBlendMode(device, mode);
}

void FixedFunctionDX2::SetDepthMask(bool enable)
{
    device->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, enable ? TRUE : FALSE);
}

void FixedFunctionDX2::SetDepthFunc(DepthFunc func)
{
    if (func == DEPTH_FUNC_ALWAYS)
        device->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);
    else
        device->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------
void FixedFunctionDX2::SetClearDepth(f32 depth)
{
    clearDepth = depth;
}

void FixedFunctionDX2::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    BYTE A = static_cast<BYTE>(a * 255.0f);
    BYTE R = static_cast<BYTE>(r * 255.0f);
    BYTE G = static_cast<BYTE>(g * 255.0f);
    BYTE B = static_cast<BYTE>(b * 255.0f);
    clearColorDWORD = (static_cast<DWORD>(A) << 24) |
                      (static_cast<DWORD>(R) << 16) |
                      (static_cast<DWORD>(G) <<  8) |
                       static_cast<DWORD>(B);
}

void FixedFunctionDX2::Clear(u32 clearBits)
{
    if (clearBits & CLEAR_COLOR_BUFFER)
    {
        DDBLTFX bltfx;
        memset(&bltfx, 0, sizeof(bltfx));
        bltfx.dwSize      = sizeof(bltfx);
        bltfx.dwFillColor = clearColorDWORD;
        backSurf->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &bltfx);
    }
    if (clearBits & CLEAR_DEPTH_BUFFER)
    {
        DDBLTFX bltfx;
        memset(&bltfx, 0, sizeof(bltfx));
        bltfx.dwSize      = sizeof(bltfx);
        bltfx.dwFillColor = static_cast<DWORD>(clearDepth * 0xFFFF);
        zbufSurf->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &bltfx);
    }
}

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------
GfxTextureHandle FixedFunctionDX2::CreateTexture()
{
    TextureEntry e;
    memset(&e, 0, sizeof(e));
    e.valid = true;
    textures.push_back(e);
    return static_cast<GfxTextureHandle>(textures.size() - 1);
}

void FixedFunctionDX2::BindTexture(GfxTextureHandle handle)
{
    boundTexture = static_cast<i32>(handle);
    if (boundTexture < 0 ||
        boundTexture >= static_cast<i32>(textures.size()) ||
        !textures[boundTexture].valid)
    {
        device->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
        return;
    }
    TextureEntry &e = textures[boundTexture];
    if (e.tex)
    {
        D3DTEXTUREHANDLE th;
        e.tex->GetHandle(device, &th);
        device->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, th);
    }
}

void FixedFunctionDX2::DeleteTexture(GfxTextureHandle handle)
{
    i32 idx = static_cast<i32>(handle);
    if (idx < 0 || idx >= static_cast<i32>(textures.size())) return;
    TextureEntry &e = textures[idx];
    if (e.tex)  { e.tex->Release();  e.tex  = NULL; }
    if (e.surf) { e.surf->Release(); e.surf = NULL; }
    e.valid = false;
}

/*static*/ void FixedFunctionDX2::_FillPixelFormat(DDPIXELFORMAT &pf,
                                                    PixelFormat fmt,
                                                    PixelDataType type)
{
    memset(&pf, 0, sizeof(pf));
    pf.dwSize = sizeof(pf);

    switch (fmt)
    {
        case PIXEL_RGBA:
            pf.dwFlags           = DDPF_RGB | DDPF_ALPHAPIXELS;
            pf.dwRGBBitCount     = 32;
            pf.dwRBitMask        = 0x00FF0000;
            pf.dwGBitMask        = 0x0000FF00;
            pf.dwBBitMask        = 0x000000FF;
            pf.dwRGBAlphaBitMask = 0xFF000000;
            break;
        case PIXEL_RGB:
        default:
            pf.dwFlags       = DDPF_RGB;
            pf.dwRGBBitCount = 24;
            pf.dwRBitMask    = 0x00FF0000;
            pf.dwGBitMask    = 0x0000FF00;
            pf.dwBBitMask    = 0x000000FF;
            break;
    }

    switch (type)
    {
        case PIXEL_UNSIGNED_SHORT_5_5_5_1:
            pf.dwFlags           = DDPF_RGB | DDPF_ALPHAPIXELS;
            pf.dwRGBBitCount     = 16;
            pf.dwRBitMask        = 0x7C00;
            pf.dwGBitMask        = 0x03E0;
            pf.dwBBitMask        = 0x001F;
            pf.dwRGBAlphaBitMask = 0x8000;
            break;
        case PIXEL_UNSIGNED_SHORT_5_6_5:
            pf.dwFlags       = DDPF_RGB;
            pf.dwRGBBitCount = 16;
            pf.dwRBitMask    = 0xF800;
            pf.dwGBitMask    = 0x07E0;
            pf.dwBBitMask    = 0x001F;
            break;
        case PIXEL_UNSIGNED_SHORT_4_4_4_4:
            pf.dwFlags           = DDPF_RGB | DDPF_ALPHAPIXELS;
            pf.dwRGBBitCount     = 16;
            pf.dwRBitMask        = 0x0F00;
            pf.dwGBitMask        = 0x00F0;
            pf.dwBBitMask        = 0x000F;
            pf.dwRGBAlphaBitMask = 0xF000;
            break;
        default:
            break;
    }
}

void FixedFunctionDX2::SetTextureImage(u32 width, u32 height,
                                        PixelFormat fmt, PixelDataType type,
                                        const void *data)
{
    if(!data)return;
    LOG_COMPAT("FixedFunctionDX2::SetTextureImage 1\n");
    if (boundTexture < 0 ||
        boundTexture >= static_cast<i32>(textures.size())) return;
    TextureEntry &e = textures[boundTexture];

    LOG_COMPAT("FixedFunctionDX2::SetTextureImage 2\n");
    if (e.tex)  { e.tex->Release();  e.tex  = NULL; }
    if (e.surf) { e.surf->Release(); e.surf = NULL; }

    e.width    = width;
    e.height   = height;
    e.fmt      = fmt;
    e.dataType = type;

    DDSURFACEDESC ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize  = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwWidth  = width;
    ddsd.dwHeight = height;


    u32 bpp = 4;

    switch (type)
    {
        case PIXEL_UNSIGNED_BYTE:
            bpp = (fmt == PIXEL_RGBA) ? 4 : 3;
            break;

        case PIXEL_UNSIGNED_SHORT_5_5_5_1:
        case PIXEL_UNSIGNED_SHORT_5_6_5:
        case PIXEL_UNSIGNED_SHORT_4_4_4_4:
            bpp = 2;
            break;

        default:
            LOG_COMPAT("Unknown pixel format\n");
            e.surf->Unlock(NULL);
            return;
    }

    u32 rowBytes = width * bpp;

    LOG_COMPAT("FixedFunctionDX2::SetTextureImage 3\n");
    _FillPixelFormat(ddsd.ddpfPixelFormat, fmt, type);

    LOG_COMPAT("FixedFunctionDX2::SetTextureImage 4\n");
    DX2_CHECK(dd->CreateSurface(&ddsd, &e.surf, NULL));
    if (FAILED(lastResult)) return;

    DDSURFACEDESC lockDesc;
    memset(&lockDesc, 0, sizeof(lockDesc));
    lockDesc.dwSize = sizeof(lockDesc);
    LOG_COMPAT("FixedFunctionDX2::SetTextureImage 5\n");

    if (SUCCEEDED(e.surf->Lock(NULL, &lockDesc, DDLOCK_WRITEONLY | DDLOCK_WAIT, NULL)))
    {
        LOG_COMPAT("FixedFunctionDX2::SetTextureImage 5.1\n");
        const BYTE *src  = static_cast<const BYTE *>(data);
        LOG_COMPAT("FixedFunctionDX2::SetTextureImage 5.2\n");
        BYTE       *dst  = static_cast<BYTE *>(lockDesc.lpSurface);
        LOG_COMPAT("FixedFunctionDX2::SetTextureImage 5.5\n");

        if (!lockDesc.lpSurface)
        {
            LOG_COMPAT("lpSurface NULL\n");
            e.surf->Unlock(NULL);
            return;
        }
        
        BYTE *dstRow = dst;
        const BYTE *srcRow = src;

        for (u32 row = 0; row < height; ++row){
            memcpy(dstRow, srcRow, rowBytes);
            dstRow += lockDesc.lPitch;
            srcRow += rowBytes;
        }
        LOG_COMPAT("FixedFunctionDX2::SetTextureImage 5.6\n");
        e.surf->Unlock(NULL);
        LOG_COMPAT("FixedFunctionDX2::SetTextureImage 5.7\n");
    }

    LOG_COMPAT("FixedFunctionDX2::SetTextureImage 6\n");
    DX2_CHECK(e.surf->QueryInterface(IID_IDirect3DTexture2,
                                     reinterpret_cast<void **>(&e.tex)));
    LOG_COMPAT("FixedFunctionDX2::SetTextureImage fin\n");
}

void FixedFunctionDX2::SetTextureSubImage(i32 xoffset, i32 yoffset,
                                           i32 width, i32 height,
                                           const void *data)
{
    LOG_COMPAT("FixedFunctionDX2::SetTextureSubImage 1\n");
    if(!data)return;
    if (boundTexture < 0 ||
        boundTexture >= static_cast<i32>(textures.size())) return;
    TextureEntry &e = textures[boundTexture];
    LOG_COMPAT("FixedFunctionDX2::SetTextureSubImage 2\n");
    if (!e.surf) return;

    RECT r = { xoffset, yoffset, xoffset + width, yoffset + height };
    DDSURFACEDESC lockDesc;
    LOG_COMPAT("FixedFunctionDX2::SetTextureSubImage 3\n");
    memset(&lockDesc, 0, sizeof(lockDesc));
    lockDesc.dwSize = sizeof(lockDesc);

    LOG_COMPAT("FixedFunctionDX2::SetTextureSubImage 4\n");
    if (SUCCEEDED(e.surf->Lock(&r, &lockDesc, DDLOCK_WRITEONLY | DDLOCK_WAIT, NULL)))
    {
        const BYTE *src  = static_cast<const BYTE *>(data);
        BYTE       *dst  = static_cast<BYTE *>(lockDesc.lpSurface);
        u32 bpp      = lockDesc.ddpfPixelFormat.dwRGBBitCount / 8;
        u32 rowBytes = static_cast<u32>(width) * bpp;
        LOG_COMPAT("FixedFunctionDX2::SetTextureSubImage 5\n");
        if (!lockDesc.lpSurface)
        {
            LOG_COMPAT("lpSurface NULL\n");
            e.surf->Unlock(NULL);
            return;
        }
        LOG_COMPAT("FixedFunctionDX2::SetTextureSubImage 6\n");
        for (i32 row = 0; row < height; ++row)
            memcpy(dst + row * lockDesc.lPitch, src + row * rowBytes, rowBytes);
        LOG_COMPAT("FixedFunctionDX2::SetTextureSubImage 7\n");
        e.surf->Unlock(&r);
    }
    LOG_COMPAT("FixedFunctionDX2::SetTextureSubImage fin\n");
}

// ---------------------------------------------------------------------------
// ReadPixels
// ---------------------------------------------------------------------------
void FixedFunctionDX2::ReadPixels(i32 x, i32 y, i32 width, i32 height,
                                   const void *pixels)
{
    RECT r = { x, y, x + width, y + height };
    DDSURFACEDESC lockDesc;
    memset(&lockDesc, 0, sizeof(lockDesc));
    lockDesc.dwSize = sizeof(lockDesc);
    if (SUCCEEDED(backSurf->Lock(&r, &lockDesc, DDLOCK_READONLY | DDLOCK_WAIT, NULL)))
    {
        const BYTE *src  = static_cast<const BYTE *>(lockDesc.lpSurface);
        BYTE       *dst  = const_cast<BYTE *>(static_cast<const BYTE *>(pixels));
        u32 bpp      = lockDesc.ddpfPixelFormat.dwRGBBitCount / 8;
        u32 rowBytes = static_cast<u32>(width) * bpp;
        for (i32 row = 0; row < height; ++row)
            memcpy(dst + row * rowBytes, src + row * lockDesc.lPitch, rowBytes);
        backSurf->Unlock(&r);
    }
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------
void FixedFunctionDX2::_PackVertices(DX2Vertex *dst, i32 start, i32 count)
{
    const AttrState &posA = attribs[VERTEX_ARRAY_POSITION];
    const AttrState &colA = attribs[VERTEX_ARRAY_DIFFUSE];
    const AttrState &uvA  = attribs[VERTEX_ARRAY_TEX_COORD];

    for (i32 i = 0; i < count; ++i)
    {
        i32 idx = start + i;
        DX2Vertex &v = dst[i];

        if (posA.ptr)
        {
            const f32 *p = reinterpret_cast<const f32 *>(
                static_cast<const BYTE *>(posA.ptr) + idx * posA.stride);
            v.x = p[0]; v.y = p[1]; v.z = p[2];
            v.rhw = 1.0f;
        }

        if (colA.enabled && colA.ptr)
        {
            const BYTE *c = static_cast<const BYTE *>(colA.ptr) + idx * colA.stride;
            v.color = (static_cast<DWORD>(c[3]) << 24) |
                      (static_cast<DWORD>(c[0]) << 16) |
                      (static_cast<DWORD>(c[1]) <<  8) |
                       static_cast<DWORD>(c[2]);
        }
        else
        {
            v.color = 0xFFFFFFFF;
        }
        v.specular = 0xFF000000;

        if (uvA.enabled && uvA.ptr)
        {
            const f32 *uv = reinterpret_cast<const f32 *>(
                static_cast<const BYTE *>(uvA.ptr) + idx * uvA.stride);
            v.tu = uv[0];
            v.tv = uv[1];
        }
        else
        {
            v.tu = v.tv = 0.0f;
        }
    }
}

void FixedFunctionDX2::_ExecuteDraw(D3DPRIMITIVETYPE primType, i32 start, i32 count)
{
    LOG_COMPAT("FixedFunctionDX2::_ExecuteDraw 1\n");
    DWORD vtxBytes  = static_cast<DWORD>(count) * sizeof(DX2Vertex);
    DWORD instrBase = vtxBytes;

    i32 primCount = 0;
    switch (primType)
    {
        case D3DPT_TRIANGLELIST:  primCount = count / 3;  break;
        case D3DPT_TRIANGLESTRIP:
        case D3DPT_TRIANGLEFAN:   primCount = count - 2;  break;
        case D3DPT_LINELIST:      primCount = count / 2;  break;
        case D3DPT_LINESTRIP:     primCount = count - 1;  break;
        case D3DPT_POINTLIST:     primCount = count;      break;
        default:                  primCount = count / 3;  break;
    }

    DWORD instrBytes =
        sizeof(D3DINSTRUCTION) + sizeof(D3DPROCESSVERTICES) +
        sizeof(D3DINSTRUCTION) + static_cast<DWORD>(primCount) * sizeof(D3DTRIANGLE) +
        sizeof(D3DINSTRUCTION);

    DWORD totalSize = vtxBytes + instrBytes;

    LOG_COMPAT("FixedFunctionDX2::_ExecuteDraw 2\n");
    if (!execBuf || execBufSize < totalSize)
    {
        if (execBuf) { execBuf->Release(); execBuf = NULL; }
        D3DEXECUTEBUFFERDESC ebd;
        memset(&ebd, 0, sizeof(ebd));
        ebd.dwSize       = sizeof(ebd);
        ebd.dwFlags      = D3DDEB_BUFSIZE;
        ebd.dwBufferSize = totalSize;
        // DX2_CHECK(device->CreateExecuteBuffer(&ebd, &execBuf, NULL));
        if (FAILED(lastResult)) return;
        execBufSize = totalSize;
    }
 
    LOG_COMPAT("FixedFunctionDX2::_ExecuteDraw 2+\n");

    D3DEXECUTEBUFFERDESC lockDesc;
    memset(&lockDesc, 0, sizeof(lockDesc));
    lockDesc.dwSize = sizeof(lockDesc);
    // DX2_CHECK(execBuf->Lock(&lockDesc));
    LOG_COMPAT("FixedFunctionDX2::_ExecuteDraw 2++\n");
    return;
    if (FAILED(lastResult)) return;

    BYTE *base = static_cast<BYTE *>(lockDesc.lpData);

    _PackVertices(reinterpret_cast<DX2Vertex *>(base), start, count);

    BYTE *ip = base + instrBase;

    LOG_COMPAT("FixedFunctionDX2::_ExecuteDraw 3\n");
    // PROCESSVERTICES
    {
        D3DINSTRUCTION *ins = reinterpret_cast<D3DINSTRUCTION *>(ip);
        ins->bOpcode = D3DOP_PROCESSVERTICES;
        ins->bSize   = sizeof(D3DPROCESSVERTICES);
        ins->wCount  = 1;
        ip += sizeof(D3DINSTRUCTION);

        D3DPROCESSVERTICES *pv = reinterpret_cast<D3DPROCESSVERTICES *>(ip);
        pv->dwFlags    = D3DPROCESSVERTICES_COPY;
        pv->wStart     = 0;
        pv->wDest      = 0;
        pv->dwCount    = static_cast<DWORD>(count);
        pv->dwReserved = 0;
        ip += sizeof(D3DPROCESSVERTICES);
    }

    LOG_COMPAT("FixedFunctionDX2::_ExecuteDraw 4\n");
    // Geometry
    if (primType == D3DPT_TRIANGLELIST  ||
        primType == D3DPT_TRIANGLESTRIP ||
        primType == D3DPT_TRIANGLEFAN)
    {
        D3DINSTRUCTION *ins = reinterpret_cast<D3DINSTRUCTION *>(ip);
        ins->bOpcode = D3DOP_TRIANGLE;
        ins->bSize   = sizeof(D3DTRIANGLE);
        ins->wCount  = static_cast<WORD>(primCount);
        ip += sizeof(D3DINSTRUCTION);

        for (i32 t = 0; t < primCount; ++t)
        {
            D3DTRIANGLE *tri = reinterpret_cast<D3DTRIANGLE *>(ip);
            if (primType == D3DPT_TRIANGLELIST)
            {
                tri->v1 = static_cast<WORD>(t * 3 + 0);
                tri->v2 = static_cast<WORD>(t * 3 + 1);
                tri->v3 = static_cast<WORD>(t * 3 + 2);
            }
            else if (primType == D3DPT_TRIANGLESTRIP)
            {
                tri->v1 = static_cast<WORD>(t);
                tri->v2 = static_cast<WORD>(t + 1);
                tri->v3 = static_cast<WORD>(t + 2);
            }
            else // FAN
            {
                tri->v1 = 0;
                tri->v2 = static_cast<WORD>(t + 1);
                tri->v3 = static_cast<WORD>(t + 2);
            }
            tri->wFlags = D3DTRIFLAG_EDGEENABLETRIANGLE;
            ip += sizeof(D3DTRIANGLE);
        }
    }
    else if (primType == D3DPT_LINELIST || primType == D3DPT_LINESTRIP)
    {
        D3DINSTRUCTION *ins = reinterpret_cast<D3DINSTRUCTION *>(ip);
        ins->bOpcode = D3DOP_LINE;
        ins->bSize   = sizeof(D3DLINE);
        ins->wCount  = static_cast<WORD>(primCount);
        ip += sizeof(D3DINSTRUCTION);

        for (i32 l = 0; l < primCount; ++l)
        {
            D3DLINE *line = reinterpret_cast<D3DLINE *>(ip);
            if (primType == D3DPT_LINELIST)
            {
                line->v1 = static_cast<WORD>(l * 2);
                line->v2 = static_cast<WORD>(l * 2 + 1);
            }
            else
            {
                line->v1 = static_cast<WORD>(l);
                line->v2 = static_cast<WORD>(l + 1);
            }
            ip += sizeof(D3DLINE);
        }
    }
    else if (primType == D3DPT_POINTLIST)
    {
        D3DINSTRUCTION *ins = reinterpret_cast<D3DINSTRUCTION *>(ip);
        ins->bOpcode = D3DOP_POINT;
        ins->bSize   = sizeof(D3DPOINT);
        ins->wCount  = static_cast<WORD>(primCount);
        ip += sizeof(D3DINSTRUCTION);

        for (i32 p = 0; p < primCount; ++p)
        {
            D3DPOINT *pt = reinterpret_cast<D3DPOINT *>(ip);
            pt->wFirst = static_cast<WORD>(p);
            pt->wCount = 1;
            ip += sizeof(D3DPOINT);
        }
    }

    LOG_COMPAT("FixedFunctionDX2::_ExecuteDraw 5\n");
    // EXIT
    {
        D3DINSTRUCTION *ins = reinterpret_cast<D3DINSTRUCTION *>(ip);
        ins->bOpcode = D3DOP_EXIT;
        ins->bSize   = 0;
        ins->wCount  = 0;
    }

    execBuf->Unlock();

    D3DEXECUTEDATA ed;
    memset(&ed, 0, sizeof(ed));
    ed.dwSize              = sizeof(ed);
    ed.dwVertexOffset      = 0;
    ed.dwVertexCount       = static_cast<DWORD>(count);
    ed.dwInstructionOffset = instrBase;
    ed.dwInstructionLength = instrBytes;
    DX2_CHECK(execBuf->SetExecuteData(&ed));
    if (FAILED(lastResult)) return;

    LOG_COMPAT("FixedFunctionDX2::_ExecuteDraw fin\n");
    // DX2_CHECK(device->Execute(execBuf, vp, D3DEXECUTE_UNCLIPPED));
}

void FixedFunctionDX2::Draw(PrimitiveType type, i32 start, i32 count)
{
    _ExecuteDraw(MapPrimType(type), start, count);
}

// ---------------------------------------------------------------------------
// SwapBuffers
// ---------------------------------------------------------------------------
void FixedFunctionDX2::SwapBuffers()
{
    if (!primarySurf || !backSurf)
        return;

    RECT rcClient;
    GetClientRect(hwnd, &rcClient);

    POINT pt1;
    pt1.x = rcClient.left;
    pt1.y = rcClient.top;

    POINT pt2;
    pt2.x = rcClient.right;
    pt2.y = rcClient.bottom;

    ClientToScreen(hwnd, &pt1);
    ClientToScreen(hwnd, &pt2);

    RECT rcDst;
    rcDst.left   = pt1.x;
    rcDst.top    = pt1.y;
    rcDst.right  = pt2.x;
    rcDst.bottom = pt2.y;

    HRESULT hr = primarySurf->Blt(
        &rcDst,
        backSurf,
        NULL,
        DDBLT_WAIT,
        NULL);

    if (FAILED(hr))
    {
        LOG_COMPAT(
            "Blt failed : 0x%08X\n",
            hr);
    }

    SDL_PumpEvents();
}