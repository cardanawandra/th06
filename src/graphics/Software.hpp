#pragma once

#ifndef NO_SDL
#include <SDL.h>
#endif
#define RENDER_SOFTWARE true

#include "AnmManager.hpp"
#include "GfxInterface.hpp"
#include <vector>
#include <memory>
#include <stddef.h>

#define FIXED_ONE 65536.0f
typedef i32 fixed32; // signed 16.16

struct Texture {
    std::vector<u32> texels; //ARGB8888
    std::vector<u8> texR;
    std::vector<u8> texG;
    std::vector<u8> texB;
    std::vector<u8> texA;
    i32 width, height;
    i8 shift;
    PixelFormat format;
    PixelDataType type;
};

//It is extremely recommended that you compile as Release if you want to use the software rasterizer
//because it runs extremely slow on Debug
//(or just straight up avoid using it unless extremely necessary)

inline u8 ZunA(ZunColor c) { return c >> 24; }
inline u8 ZunR(ZunColor c) { return (c >> 16) & 0xFF; }
inline u8 ZunG(ZunColor c) { return (c >> 8) & 0xFF; }
inline u8 ZunB(ZunColor c) { return c & 0xFF; }

struct Diffuse {
    f32 r,g,b,a;
    Diffuse() {}
    Diffuse(f32 r, f32 g, f32 b, f32 a) {
        this->r = r;
        this->g = g;
        this->b = b;
        this->a = a;
    }
    Diffuse(ColorData colorData) {
        this->r = colorData.r;
        this->g = colorData.g;
        this->b = colorData.b;
        this->a = colorData.a;
    }
    Diffuse operator*(const f32 mult) const
    {
        return Diffuse(this->r * mult, this->g * mult, this->b * mult, this->a * mult);
    }

    Diffuse operator*(const Diffuse &mult) const
    {
        return Diffuse(this->r * mult.r, this->g * mult.g, this->b * mult.b, this->a * mult.a);
    }
    Diffuse operator+(const f32 mult) const
    {
        return Diffuse(this->r + mult, this->g + mult, this->b + mult, this->a + mult);
    }

    Diffuse operator+(const Diffuse &mult) const
    {
        return Diffuse(this->r + mult.r, this->g + mult.g, this->b + mult.b, this->a + mult.a);
    }

    Diffuse &operator+=(const Diffuse &b) {
        this->r += b.r;
        this->g += b.g;
        this->b += b.b;
        this->a += b.a;
    
        return *this;
    }
};
struct Software : GfxInterface
{
    static GfxInterface *Init();
    static void SetContextFlags();
    virtual void Exit();
    ~Software() {
        Exit();
    };

    virtual void SetFogRange(f32 nearPlane, f32 farPlane);
    virtual void SetFogColor(ZunColor color);
    virtual void ToggleVertexAttribute(u8 attr, bool enable);
    virtual void SetAttributePointer(VertexAttributeArrays attr, size_t stride, void *ptr);
    virtual void SetColorOp(TextureOpComponent component, ColorOp op);
    virtual void SetTextureFactor(ZunColor factor);
    virtual void SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix);
    virtual void Set2D(bool is2D);

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
    std::vector<Texture> textures;
    std::vector<u32> freeTextures;


    Texture* boundTexture;// = nullptr;

    #ifndef NO_SDL
    #if SDL_MAJOR_VERSION == 1
    SDL_Surface* screen;
    #else
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* framebufferTexture;
    #endif
    #endif
    u32* framebuffer;

    i32 viewport[4];   //x, y, w, h
    //precompute viewport
    f32 screenScaleX;
    f32 screenBiasX;
    f32 screenScaleY;
    f32 screenBiasY;

    ZunColor clearColor; //r, g, b, a
    f32 clearDepth;// = 1;
    f32 fogNear;
    f32 fogFar;
    u8 fogColor_r;
    u8 fogColor_g;
    u8 fogColor_b;
    u8 fogColor_a;
    f32 precompFogScale;
    f32 precompFogBias;
    
    f32 depthNear, depthFar;
    bool depthMask;
    DepthFunc depthFunc;
    bool useDepthTest;

    u8 textureFactor_r;
    u8 textureFactor_g;
    u8 textureFactor_b;
    u8 textureFactor_a;
    BlendMode blendMode;

    ZunMatrix model;
    ZunMatrix view;
    //pre calculate
    bool render2D;
    ZunMatrix mvp;

    ZunMatrix projection;
    ZunMatrix textureMatrix;
    f32 tm00;
    f32 tm01;
    f32 tm02;
    f32 tm10;
    f32 tm11;
    f32 tm12;

    bool noVertexBuffer;
    bool noFog;
    bool useFragDepth;

    void* vertexData;
    size_t vertexStride;
    void* texCoordData;
    size_t texCoordStride;
    void* diffuseData;
    size_t diffuseStride;

    bool useTexCoord;// = false;
    bool useDiffuse;// = false;

    ColorOp colorOp;

    inline ZunVec3 ProjectToNDC(ZunVec3 vertex, ZunMatrix mv, ZunMatrix p, f32 &viewZ, f32 &W);
    inline ZunVec2 ProjectToNDCZunVec2(ZunVec3 vertex, f32 &z);
    inline ZunVec2 ProjectTexCoordToNDC(ZunVec2 texCoord, ZunMatrix textureMatrix);
    inline ZunVec3 NDCToScreen(ZunVec3 vertex);
    inline ZunVec2 NDCToScreenZunVec2(ZunVec2 vertex);
    void ConvertToARGB8888Pitch(
        u32 width,
        u32 height,
        PixelFormat fmt,
        PixelDataType type,
        const void* srcData,
        u32 srcPitchBytes,
        u32* dstData,
        u32 dstPitchBytes);
};