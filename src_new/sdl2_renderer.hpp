#pragma once
// =============================================================================
// sdl2_renderer.hpp - OpenGL 1.x/2.x fixed-function rendering backend for th06
// Implements IRenderer; legacy code preserved as RendererGL.
// =============================================================================

#include "IRenderer.hpp"
#ifdef __ANDROID__
#include <GLES2/gl2.h>
#ifndef APIENTRY
#define APIENTRY GL_APIENTRY
#endif
#else
#include <SDL_opengl.h>
#endif

// Blend modes matching original D3D8 blend states
enum BlendMode
{
    BLEND_MODE_ALPHA = 0,         // SrcAlpha, InvSrcAlpha
    BLEND_MODE_ADD = 1,           // SrcAlpha, One
    BLEND_MODE_INV_SRC_ALPHA = 0, // alias for ALPHA
    BLEND_MODE_ADDITIVE = 1,      // alias for ADD
};

// Vertex structures matching original FVF formats
struct VertexDiffuseXyzrwh
{
    D3DXVECTOR4 position;
    D3DCOLOR diffuse;
};

struct VertexTex1Xyzrwh
{
    D3DXVECTOR4 position;
    D3DXVECTOR2 textureUV;
};

struct VertexTex1DiffuseXyzrwh
{
    D3DXVECTOR4 position;
    D3DCOLOR diffuse;
    D3DXVECTOR2 textureUV;
};

// struct VertexTex1DiffuseXyz
// {
//     D3DXVECTOR3 position;
//     D3DCOLOR diffuse;
//     D3DXVECTOR2 textureUV;
// };

// RenderVertexInfo is defined in AnmManager.hpp
struct RenderVertexInfo;

// OpenGL 1.x/2.x fixed-function pipeline renderer (original implementation).
struct RendererGL : public IRenderer
{
    // FBO for render-to-texture (fullscreen scaling) — GL-specific handles
    GLuint fbo;
    GLuint fboColorTex;
    GLuint fboDepthRb;

    void Init(SDL_Window *win, SDL_GLContext ctx, i32 w, i32 h) override;
    void InitDevice(u32 opts) override;
    void Release() override;
    void ResizeTarget() override;
    void BeginScene() override;
    void EndScene() override;
    void BeginFrame() override;
    void EndFrame() override;

    void Clear(D3DCOLOR color, i32 clearColor, i32 clearDepth) override;
    void SetViewport(i32 x, i32 y, i32 w, i32 h, f32 minZ = 0.0f, f32 maxZ = 1.0f) override;

    void SetBlendMode(u8 mode) override;
    void SetColorOp(u8 colorOp) override;
    void SetTexture(u32 tex) override;
    void SetTextureFactor(D3DCOLOR factor) override;
    void SetZWriteDisable(u8 disable) override;
    void SetDepthFunc(i32 alwaysPass) override;
    void SetDestBlendInvSrcAlpha() override;
    void SetDestBlendOne() override;
    void SetTextureStageSelectDiffuse() override;
    void SetTextureStageModulateTexture() override;
    void SetFog(i32 enable, D3DCOLOR color, f32 start, f32 end) override;

    void SetViewTransform(const D3DXMATRIX *matrix) override;
    void SetProjectionTransform(const D3DXMATRIX *matrix) override;
    void SetWorldTransform(const D3DXMATRIX *matrix) override;
    void SetTextureTransform(const D3DXMATRIX *matrix) override;

    void DrawTriangleStrip(const VertexDiffuseXyzrwh *verts, i32 count) override;
    void DrawTriangleStripTex(const VertexTex1Xyzrwh *verts, i32 count) override;
    void DrawTriangleStripTextured(const VertexTex1DiffuseXyzrwh *verts, i32 count) override;
    void DrawTriangleStripTextured3D(const VertexTex1DiffuseXyz *verts, i32 count) override;
    void DrawTriangleFanTextured(const VertexTex1DiffuseXyzrwh *verts, i32 count) override;
    void DrawTriangleFanTextured3D(const VertexTex1DiffuseXyz *verts, i32 count) override;
    void DrawVertexBuffer3D(const RenderVertexInfo *verts, i32 count) override;

    u32 CreateTextureFromMemory(const u8 *data, i32 dataLen, D3DCOLOR colorKey, i32 *outWidth, i32 *outHeight) override;
    u32 CreateEmptyTexture(i32 width, i32 height) override;
    void DeleteTexture(u32 tex) override;
    void CopyAlphaChannel(u32 dstTex, const u8 *srcData, i32 dataLen, i32 width, i32 height) override;
    void UpdateTextureSubImage(u32 tex, i32 x, i32 y, i32 w, i32 h, const u8 *rgbaPixels) override;

    u32 LoadSurfaceFromFile(const u8 *data, i32 dataLen, i32 *outWidth, i32 *outHeight) override;
    u32 LoadSurfaceFromFile(const u8 *data, i32 dataLen, D3DXIMAGE_INFO *info) override;
    void CopySurfaceToScreen(u32 surfaceTex, i32 srcX, i32 srcY, i32 dstX, i32 dstY, i32 w, i32 h,
                             i32 texW, i32 texH) override;
    void CopySurfaceToScreen(u32 surfaceTex, i32 texW, i32 texH, i32 dstX, i32 dstY) override;
    void CopySurfaceRectToScreen(u32 surfaceTex, i32 srcX, i32 srcY, i32 srcW, i32 srcH,
                                 i32 dstX, i32 dstY, i32 texW, i32 texH) override;
    void TakeScreenshot(u32 dstTex, i32 left, i32 top, i32 width, i32 height) override;

    // Internal helper
    void BlitFBOToScreen();
};

  
