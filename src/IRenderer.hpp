#pragma once
// =============================================================================
// IRenderer.hpp - Abstract rendering interface for th06
// Allows multiple backend implementations (OpenGL 1.x, OpenGL ES 2.0, etc.)
// without coupling game code to a specific graphics API.
// =============================================================================

#include "sdl2_compat.hpp"
#include <SDL.h>

struct VertexDiffuseXyzrwh;
struct VertexTex1Xyzrwh;
struct VertexTex1DiffuseXyzrwh;
struct VertexTex1DiffuseXyz;
struct RenderVertexInfo;

struct IRenderer
{
    virtual ~IRenderer() {}

    // --- Lifecycle ---
    virtual void Init(SDL_Window *win, SDL_GLContext ctx, i32 w, i32 h) = 0;
    virtual void InitDevice(u32 opts) = 0;
    virtual void Release() = 0;
    virtual void ResizeTarget() = 0;
    virtual void BeginScene() = 0;
    virtual void EndScene() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    // --- Clear / Viewport ---
    virtual void Clear(D3DCOLOR color, i32 clearColor, i32 clearDepth) = 0;
    virtual void SetViewport(i32 x, i32 y, i32 w, i32 h, f32 minZ = 0.0f, f32 maxZ = 1.0f) = 0;

    // --- Render State ---
    virtual void SetBlendMode(u8 mode) = 0;
    virtual void SetColorOp(u8 colorOp) = 0;
    virtual void SetTexture(u32 tex) = 0;
    virtual void SetTextureFactor(D3DCOLOR factor) = 0;
    virtual void SetZWriteDisable(u8 disable) = 0;
    virtual void SetDepthFunc(i32 alwaysPass) = 0;
    virtual void SetDestBlendInvSrcAlpha() = 0;
    virtual void SetDestBlendOne() = 0;
    virtual void SetTextureStageSelectDiffuse() = 0;
    virtual void SetTextureStageModulateTexture() = 0;
    virtual void SetFog(i32 enable, D3DCOLOR color, f32 start, f32 end) = 0;

    // --- Transforms ---
    virtual void SetViewTransform(const D3DXMATRIX *matrix) = 0;
    virtual void SetProjectionTransform(const D3DXMATRIX *matrix) = 0;
    virtual void SetWorldTransform(const D3DXMATRIX *matrix) = 0;
    virtual void SetTextureTransform(const D3DXMATRIX *matrix) = 0;

    // --- Draw Primitives ---
    virtual void DrawTriangleStrip(const VertexDiffuseXyzrwh *verts, i32 count) = 0;
    virtual void DrawTriangleStripTex(const VertexTex1Xyzrwh *verts, i32 count) = 0;
    virtual void DrawTriangleStripTextured(const VertexTex1DiffuseXyzrwh *verts, i32 count) = 0;
    virtual void DrawTriangleStripTextured3D(const VertexTex1DiffuseXyz *verts, i32 count) = 0;
    virtual void DrawTriangleFanTextured(const VertexTex1DiffuseXyzrwh *verts, i32 count) = 0;
    virtual void DrawTriangleFanTextured3D(const VertexTex1DiffuseXyz *verts, i32 count) = 0;
    virtual void DrawVertexBuffer3D(const RenderVertexInfo *verts, i32 count) = 0;

    // --- Texture Management ---
    virtual u32 CreateTextureFromMemory(const u8 *data, i32 dataLen, D3DCOLOR colorKey, i32 *outWidth, i32 *outHeight) = 0;
    virtual u32 CreateEmptyTexture(i32 width, i32 height) = 0;
    virtual void DeleteTexture(u32 tex) = 0;
    virtual void CopyAlphaChannel(u32 dstTex, const u8 *srcData, i32 dataLen, i32 width, i32 height) = 0;
    virtual void UpdateTextureSubImage(u32 tex, i32 x, i32 y, i32 w, i32 h, const u8 *rgbaPixels) = 0;

    // --- Surface Operations ---
    virtual u32 LoadSurfaceFromFile(const u8 *data, i32 dataLen, i32 *outWidth, i32 *outHeight) = 0;
    virtual u32 LoadSurfaceFromFile(const u8 *data, i32 dataLen, D3DXIMAGE_INFO *info) = 0;
    virtual void CopySurfaceToScreen(u32 surfaceTex, i32 srcX, i32 srcY, i32 dstX, i32 dstY, i32 w, i32 h,
                                     i32 texW, i32 texH) = 0;
    virtual void CopySurfaceToScreen(u32 surfaceTex, i32 texW, i32 texH, i32 dstX, i32 dstY) = 0;
    virtual void CopySurfaceRectToScreen(u32 surfaceTex, i32 srcX, i32 srcY, i32 srcW, i32 srcH,
                                         i32 dstX, i32 dstY, i32 texW, i32 texH) = 0;
    virtual void TakeScreenshot(u32 dstTex, i32 left, i32 top, i32 width, i32 height) = 0;

    // --- Public state accessible by game code ---
    SDL_Window *window;
    SDL_GLContext glContext;
    i32 screenWidth;
    i32 screenHeight;
    i32 viewportX, viewportY, viewportW, viewportH;
    u32 currentTexture;
    u8 currentBlendMode;
    u8 currentColorOp;
    u8 currentVertexShader;
    u8 currentZWriteDisable;
    D3DCOLOR textureFactor;
    D3DXMATRIX viewMatrix;
    D3DXMATRIX projectionMatrix;
    i32 fogEnabled;
    D3DCOLOR fogColor;
    f32 fogStart;
    f32 fogEnd;
    i32 realScreenWidth;
    i32 realScreenHeight;
};

// The global renderer pointer. Game code uses g_Renderer->Method().
// Actual backend is selected at initialization time and can be switched at runtime.
extern IRenderer *g_Renderer;

// Access the two static renderer instances.
#ifndef __ANDROID__
IRenderer *GetRendererGL();
#endif
IRenderer *GetRendererGLES();

// Returns true if the current renderer is RendererGLES.
bool IsUsingGLES();

// Hot-switch the active renderer (GLES = shader-based, GL = fixed-function).
// Reinitializes the new backend, preserving the GL context and all textures.
void SwitchRenderer(bool useGLES);

// Refresh the SDL window title with current backend and driver information.
void UpdateWindowTitle();

// Hot-switch between fullscreen and windowed mode without restarting.
void SetWindowMode(bool windowed);

// Request an in-process restart (re-creates window and renderer from current config).
void RequestRestart();

  
