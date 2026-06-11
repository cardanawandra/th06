#include "FixedFunctionPS2.hpp"
#include "Supervisor.hpp"
#include "GameWindow.hpp"
#include "i18n.hpp"

#include <gl/GL.h>
#include <libpad.h>

void FixedFunctionPS2::SetContextFlags()
{
}

GfxInterface* FixedFunctionPS2::Init()
{
    LOG_COMPAT("FixedFunctionPS2::Init 1\n");

    FixedFunctionPS2* gfx = new FixedFunctionPS2();

    //
    // Initialize PS2GL
    //
    pglInit();

    LOG_COMPAT("FixedFunctionPS2::Init 2\n");

    g_glFuncTable.ResolveFunctions(false);

    LOG_COMPAT("FixedFunctionPS2::Init 3\n");

    g_glFuncTable.glEnable(GL_TEXTURE_2D);
    g_glFuncTable.glEnableClientState(GL_VERTEX_ARRAY);

    g_glFuncTable.glEnable(GL_ALPHA_TEST);
    g_glFuncTable.glAlphaFunc(GL_GEQUAL, 4.0f / 255.0f);

    if (((g_Supervisor.cfg.opts >> GCOS_SUPPRESS_USE_OF_GOROUD_SHADING) & 1) == 1)
    {
        g_glFuncTable.glShadeModel(GL_FLAT);
    }

    if (((g_Supervisor.cfg.opts >> GCOS_DONT_USE_FOG) & 1) == 0)
    {
        g_glFuncTable.glEnable(GL_FOG);
    }

    g_glFuncTable.glFogf(GL_FOG_DENSITY, 1.0f);
    g_glFuncTable.glFogf(GL_FOG_MODE, GL_LINEAR);

    g_glFuncTable.glTexEnvi(
        GL_TEXTURE_ENV,
        GL_TEXTURE_ENV_MODE,
        GL_COMBINE
    );

    if (((g_Supervisor.cfg.opts >> GCOS_NO_COLOR_COMP) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(
            GL_TEXTURE_ENV,
            GL_COMBINE_ALPHA,
            GL_MODULATE
        );
    }
    else
    {
        g_glFuncTable.glTexEnvi(
            GL_TEXTURE_ENV,
            GL_COMBINE_ALPHA,
            GL_REPLACE
        );
    }

    g_glFuncTable.glTexEnvi(
        GL_TEXTURE_ENV,
        GL_OPERAND0_ALPHA,
        GL_SRC_ALPHA
    );

    if (((g_Supervisor.cfg.opts >> GCOS_DONT_USE_VERTEX_BUF) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(
            GL_TEXTURE_ENV,
            GL_SRC1_ALPHA,
            GL_CONSTANT
        );
    }
    else
    {
        g_glFuncTable.glTexEnvi(
            GL_TEXTURE_ENV,
            GL_SRC1_ALPHA,
            GL_PRIMARY_COLOR
        );
    }

    g_glFuncTable.glTexEnvi(
        GL_TEXTURE_ENV,
        GL_OPERAND1_ALPHA,
        GL_SRC_ALPHA
    );

    if (((g_Supervisor.cfg.opts >> GCOS_NO_COLOR_COMP) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(
            GL_TEXTURE_ENV,
            GL_COMBINE_RGB,
            GL_MODULATE
        );
    }
    else
    {
        g_glFuncTable.glTexEnvi(
            GL_TEXTURE_ENV,
            GL_COMBINE_RGB,
            GL_REPLACE
        );
    }

    g_glFuncTable.glTexEnvi(
        GL_TEXTURE_ENV,
        GL_OPERAND0_RGB,
        GL_SRC_COLOR
    );

    if (((g_Supervisor.cfg.opts >> GCOS_DONT_USE_VERTEX_BUF) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(
            GL_TEXTURE_ENV,
            GL_SRC1_RGB,
            GL_CONSTANT
        );
    }
    else
    {
        g_glFuncTable.glTexEnvi(
            GL_TEXTURE_ENV,
            GL_SRC1_RGB,
            GL_PRIMARY_COLOR
        );
    }

    g_glFuncTable.glTexEnvi(
        GL_TEXTURE_ENV,
        GL_OPERAND1_RGB,
        GL_SRC_COLOR
    );

    LOG_COMPAT("FixedFunctionPS2::Init finish\n");

    return gfx;
}

bool FixedFunctionPS2::GameLoop()
{
    padRead(0, 0, &buttons);

    if (!(buttons.btns & PAD_START))
    {
        return false;
    }

    return true;
}


void FixedFunctionPS2::Exit()
{
    pglShutdown();
}

void FixedFunctionPS2::SetFogRange(f32 nearPlane, f32 farPlane)
{
    g_glFuncTable.glFogf(GL_FOG_START, nearPlane);
    g_glFuncTable.glFogf(GL_FOG_END, farPlane);
}

void FixedFunctionPS2::SetFogColor(ZunColor color)
{
    GLfloat normalizedFogColor[4] = {((color >> 16) & 0xFF) / 255.0f, ((color >> 8) & 0xFF) / 255.0f,
                                     (color & 0xFF) / 255.0f, ((color >> 24) & 0xFF) / 255.0f};

    g_glFuncTable.glFogfv(GL_FOG_COLOR, normalizedFogColor);
}

void FixedFunctionPS2::ToggleVertexAttribute(u8 attr, bool enable)
{
    if (attr & VERTEX_ATTR_TEX_COORD)
    {
        // Arg 0 will be the texture is it's used, and diffuse otherwise. Arg 1 will always be diffuse
        if (enable)
        {
            g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
            g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
            g_glFuncTable.glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        else
        {
            g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PRIMARY_COLOR);
            g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PRIMARY_COLOR);
            g_glFuncTable.glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
    }

    if (attr & VERTEX_ATTR_DIFFUSE)
    {
        if (enable)
        {
            g_glFuncTable.glEnableClientState(GL_COLOR_ARRAY);
        }
        else
        {
            g_glFuncTable.glDisableClientState(GL_COLOR_ARRAY);
        }
    }
}

void FixedFunctionPS2::SetAttributePointer(VertexAttributeArrays attr, size_t stride, void *ptr)
{
    switch (attr)
    {
    case VERTEX_ARRAY_POSITION:
        g_glFuncTable.glVertexPointer(3, GL_FLOAT, stride, ptr);
        break;
    case VERTEX_ARRAY_TEX_COORD:
        g_glFuncTable.glTexCoordPointer(2, GL_FLOAT, stride, ptr);
        break;
    case VERTEX_ARRAY_DIFFUSE:
        g_glFuncTable.glColorPointer(4, GL_UNSIGNED_BYTE, stride, ptr);
        break;
    }
}

void FixedFunctionPS2::SetColorOp(TextureOpComponent component, ColorOp op)
{
    const GLenum opEnums[3] = {GL_MODULATE, GL_ADD, GL_REPLACE};

    if (component > COMPONENT_ALPHA || op > COLOR_OP_REPLACE)
    {
        return;
    }

    GLenum componentEnum = component == COMPONENT_ALPHA ? GL_COMBINE_ALPHA : GL_COMBINE_RGB;

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, componentEnum, opEnums[op]);
}

void FixedFunctionPS2::SetTextureFactor(ZunColor factor)
{
    GLfloat tfactorColor[4] = {((factor >> 16) & 0xFF) / 255.0f, ((factor >> 8) & 0xFF) / 255.0f,
                               (factor & 0xFF) / 255.0f, ((factor >> 24) & 0xFF) / 255.0f};

    g_glFuncTable.glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, tfactorColor);
}

void FixedFunctionPS2::SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix)
{
    // This is not going to work for modelview
    GLenum matrixEnum[4] = {GL_MODELVIEW, GL_MODELVIEW, GL_PROJECTION, GL_TEXTURE};

    g_glFuncTable.glMatrixMode(matrixEnum[type]);
    g_glFuncTable.glLoadMatrixf((const GLfloat *)&matrix);
}


void FixedFunctionPS2::Enable(Capabilities cap) {
    switch (cap) {
        case CAPS_BLEND:
            g_glFuncTable.glEnable(GL_BLEND);
            break;
        case CAPS_DEPTH_TEST:
            g_glFuncTable.glEnable(GL_DEPTH_TEST);
            break;
    }
}

bool FixedFunctionPS2::HasError() {
    return g_glFuncTable.glGetError() != GL_NO_ERROR;
}

void FixedFunctionPS2::SetBlendMode(BlendMode mode) {
    if (mode == BLEND_INV_SRC_ALPHA)
    {
        g_glFuncTable.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        g_glFuncTable.glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
}

void FixedFunctionPS2::SetViewport(i32 x, i32 y, i32 width, i32 height) {
    g_glFuncTable.glViewport(x, y, width, height);
}

void FixedFunctionPS2::GetViewport(u32* viewport) {
    g_glFuncTable.glGetIntegerv(GL_VIEWPORT, (GLint*)viewport);
}

void FixedFunctionPS2::GetDepthRange(f32* depthRange) {
    g_glFuncTable.glGetFloatv(GL_DEPTH_RANGE, depthRange);
}

void FixedFunctionPS2::SetClearColor(f32 r, f32 g, f32 b, f32 a) {
    g_glFuncTable.glClearColor(r, g, b, a);
}

void FixedFunctionPS2::SetTextureFilter() {
    g_glFuncTable.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void FixedFunctionPS2::SetClearDepth(f32 depth) {
    g_glFuncTable.GLClearDepthCompat(depth);
}

void FixedFunctionPS2::Clear(u32 clearBits) {
    GLbitfield mask = 0;

    if (clearBits & CLEAR_COLOR_BUFFER) mask |= GL_COLOR_BUFFER_BIT;
    if (clearBits & CLEAR_DEPTH_BUFFER) mask |= GL_DEPTH_BUFFER_BIT;

    g_glFuncTable.glClear(mask);
}

void FixedFunctionPS2::SetDepthRange(f32 nearPlane, f32 farPlane) {
    g_glFuncTable.GLDepthRangeCompat(nearPlane, farPlane);
}

void FixedFunctionPS2::SetDepthMask(bool enable) {
    g_glFuncTable.glDepthMask(enable);
}

void FixedFunctionPS2::SetDepthFunc(DepthFunc func) {
    // This'll end up less awkward once there's a render backend abstraction layer I swear
    if (func == DEPTH_FUNC_ALWAYS)
    {
        g_glFuncTable.glDepthFunc(GL_ALWAYS);
    }
    else
    {
        g_glFuncTable.glDepthFunc(GL_LEQUAL);
    }
}

GfxTextureHandle FixedFunctionPS2::CreateTexture() {
    GLuint texture;
    g_glFuncTable.glGenTextures(1, &texture);
    return texture;
}

void FixedFunctionPS2::BindTexture(GfxTextureHandle handle) {
    g_glFuncTable.glBindTexture(GL_TEXTURE_2D, handle);
}

void FixedFunctionPS2::DeleteTexture(GfxTextureHandle handle) {
    g_glFuncTable.glDeleteTextures(1, (GLuint*)&handle);
}

void FixedFunctionPS2::SetTextureImage(u32 width, u32 height, PixelFormat fmt, PixelDataType type, const void* data) {
    GLenum glFmt;
    GLenum glType;

    switch (fmt)
    {
    case PIXEL_RGBA:
        glFmt = GL_RGBA;
        break;
    case PIXEL_RGB:
        glFmt = GL_RGB;
        break;
    }

    switch (type)
    {
    case PIXEL_UNSIGNED_BYTE:
        glType = GL_UNSIGNED_BYTE;
        break;
    case PIXEL_UNSIGNED_SHORT_5_5_5_1:
        glType = GL_UNSIGNED_SHORT_5_5_5_1;
        break;
    case PIXEL_UNSIGNED_SHORT_5_6_5:
        glType = GL_UNSIGNED_SHORT_5_6_5;
        break;
    case PIXEL_UNSIGNED_SHORT_4_4_4_4:
        glType = GL_UNSIGNED_SHORT_4_4_4_4;
        break;
    }

    g_glFuncTable.glTexImage2D(GL_TEXTURE_2D, 0, glFmt, width, height, 0, glFmt, glType, data);
}

void FixedFunctionPS2::SetTextureSubImage(i32 xoffset, i32 yoffset, i32 width, i32 height, const void *data)
{
    g_glFuncTable.glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
}

void FixedFunctionPS2::ReadPixels(i32 x, i32 y, i32 width, i32 height, const void* pixels) {
    g_glFuncTable.glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (void*)pixels);
}

void FixedFunctionPS2::Draw(PrimitiveType type, i32 start, i32 count)
{
    GLenum glPrim;

    switch (type) {
        case PRIM_TRIANGLE_STRIP:
            glPrim = GL_TRIANGLE_STRIP;
            break;
        case PRIM_TRIANGLES:
            glPrim = GL_TRIANGLES;
            break;
    }

    g_glFuncTable.glDrawArrays(glPrim, start, count);
}

void FixedFunctionPS2::SwapBuffers()
{
    pglSwapBuffers();
}