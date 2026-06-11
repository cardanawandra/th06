#pragma once

#include <SDL.h>
#define RENDER_HARDWARE true

#include "AnmManager.hpp"
#include "GfxInterface.hpp"
#include <vector>
#include <memory>

struct GPUTexture {
    SDL_GPUTexture* gpuTexture = nullptr;

    i32 width, height;
    PixelFormat format;
    PixelDataType type;
    inline ZunColor GetPixel(i32 x, i32 y);
};


struct Hardware : GfxInterface
{
    static GfxInterface *Init();
    static void SetContextFlags();
    virtual void Exit();
    ~Hardware() override {
        Exit();
    };

    virtual void SetFogRange(f32 nearPlane, f32 farPlane);
    virtual void SetFogColor(ZunColor color);
    virtual void ToggleVertexAttribute(u8 attr, bool enable);
    virtual void SetAttributePointer(VertexAttributeArrays attr, std::size_t stride, void *ptr);
    virtual void SetColorOp(TextureOpComponent component, ColorOp op);
    virtual void SetTextureFactor(ZunColor factor);
    virtual void SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix);

    virtual void SetTextureFilter();

    virtual void GetViewport(u32* viewport);
    virtual void GetDepthRange(f32* depthRange);
    virtual void SetViewport(i32 x, i32 y, i32 width, i32 height);
    virtual void SetDepthRange(f32 nearPlane, f32 farPlane);

    virtual void Enable(Capabilities cap);
    virtual bool HasError() {return false; };
    virtual void SetBlendMode(BlendMode mode);
    virtual void SetDepthMask(bool enable);
    virtual void SetDepthFunc(DepthFunc func);

    virtual void SetClearDepth(f32 depth);
    virtual void SetClearColor(f32 r, f32 g, f32 b, f32 a);
    virtual void Clear(u32 clearBits);

    virtual GfxTextureHandle CreateTexture();
    virtual void BindTexture(GfxTextureHandle handle);
    virtual void DeleteTexture(GfxTextureHandle handle);
    virtual void SetTextureImage(u32 width, u32 height, PixelFormat fmt, PixelDataType type, const void* data);
    virtual void SetTextureSubImage(i32 xoffset, i32 yoffset, i32 width, i32 height, const void* data);

    virtual void ReadPixels(i32 x, i32 y, i32 width, i32 height, const void* pixels);

    virtual void Draw(PrimitiveType type, i32 start, i32 count);
    virtual void SwapBuffers();

    virtual bool GameLoop();

  private:
    std::vector<u32> freeTextures;
    std::vector<std::unique_ptr<GPUTexture>> textures;
    GPUTexture* boundTexture = nullptr;

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* framebufferTexture;
    SDL_GPUDevice* device = nullptr;
    SDL_GPUGraphicsPipelineCreateInfo info = {};
    SDL_GPUCommandBuffer* cmd = nullptr;
    SDL_GPURenderPass* renderPass = nullptr;
    SDL_GPUGraphicsPipeline* pipeline;
    SDL_GPUBuffer* vertexBuffer;
    SDL_GPUBufferBinding vertexBinding;
    SDL_GPUSampler* gpuSampler;

    u32* framebuffer;
    f32* depthBuffer;

    i32 viewport[4];   //x, y, w, h
    ZunColor clearColor; //r, g, b, a
    f32 clearDepth = 1;
    f32 fogNear;
    f32 fogFar;
    ZunColor fogColor;
    f32 depthNear, depthFar;
    bool depthMask;
    DepthFunc depthFunc;
    bool useDepthTest;

    ZunColor textureFactor;
    BlendMode blendMode;

    ZunMatrix model;
    ZunMatrix view;
    ZunMatrix projection;
    ZunMatrix textureMatrix;

    bool noVertexBuffer;
    bool noFog;
    bool useFragDepth;

    void* vertexData;
    std::size_t vertexStride;
    void* texCoordData;
    std::size_t texCoordStride;
    void* diffuseData;
    std::size_t diffuseStride;

    bool useTexCoord = false;
    bool useDiffuse = false;

    ColorOp colorOp;

    inline ZunVec3 ProjectToNDC(ZunVec3 vertex, ZunMatrix mv, ZunMatrix p, f32 &viewZ, f32 &W);
    inline ZunVec2 ProjectTexCoordToNDC(ZunVec2 texCoord, ZunMatrix textureMatrix);
    inline ZunVec3 NDCToScreen(ZunVec3 vertex);
};