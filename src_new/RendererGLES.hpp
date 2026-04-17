#pragma once
// =============================================================================
// RendererGLES.hpp - OpenGL ES 2.0 rendering backend for th06
// Implements IRenderer using shaders + VBO instead of fixed-function pipeline.
// =============================================================================

#include "IRenderer.hpp"
#include "sdl2_renderer.hpp" // for vertex structs, BlendMode enum
#include <vector>
#ifdef __ANDROID__
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#ifndef APIENTRY
#define APIENTRY GL_APIENTRY
#endif
#else
#include <SDL_opengl.h>
#endif


struct RendererGLES : public IRenderer
{
    // Shader program
    GLuint shaderProgram;
    GLint loc_a_Position;
    GLint loc_a_Color;
    GLint loc_a_TexCoord;
    GLint loc_u_MVP;
    GLint loc_u_TexMatrix;
    GLint loc_u_ModelView;
    GLint loc_u_Texture;
    GLint loc_u_TextureEnabled;
    GLint loc_u_ColorOp;
    GLint loc_u_AlphaRef;
    GLint loc_u_FogEnabled;
    GLint loc_u_FogStart;
    GLint loc_u_FogEnd;
    GLint loc_u_FogColor;

    // CPU-side matrix state (replaces GL matrix stack)
    D3DXMATRIX modelviewMatrix;   // view * world
    D3DXMATRIX worldMatrix;
    D3DXMATRIX textureMatrix;

    // Current state tracking
    u8 textureEnabled;

    // FBO (GLES core — not EXT)
    GLuint fbo;
    GLuint fboColorTex;
    GLuint fboDepthRb;

    // Dynamic VBO for streaming vertices
    GLuint vbo;

    // Reusable CPU-side interleaved vertex staging buffer
    std::vector<f32> drawScratch;
    bool attribsEnabled;

    // --- IRenderer interface ---
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

    // Internal helpers
    void BlitFBOToScreen();
    void UploadUniforms();
    void UploadMVP();
    void DrawArrays(GLenum mode, const f32 *positions, const f32 *colors, const f32 *texcoords, i32 vertCount);
};

  
