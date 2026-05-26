#include <SDL.h>
#include "FixedFunctionGL.hpp"
#include "Supervisor.hpp"
#include "GameWindow.hpp"
#include "i18n.hpp"

#include "SDLCompat.hpp"

void FixedFunctionGL::SetContextFlags()
{
    #if SDL_MAJOR_VERSION >= 2
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    #endif
    // SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
}

GfxInterface *FixedFunctionGL::Init()
{
    LOG_COMPAT("FixedFunctionGL::Init 1\n");
    FixedFunctionGL *gfx = new FixedFunctionGL();

    LOG_COMPAT("FixedFunctionGL::Init 2\n");
    SetContextFlags();

    LOG_COMPAT("FixedFunctionGL::Init 3\n");
    SDL_Init(SDL_INIT_VIDEO);
    u32 flags = WINDOW_FLAGS_COMPAT;

    if (g_Supervisor.cfg.windowed == 0)
    {
        flags |= SDL_FULLSCREEN_COMPAT;
    }

    LOG_COMPAT("FixedFunctionGL::Init 4\n");
    g_GameWindow.CONFIGURE_INIT();
    #ifdef __ANDROID__
    GetWindowSize(&g_GameWindow.GAME_WINDOW_WIDTH_REAL,&g_GameWindow.GAME_WINDOW_HEIGHT_REAL);
    #endif
    g_GameWindow.CONFIGURE_VIEW();
    i32 width=g_GameWindow.GAME_WINDOW_WIDTH_REAL;
    i32 height=g_GameWindow.GAME_WINDOW_HEIGHT_REAL;
    i32 x = SDL_WINDOWPOS_UNDEFINED_COMPAT;
    i32 y = SDL_WINDOWPOS_UNDEFINED_COMPAT;

    LOG_COMPAT("FixedFunctionGL::Init 5\n");
    gfx->window = SDL_CreateWindowCompat(TH_WINDOW_TITLE, x, y, width, height, flags);

    LOG_COMPAT("FixedFunctionGL::Init 6\n");
    if (gfx->window == NULL)
    {
        delete gfx;
        return NULL;
    }

    LOG_COMPAT("FixedFunctionGL::Init 7\n");
    gfx->glContext = SDL_GL_CREATE_CONTEXT_COMPAT(gfx->window);

    LOG_COMPAT("FixedFunctionGL::Init 8\n");
    if (gfx->glContext == NULL)
    {
        LOG_COMPAT("g_GameWindow.glContext is null\n");
        delete gfx;
        return NULL;
    }

    LOG_COMPAT("FixedFunctionGL::Init 9\n");
    if (SDL_GL_MAKE_CURRENT_COMPAT(gfx->window, gfx->glContext) != SDL_GL_MAKE_CURRENT_COMPAT_SUCCESS)
    {
        LOG_COMPAT("SDL_GL_MAKE_CURRENT_COMPAT isn't 0\n");
        delete gfx;
        return NULL;
    }

    LOG_COMPAT("FixedFunctionGL::Init 10\n");
    SDL_GL_SET_SWAP_INTERVAL_COMPAT(1);

    LOG_COMPAT("FixedFunctionGL::Init 11\n");
    g_glFuncTable.ResolveFunctions(false);

    g_glFuncTable.glEnable(GL_TEXTURE_2D);
    g_glFuncTable.glEnableClientState(GL_VERTEX_ARRAY);

    g_glFuncTable.glEnable(GL_ALPHA_TEST);
    g_glFuncTable.glAlphaFunc(GL_GEQUAL, 4 / 255.0f);

    LOG_COMPAT("FixedFunctionGL::Init 12\n");
    if (((g_Supervisor.cfg.opts >> GCOS_SUPPRESS_USE_OF_GOROUD_SHADING) & 1) == 1)
    {
        g_glFuncTable.glShadeModel(GL_FLAT);
    }

    if (((g_Supervisor.cfg.opts >> GCOS_DONT_USE_FOG) & 1) == 0)
    {
        g_glFuncTable.glEnable(GL_FOG);
    }

    LOG_COMPAT("FixedFunctionGL::Init 13\n");
    g_glFuncTable.glFogf(GL_FOG_DENSITY, 1.0f);
    g_glFuncTable.glFogf(GL_FOG_MODE, GL_LINEAR);

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

    LOG_COMPAT("FixedFunctionGL::Init 14\n");
    if (((g_Supervisor.cfg.opts >> GCOS_NO_COLOR_COMP) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
    }
    else
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    }

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

    LOG_COMPAT("FixedFunctionGL::Init 15\n");
    if (((g_Supervisor.cfg.opts >> GCOS_DONT_USE_VERTEX_BUF) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_CONSTANT);
    }
    else
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR);
    }

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

    LOG_COMPAT("FixedFunctionGL::Init 16\n");
    if (((g_Supervisor.cfg.opts >> GCOS_NO_COLOR_COMP) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    }
    else
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
    }

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

    LOG_COMPAT("FixedFunctionGL::Init 17\n");
    if (((g_Supervisor.cfg.opts >> GCOS_DONT_USE_VERTEX_BUF) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
    }
    else
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR);
    }

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

    LOG_COMPAT("FixedFunctionGL::Init finish\n");
    return gfx;
}


void FixedFunctionGL::Exit()
{
    if (this->glContext)
    {
        SDL_GL_DELETE_CONTEXT_COMPAT(this->glContext);
        this->glContext = NULL;
    }
    if (this->window)
    {
        SDL_DESTROY_WINDOW_COMPAT(this->window);
        this->window = NULL;
    }
}

void FixedFunctionGL::SetFogRange(f32 nearPlane, f32 farPlane)
{
    g_glFuncTable.glFogf(GL_FOG_START, nearPlane);
    g_glFuncTable.glFogf(GL_FOG_END, farPlane);
}

void FixedFunctionGL::SetFogColor(ZunColor color)
{
    GLfloat normalizedFogColor[4] = {((color >> 16) & 0xFF) / 255.0f, ((color >> 8) & 0xFF) / 255.0f,
                                     (color & 0xFF) / 255.0f, ((color >> 24) & 0xFF) / 255.0f};

    g_glFuncTable.glFogfv(GL_FOG_COLOR, normalizedFogColor);
}

void FixedFunctionGL::ToggleVertexAttribute(u8 attr, bool enable)
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

void FixedFunctionGL::SetAttributePointer(VertexAttributeArrays attr, size_t stride, void *ptr)
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

void FixedFunctionGL::SetColorOp(TextureOpComponent component, ColorOp op)
{
    const GLenum opEnums[3] = {GL_MODULATE, GL_ADD, GL_REPLACE};

    if (component > COMPONENT_ALPHA || op > COLOR_OP_REPLACE)
    {
        return;
    }

    GLenum componentEnum = component == COMPONENT_ALPHA ? GL_COMBINE_ALPHA : GL_COMBINE_RGB;

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, componentEnum, opEnums[op]);
}

void FixedFunctionGL::SetTextureFactor(ZunColor factor)
{
    GLfloat tfactorColor[4] = {((factor >> 16) & 0xFF) / 255.0f, ((factor >> 8) & 0xFF) / 255.0f,
                               (factor & 0xFF) / 255.0f, ((factor >> 24) & 0xFF) / 255.0f};

    g_glFuncTable.glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, tfactorColor);
}

void FixedFunctionGL::SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix)
{
    // This is not going to work for modelview
    GLenum matrixEnum[4] = {GL_MODELVIEW, GL_MODELVIEW, GL_PROJECTION, GL_TEXTURE};

    g_glFuncTable.glMatrixMode(matrixEnum[type]);
    g_glFuncTable.glLoadMatrixf((const GLfloat *)&matrix);
}


void FixedFunctionGL::Enable(Capabilities cap) {
    switch (cap) {
        case CAPS_BLEND:
            g_glFuncTable.glEnable(GL_BLEND);
            break;
        case CAPS_DEPTH_TEST:
            g_glFuncTable.glEnable(GL_DEPTH_TEST);
            break;
    }
}

bool FixedFunctionGL::HasError() {
    return g_glFuncTable.glGetError() != GL_NO_ERROR;
}

void FixedFunctionGL::SetBlendMode(BlendMode mode) {
    if (mode == BLEND_INV_SRC_ALPHA)
    {
        g_glFuncTable.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        g_glFuncTable.glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
}

void FixedFunctionGL::SetViewport(i32 x, i32 y, i32 width, i32 height) {
    g_glFuncTable.glViewport(x, y, width, height);
}

void FixedFunctionGL::GetViewport(u32* viewport) {
    g_glFuncTable.glGetIntegerv(GL_VIEWPORT, (GLint*)viewport);
}

void FixedFunctionGL::GetDepthRange(f32* depthRange) {
    g_glFuncTable.glGetFloatv(GL_DEPTH_RANGE, depthRange);
}

void FixedFunctionGL::SetClearColor(f32 r, f32 g, f32 b, f32 a) {
    g_glFuncTable.glClearColor(r, g, b, a);
}

void FixedFunctionGL::SetTextureFilter() {
    g_glFuncTable.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void FixedFunctionGL::SetClearDepth(f32 depth) {
    g_glFuncTable.glClearDepthf(depth);
}

void FixedFunctionGL::Clear(u32 clearBits) {
    GLbitfield mask = 0;

    if (clearBits & CLEAR_COLOR_BUFFER) mask |= GL_COLOR_BUFFER_BIT;
    if (clearBits & CLEAR_DEPTH_BUFFER) mask |= GL_DEPTH_BUFFER_BIT;

    g_glFuncTable.glClear(mask);
}

void FixedFunctionGL::SetDepthRange(f32 nearPlane, f32 farPlane) {
    g_glFuncTable.GLDepthRangeCompat(nearPlane, farPlane);
}

void FixedFunctionGL::SetDepthMask(bool enable) {
    g_glFuncTable.glDepthMask(enable);
}

void FixedFunctionGL::SetDepthFunc(DepthFunc func) {
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

GfxTextureHandle FixedFunctionGL::CreateTexture() {
    GLuint texture;
    g_glFuncTable.glGenTextures(1, &texture);
    return texture;
}

void FixedFunctionGL::BindTexture(GfxTextureHandle handle) {
    g_glFuncTable.glBindTexture(GL_TEXTURE_2D, handle);
}

void FixedFunctionGL::DeleteTexture(GfxTextureHandle handle) {
    g_glFuncTable.glDeleteTextures(1, (GLuint*)&handle);
}

void FixedFunctionGL::SetTextureImage(u32 width, u32 height, PixelFormat fmt, PixelDataType type, const void* data) {
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

void FixedFunctionGL::SetTextureSubImage(i32 xoffset, i32 yoffset, i32 width, i32 height, const void *data)
{
    g_glFuncTable.glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
}

void FixedFunctionGL::ReadPixels(i32 x, i32 y, i32 width, i32 height, const void* pixels) {
    g_glFuncTable.glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (void*)pixels);
}

void FixedFunctionGL::Draw(PrimitiveType type, i32 start, i32 count)
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

void FixedFunctionGL::SwapBuffers()
{
    SDL_GL_SWAP_COMPAT(window);
}