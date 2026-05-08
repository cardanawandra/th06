#pragma once

#include "GfxInterface.hpp"
#include <SDL_opengl.h>

// No shader uniforms needed in fixed-function pipeline

struct WebGL : GfxInterface
{
    static void SetContextFlags();
    static GfxInterface *Create();

    bool Init();

    virtual void SetFogRange(f32 nearPlane, f32 farPlane);
    virtual void SetFogColor(ZunColor color);
    virtual void ToggleVertexAttribute(u8 attr, bool enable);
    virtual void SetAttributePointer(VertexAttributeArrays attr, size_t stride, void *ptr);
    virtual void SetColorOp(TextureOpComponent component, ColorOp op);
    virtual void SetTextureFactor(ZunColor factor);
    virtual void SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix);
    virtual void Draw();

  private:
    // No shaders in SDL1.2 fixed pipeline
};