#pragma once

#ifndef NO_SDL
#include <SDL.h>
#endif
#define RENDER_SOFTWARE true

#include "../AnmManager.hpp"
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
    ZunMatrix projection;
    //pre calculate
    bool render2D;
    ZunMatrix mvp;
    f32 vm00;// = viewm00*modelm00 + viewm10*modelm01 + viewm20*modelm02 + viewm30*modelm03;
    f32 vm01;// = viewm01*modelm00 + viewm11*modelm01 + viewm21*modelm02 + viewm31*modelm03;
    f32 vm02;// = viewm02*modelm00 + viewm12*modelm01 + viewm22*modelm02 + viewm32*modelm03;
    f32 vm03;// = viewm03*modelm00 + viewm13*modelm01 + viewm23*modelm02 + viewm33*modelm03;
    f32 vm10;// = viewm00*modelm10 + viewm10*modelm11 + viewm20*modelm12 + viewm30*modelm13;
    f32 vm11;// = viewm01*modelm10 + viewm11*modelm11 + viewm21*modelm12 + viewm31*modelm13;
    f32 vm12;// = viewm02*modelm10 + viewm12*modelm11 + viewm22*modelm12 + viewm32*modelm13;
    f32 vm13;// = viewm03*modelm10 + viewm13*modelm11 + viewm23*modelm12 + viewm33*modelm13;
    f32 vm20;// = viewm00*modelm20 + viewm10*modelm21 + viewm20*modelm22 + viewm30*modelm23;
    f32 vm21;// = viewm01*modelm20 + viewm11*modelm21 + viewm21*modelm22 + viewm31*modelm23;
    f32 vm22;// = viewm02*modelm20 + viewm12*modelm21 + viewm22*modelm22 + viewm32*modelm23;
    f32 vm23;// = viewm03*modelm20 + viewm13*modelm21 + viewm23*modelm22 + viewm33*modelm23;
    f32 vm30;// = viewm00*modelm30 + viewm10*modelm31 + viewm20*modelm32 + viewm30*modelm33;
    f32 vm31;// = viewm01*modelm30 + viewm11*modelm31 + viewm21*modelm32 + viewm31*modelm33;
    f32 vm32;// = viewm02*modelm30 + viewm12*modelm31 + viewm22*modelm32 + viewm32*modelm33;
    f32 vm33;// = viewm03*modelm30 + viewm13*modelm31 + viewm23*modelm32 + viewm33*modelm33;

    f32 mvpm00;// = mvp.m[0][0];
    f32 mvpm01;// = mvp.m[0][1];
    f32 mvpm02;// = mvp.m[0][2];
    f32 mvpm03;// = mvp.m[0][3];

    f32 mvpm10;// = mvp.m[1][0];
    f32 mvpm11;// = mvp.m[1][1];
    f32 mvpm12;// = mvp.m[1][2];
    f32 mvpm13;// = mvp.m[1][3];

    f32 mvpm20;// = mvp.m[2][0];
    f32 mvpm21;// = mvp.m[2][1];
    f32 mvpm22;// = mvp.m[2][2];
    f32 mvpm23;// = mvp.m[2][3];

    f32 mvpm30;// = mvp.m[3][0];
    f32 mvpm31;// = mvp.m[3][1];
    f32 mvpm32;// = mvp.m[3][2];
    f32 mvpm33;// = mvp.m[3][3];

    f32 modelm00, modelm01, modelm02, modelm03;
    f32 modelm10, modelm11, modelm12, modelm13;
    f32 modelm20, modelm21, modelm22, modelm23;
    f32 modelm30, modelm31, modelm32, modelm33;

    f32 viewm00, viewm01, viewm02, viewm03;
    f32 viewm10, viewm11, viewm12, viewm13;
    f32 viewm20, viewm21, viewm22, viewm23;
    f32 viewm30, viewm31, viewm32, viewm33;

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

    void InitFlattenedMatrix(){
        tm00 = tm01 = tm02 =
        tm10 = tm11 = tm12 =
        modelm00 = modelm01 = modelm02 = modelm03 =
        modelm10 = modelm11 = modelm12 = modelm13 =
        modelm20 = modelm21 = modelm22 = modelm23 =
        modelm30 = modelm31 = modelm32 = modelm33 =
        viewm00 = viewm01 = viewm02 = viewm03 =
        viewm10 = viewm11 = viewm12 = viewm13 =
        viewm20 = viewm21 = viewm22 = viewm23 =
        viewm30 = viewm31 = viewm32 = viewm33 = 0.0f;

        tm00 = tm11 = 
        modelm00 = modelm11 = modelm22 = modelm33 = 
        viewm00 = viewm11 = viewm22 = viewm33 = 1.0f;
    }
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