// =============================================================================
// sdl2_renderer.cpp - OpenGL 1.x/2.x fixed-function rendering backend (RendererGL)
// =============================================================================

#include "sdl2_renderer.hpp"
#include "AnmManager.hpp"
#include "Supervisor.hpp"
// #include "thprac_gui_integration.h"
#include <SDL_image.h>
#include <cstring>
#include <vector>

// FBO extension function pointers (loaded at runtime)
typedef void (APIENTRY *PFNGLGENFRAMEBUFFERSEXTPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFNGLDELETEFRAMEBUFFERSEXTPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFNGLBINDFRAMEBUFFEREXTPROC)(GLenum, GLuint);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef void (APIENTRY *PFNGLGENRENDERBUFFERSEXTPROC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFNGLDELETERENDERBUFFERSEXTPROC)(GLsizei, const GLuint*);
typedef void (APIENTRY *PFNGLBINDRENDERBUFFEREXTPROC)(GLenum, GLuint);
typedef void (APIENTRY *PFNGLRENDERBUFFERSTORAGEEXTPROC)(GLenum, GLenum, GLsizei, GLsizei);
typedef void (APIENTRY *PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)(GLenum, GLenum, GLenum, GLuint);
typedef GLenum (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)(GLenum);

static PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT_ = NULL;
static PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT_ = NULL;
static PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT_ = NULL;
static PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT_ = NULL;
static PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT_ = NULL;
static PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT_ = NULL;
static PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT_ = NULL;
static PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT_ = NULL;
static PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT_ = NULL;
static PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT_ = NULL;

#define GL_FRAMEBUFFER_EXT             0x8D40
#define GL_RENDERBUFFER_EXT            0x8D41
#define GL_COLOR_ATTACHMENT0_EXT       0x8CE0
#define GL_DEPTH_ATTACHMENT_EXT        0x8D00
#define GL_DEPTH_COMPONENT16           0x81A5
#define GL_FRAMEBUFFER_COMPLETE_EXT    0x8CD5
#define GL_FRAMEBUFFER_BINDING_EXT     0x8CA6

static bool LoadFBOExtensions()
{
    glGenFramebuffersEXT_ = (PFNGLGENFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glGenFramebuffersEXT");
    glDeleteFramebuffersEXT_ = (PFNGLDELETEFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
    glBindFramebufferEXT_ = (PFNGLBINDFRAMEBUFFEREXTPROC)SDL_GL_GetProcAddress("glBindFramebufferEXT");
    glFramebufferTexture2DEXT_ = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
    glGenRenderbuffersEXT_ = (PFNGLGENRENDERBUFFERSEXTPROC)SDL_GL_GetProcAddress("glGenRenderbuffersEXT");
    glDeleteRenderbuffersEXT_ = (PFNGLDELETERENDERBUFFERSEXTPROC)SDL_GL_GetProcAddress("glDeleteRenderbuffersEXT");
    glBindRenderbufferEXT_ = (PFNGLBINDRENDERBUFFEREXTPROC)SDL_GL_GetProcAddress("glBindRenderbufferEXT");
    glRenderbufferStorageEXT_ = (PFNGLRENDERBUFFERSTORAGEEXTPROC)SDL_GL_GetProcAddress("glRenderbufferStorageEXT");
    glFramebufferRenderbufferEXT_ = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)SDL_GL_GetProcAddress("glFramebufferRenderbufferEXT");
    glCheckFramebufferStatusEXT_ = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)SDL_GL_GetProcAddress("glCheckFramebufferStatusEXT");

    return glGenFramebuffersEXT_ && glBindFramebufferEXT_ && glFramebufferTexture2DEXT_ &&
           glGenRenderbuffersEXT_ && glBindRenderbufferEXT_ && glRenderbufferStorageEXT_ &&
           glFramebufferRenderbufferEXT_ && glCheckFramebufferStatusEXT_;
}

static GLboolean g_PrevDepthTestEnabled = GL_FALSE;
static GLboolean g_PrevDepthMask = GL_TRUE;
static GLint g_PrevScissorBox[4] = {0, 0, 640, 480};

static void UploadRgbaSurface(GLuint tex, SDL_Surface *rgba)
{
    const i32 bytesPerPixel = 4;
    const i32 tightPitch = rgba->w * bytesPerPixel;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (rgba->pitch == tightPitch)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
        return;
    }

    // SDL surfaces may have padded rows; repack to tightly packed RGBA rows for GL upload.
    std::vector<u8> packed((size_t)tightPitch * (size_t)rgba->h);
    const u8 *src = (const u8 *)rgba->pixels;
    for (i32 y = 0; y < rgba->h; ++y)
    {
        memcpy(&packed[(size_t)y * (size_t)tightPitch], src + (size_t)y * (size_t)rgba->pitch, (size_t)tightPitch);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, packed.data());
}

static void ApplySamplerFor2D()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

static void ApplySamplerFor3D()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

// The static RendererGL instance, allocated when this translation unit is linked.
static RendererGL s_RendererGL;

// Accessor for the fixed-function renderer (used by SwitchRenderer).
IRenderer *GetRendererGL() { return &s_RendererGL; }

// The global renderer pointer. Defaults to GLES; can be switched at runtime.
IRenderer *g_Renderer = nullptr;

void RendererGL::Init(SDL_Window *win, SDL_GLContext ctx, i32 w, i32 h)
{
    this->window = win;
    this->glContext = ctx;
    this->screenWidth = w;
    this->screenHeight = h;
    this->currentTexture = 0;
    this->currentBlendMode = 0xff;
    this->currentColorOp = 0xff;
    this->currentVertexShader = 0xff;
    this->currentZWriteDisable = 0xff;
    this->textureFactor = 0xffffffff;
    this->fogEnabled = 0;
    this->fogColor = 0xffa0a0a0;
    this->fogStart = 1000.0f;
    this->fogEnd = 5000.0f;

    // Get actual window/screen size for fullscreen scaling
    i32 drawW, drawH;
    SDL_GL_GetDrawableSize(win, &drawW, &drawH);
    this->realScreenWidth = drawW;
    this->realScreenHeight = drawH;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GEQUAL, 4.0f / 255.0f);
    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_SCISSOR_TEST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    // Match D3D's [0,1] depth range (wined3d does the same).
    glDepthRange(0.0, 1.0);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Create FBO for render-to-texture if screen size differs from game size
    this->fbo = 0;
    this->fboColorTex = 0;
    this->fboDepthRb = 0;
    if (drawW != w || drawH != h)
    {
        if (LoadFBOExtensions())
        {
            // Color texture
            glGenTextures(1, &this->fboColorTex);
            glBindTexture(GL_TEXTURE_2D, this->fboColorTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Depth renderbuffer
            glGenRenderbuffersEXT_(1, &this->fboDepthRb);
            glBindRenderbufferEXT_(GL_RENDERBUFFER_EXT, this->fboDepthRb);
            glRenderbufferStorageEXT_(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT16, w, h);

            // FBO
            glGenFramebuffersEXT_(1, &this->fbo);
            glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, this->fbo);
            glFramebufferTexture2DEXT_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, this->fboColorTex, 0);
            glFramebufferRenderbufferEXT_(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, this->fboDepthRb);

            GLenum status = glCheckFramebufferStatusEXT_(GL_FRAMEBUFFER_EXT);
            if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
            {
                // FBO incomplete — fall back to direct rendering
                glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);
                glDeleteTextures(1, &this->fboColorTex);
                glDeleteRenderbuffersEXT_(1, &this->fboDepthRb);
                glDeleteFramebuffersEXT_(1, &this->fbo);
                this->fbo = 0;
                this->fboColorTex = 0;
                this->fboDepthRb = 0;
            }
            else
            {
                glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    SetViewport(0, 0, w, h);

    // Bind FBO for the first frame
    BeginFrame();
}

void RendererGL::InitDevice(u32 opts)
{
    // opts is the raw g_Supervisor.cfg.opts bitfield
    if (((opts >> GCOS_TURN_OFF_DEPTH_TEST) & 1) == 0)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);
    }

    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (((opts >> GCOS_SUPPRESS_USE_OF_GOROUD_SHADING) & 1) == 0)
    {
        glShadeModel(GL_SMOOTH);
    }
    else
    {
        glShadeModel(GL_FLAT);
    }

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GEQUAL, 4.0f / 255.0f);

    if (((opts >> GCOS_DONT_USE_FOG) & 1) == 0)
    {
        glEnable(GL_FOG);
        this->fogEnabled = 1;
    }
    else
    {
        glDisable(GL_FOG);
        this->fogEnabled = 0;
    }
    glFogi(GL_FOG_MODE, GL_LINEAR);
    f32 fogCol[4] = {0.627f, 0.627f, 0.627f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogCol);
    glFogf(GL_FOG_START, 1000.0f);
    glFogf(GL_FOG_END, 5000.0f);
    glFogf(GL_FOG_DENSITY, 1.0f);

    this->fogColor = 0xffa0a0a0;
    this->fogStart = 1000.0f;
    this->fogEnd = 5000.0f;

    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void RendererGL::Release()
{
    if (this->fbo != 0)
    {
        glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);
        glDeleteTextures(1, &this->fboColorTex);
        glDeleteRenderbuffersEXT_(1, &this->fboDepthRb);
        glDeleteFramebuffersEXT_(1, &this->fbo);
        this->fbo = 0;
        this->fboColorTex = 0;
        this->fboDepthRb = 0;
    }
    this->window = nullptr;
    this->glContext = nullptr;
}

void RendererGL::ResizeTarget()
{
    // Clean up old FBO resources
    if (this->fbo != 0)
    {
        glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);
        glDeleteTextures(1, &this->fboColorTex);
        glDeleteRenderbuffersEXT_(1, &this->fboDepthRb);
        glDeleteFramebuffersEXT_(1, &this->fbo);
        this->fbo = 0;
        this->fboColorTex = 0;
        this->fboDepthRb = 0;
    }

    // Use pre-set realScreenWidth/realScreenHeight (set by caller before this)
    i32 rw = this->realScreenWidth;
    i32 rh = this->realScreenHeight;
    i32 w = this->screenWidth;
    i32 h = this->screenHeight;
    if (rw != w || rh != h)
    {
        if (LoadFBOExtensions())
        {
            glGenTextures(1, &this->fboColorTex);
            glBindTexture(GL_TEXTURE_2D, this->fboColorTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glGenRenderbuffersEXT_(1, &this->fboDepthRb);
            glBindRenderbufferEXT_(GL_RENDERBUFFER_EXT, this->fboDepthRb);
            glRenderbufferStorageEXT_(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT16, w, h);

            glGenFramebuffersEXT_(1, &this->fbo);
            glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, this->fbo);
            glFramebufferTexture2DEXT_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, this->fboColorTex, 0);
            glFramebufferRenderbufferEXT_(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, this->fboDepthRb);

            GLenum status = glCheckFramebufferStatusEXT_(GL_FRAMEBUFFER_EXT);
            if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
            {
                glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);
                glDeleteTextures(1, &this->fboColorTex);
                glDeleteRenderbuffersEXT_(1, &this->fboDepthRb);
                glDeleteFramebuffersEXT_(1, &this->fbo);
                this->fbo = 0;
                this->fboColorTex = 0;
                this->fboDepthRb = 0;
            }
            else
            {
                glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    SetViewport(0, 0, w, h);
    BeginFrame();
}

void RendererGL::BeginScene()
{
    // OpenGL doesn't have explicit begin/end scene
}

void RendererGL::EndScene()
{
    // OpenGL doesn't have explicit begin/end scene
}

void RendererGL::Clear(D3DCOLOR color, i32 clearColor, i32 clearDepth)
{
    GLbitfield mask = 0;
    if (clearColor)
    {
        f32 a = D3DCOLOR_A(color) / 255.0f;
        f32 r = D3DCOLOR_R(color) / 255.0f;
        f32 g = D3DCOLOR_G(color) / 255.0f;
        f32 b = D3DCOLOR_B(color) / 255.0f;
        glClearColor(r, g, b, a);
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if (clearDepth)
        mask |= GL_DEPTH_BUFFER_BIT;
    if (mask)
    {
        // D3D8 Clear ignores write masks — it always clears regardless of
        // D3DRS_ZWRITEENABLE / D3DRS_COLORWRITEENABLE.  OpenGL's glClear
        // respects glDepthMask / glColorMask, so we must temporarily enable
        // them to match D3D8 semantics.
        GLboolean prevDepthMask;
        GLboolean prevColorMask[4];
        glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
        glGetBooleanv(GL_COLOR_WRITEMASK, prevColorMask);
        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClear(mask);
        glDepthMask(prevDepthMask);
        glColorMask(prevColorMask[0], prevColorMask[1], prevColorMask[2], prevColorMask[3]);
    }
}

void RendererGL::SetViewport(i32 x, i32 y, i32 w, i32 h, f32 minZ, f32 maxZ)
{
    this->viewportX = x;
    this->viewportY = y;
    this->viewportW = w;
    this->viewportH = h;
    glViewport(x, this->screenHeight - y - h, w, h);
    glScissor(x, this->screenHeight - y - h, w, h);
    glDepthRange(minZ, maxZ);
}

void RendererGL::SetBlendMode(u8 mode)
{
    if (mode == this->currentBlendMode)
        return;
    this->currentBlendMode = mode;
    if (mode == BLEND_MODE_ADD)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
    else
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

void RendererGL::SetTexture(GLuint tex)
{
    if (tex == this->currentTexture)
        return;
    this->currentTexture = tex;
    if (tex != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void RendererGL::SetTextureFactor(D3DCOLOR factor)
{
    this->textureFactor = factor;
}

void RendererGL::SetZWriteDisable(u8 disable)
{
    if (disable == this->currentZWriteDisable)
        return;
    this->currentZWriteDisable = disable;
    glDepthMask(disable ? GL_FALSE : GL_TRUE);
}

void RendererGL::SetDepthFunc(i32 alwaysPass)
{
    glDepthFunc(alwaysPass ? GL_ALWAYS : GL_LEQUAL);
}

void RendererGL::SetDestBlendInvSrcAlpha()
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RendererGL::SetDestBlendOne()
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void RendererGL::SetTextureStageSelectDiffuse()
{
    glDisable(GL_TEXTURE_2D);
    this->currentTexture = 0;
}

void RendererGL::SetTextureStageModulateTexture()
{
    glEnable(GL_TEXTURE_2D);
}

void RendererGL::SetFog(i32 enable, D3DCOLOR color, f32 start, f32 end)
{
    this->fogEnabled = enable;
    this->fogColor = color;
    this->fogStart = start;
    this->fogEnd = end;
    if (enable)
    {
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        f32 fogCol[4] = {
            D3DCOLOR_R(color) / 255.0f,
            D3DCOLOR_G(color) / 255.0f,
            D3DCOLOR_B(color) / 255.0f,
            D3DCOLOR_A(color) / 255.0f
        };
        glFogfv(GL_FOG_COLOR, fogCol);
        glFogf(GL_FOG_START, start);
        glFogf(GL_FOG_END, end);
    }
    else
    {
        glDisable(GL_FOG);
    }
}

static void ApplyVertexColor(D3DCOLOR c)
{
    glColor4ub(D3DCOLOR_R(c), D3DCOLOR_G(c), D3DCOLOR_B(c), D3DCOLOR_A(c));
}

static void Begin2DDraw(RendererGL *r)
{
    g_PrevDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &g_PrevDepthMask);
    glGetIntegerv(GL_SCISSOR_BOX, g_PrevScissorBox);

    // Pre-transformed (XYZRHW) vertices are in full screen-space coordinates.
    // Expand viewport to full screen so the ortho projection maps screen-space
    // coordinates correctly. Keep scissor at the current viewport rect (set by
    // SetViewport) because D3D8 clips XYZRHW vertices to the viewport rect.
    // This prevents 2D sprites from gameplay managers from leaking into the
    // HUD panel area.
    glViewport(0, 0, r->screenWidth, r->screenHeight);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // 2D sprite path should not inherit texture transforms from 3D/effect passes.
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // Map D3D transformed z in [0,1] to OpenGL depth the same way (0 near, 1 far).
    glOrtho(0, r->screenWidth, r->screenHeight, 0, 0, -1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if (r->currentTexture != 0)
    {
        glBindTexture(GL_TEXTURE_2D, r->currentTexture);
        ApplySamplerFor2D();
    }
}

static void End2DDraw(RendererGL *r)
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_TEXTURE);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    // Restore viewport and scissor to the values set by SetViewport.
    glViewport(r->viewportX, r->screenHeight - r->viewportY - r->viewportH, r->viewportW, r->viewportH);
    glScissor(g_PrevScissorBox[0], g_PrevScissorBox[1], g_PrevScissorBox[2], g_PrevScissorBox[3]);

    if (g_PrevDepthTestEnabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(g_PrevDepthMask);
}

void RendererGL::DrawTriangleStrip(const VertexDiffuseXyzrwh *verts, i32 count)
{
    glDisable(GL_TEXTURE_2D);
    Begin2DDraw(this);

    glBegin(GL_TRIANGLE_STRIP);
    for (i32 i = 0; i < count; i++)
    {
        ApplyVertexColor(verts[i].diffuse);
        // XYZRHW vertices are already in screen space — use (x, y, z) directly.
        // The w component (1/W) is for perspective-correct interpolation only;
        // wined3d does not divide positions by w for pre-transformed vertices.
        glVertex3f(verts[i].position.x, verts[i].position.y, verts[i].position.z);
    }
    glEnd();

    End2DDraw(this);
    if (this->currentTexture != 0)
        glEnable(GL_TEXTURE_2D);
}

void RendererGL::DrawTriangleStripTextured(const VertexTex1DiffuseXyzrwh *verts, i32 count)
{
    Begin2DDraw(this);

    glBegin(GL_TRIANGLE_STRIP);
    for (i32 i = 0; i < count; i++)
    {
        ApplyVertexColor(verts[i].diffuse);
        glTexCoord2f(verts[i].textureUV.x, verts[i].textureUV.y);
        f32 x = verts[i].position.x;
        f32 y = verts[i].position.y;
        f32 z = verts[i].position.z;
        glVertex3f(x, y, z);
    }
    glEnd();

    End2DDraw(this);
}

void RendererGL::DrawTriangleStripTextured3D(const VertexTex1DiffuseXyz *verts, i32 count)
{
    // Apply view/projection matrices
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(&this->projectionMatrix.m[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(&this->viewMatrix.m[0][0]);

    glBegin(GL_TRIANGLE_STRIP);
    if (this->currentTexture != 0)
    {
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
        ApplySamplerFor3D();
    }
    for (i32 i = 0; i < count; i++)
    {
        // ApplyVertexColor(verts[i].diffuse);
        glTexCoord2f(verts[i].textureUV.x, verts[i].textureUV.y);
        glVertex3f(verts[i].position.x, verts[i].position.y, verts[i].position.z);
    }
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void RendererGL::DrawTriangleFanTextured(const VertexTex1DiffuseXyzrwh *verts, i32 count)
{
    Begin2DDraw(this);

    glBegin(GL_TRIANGLE_FAN);
    for (i32 i = 0; i < count; i++)
    {
        ApplyVertexColor(verts[i].diffuse);
        glTexCoord2f(verts[i].textureUV.x, verts[i].textureUV.y);
        f32 x = verts[i].position.x;
        f32 y = verts[i].position.y;
        f32 z = verts[i].position.z;
        glVertex3f(x, y, z);
    }
    glEnd();

    End2DDraw(this);
}

void RendererGL::DrawTriangleFanTextured3D(const VertexTex1DiffuseXyz *verts, i32 count)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(&this->projectionMatrix.m[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(&this->viewMatrix.m[0][0]);

    glBegin(GL_TRIANGLE_FAN);
    if (this->currentTexture != 0)
    {
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
        ApplySamplerFor3D();
    }
    for (i32 i = 0; i < count; i++)
    {
        // ApplyVertexColor(verts[i].diffuse);
        glTexCoord2f(verts[i].textureUV.x, verts[i].textureUV.y);
        glVertex3f(verts[i].position.x, verts[i].position.y, verts[i].position.z);
    }
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

GLuint RendererGL::CreateTextureFromMemory(const u8 *data, i32 dataLen, D3DCOLOR colorKey, i32 *outWidth, i32 *outHeight)
{
    SDL_RWops *rw = SDL_RWFromConstMem(data, dataLen);
    if (!rw) return 0;

    SDL_Surface *surface = IMG_Load_RW(rw, 1);
    if (!surface) return 0;

    // Convert to RGBA
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    if (!rgba) return 0;

    // Apply color key transparency
    if (colorKey != 0)
    {
        u8 ckR = D3DCOLOR_R(colorKey);
        u8 ckG = D3DCOLOR_G(colorKey);
        u8 ckB = D3DCOLOR_B(colorKey);
        SDL_LockSurface(rgba);
        u8 *pixels = (u8 *)rgba->pixels;
        i32 pixelCount = rgba->w * rgba->h;
        for (i32 i = 0; i < pixelCount; i++)
        {
            u8 *p = pixels + i * 4;
            if (p[0] == ckR && p[1] == ckG && p[2] == ckB)
            {
                p[3] = 0;
            }
        }
        SDL_UnlockSurface(rgba);
    }

    if (outWidth) *outWidth = rgba->w;
    if (outHeight) *outHeight = rgba->h;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    UploadRgbaSurface(tex, rgba);

    SDL_FreeSurface(rgba);

    // Restore current texture binding
    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);

    return tex;
}

GLuint RendererGL::CreateEmptyTexture(i32 width, i32 height)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);

    return tex;
}

void RendererGL::DeleteTexture(GLuint tex)
{
    if (tex != 0)
    {
        if (this->currentTexture == tex)
        {
            this->currentTexture = 0;
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glDeleteTextures(1, &tex);
    }
}

void RendererGL::SetColorOp(u8 colorOp)
{
    if (colorOp == this->currentColorOp)
        return;
    this->currentColorOp = colorOp;
    if (colorOp == 0) // Modulate
    {
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    else // Add
    {
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
    }
}

void RendererGL::SetViewTransform(const D3DXMATRIX *matrix)
{
    this->viewMatrix = *matrix;
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&this->viewMatrix.m[0][0]);
}

void RendererGL::SetProjectionTransform(const D3DXMATRIX *matrix)
{
    this->projectionMatrix = *matrix;
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&this->projectionMatrix.m[0][0]);
    glMatrixMode(GL_MODELVIEW);
}

void RendererGL::SetWorldTransform(const D3DXMATRIX *matrix)
{
    glMatrixMode(GL_MODELVIEW);
    D3DXMATRIX combined;
    D3DXMatrixMultiply(&combined, matrix, &this->viewMatrix);
    glLoadMatrixf(&combined.m[0][0]);
}

void RendererGL::SetTextureTransform(const D3DXMATRIX *matrix)
{
    // D3D uses row-vector * matrix (row-major), GL uses matrix * column-vector (column-major).
    // glLoadMatrixf reads the row-major D3D data as column-major, which naturally transposes
    // the matrix — this is exactly the transpose needed for the convention change.
    // (Same approach as wined3d: pass D3D matrices directly to GL.)
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(&matrix->m[0][0]);
}

GLuint RendererGL::LoadSurfaceFromFile(const u8 *data, i32 dataLen, i32 *outWidth, i32 *outHeight)
{
    SDL_RWops *rw = SDL_RWFromConstMem(data, dataLen);
    if (!rw) return 0;
    SDL_Surface *surface = IMG_Load_RW(rw, 1);
    if (!surface) return 0;

    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    if (!rgba) return 0;

    if (outWidth) *outWidth = rgba->w;
    if (outHeight) *outHeight = rgba->h;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    UploadRgbaSurface(tex, rgba);

    SDL_FreeSurface(rgba);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);

    return tex;
}

void RendererGL::CopySurfaceToScreen(GLuint surfaceTex, i32 srcX, i32 srcY, i32 dstX, i32 dstY, i32 w, i32 h,
                                       i32 texW, i32 texH)
{
    if (surfaceTex == 0) return;

    f32 u0 = (f32)srcX / texW;
    f32 v0 = (f32)srcY / texH;
    f32 u1 = (f32)(srcX + (w > 0 ? w : texW)) / texW;
    f32 v1 = (f32)(srcY + (h > 0 ? h : texH)) / texH;

    i32 drawW = w > 0 ? w : texW;
    i32 drawH = h > 0 ? h : texH;

    Begin2DDraw(this);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, surfaceTex);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(u0, v0); glVertex2f((f32)dstX, (f32)dstY);
    glTexCoord2f(u1, v0); glVertex2f((f32)(dstX + drawW), (f32)dstY);
    glTexCoord2f(u0, v1); glVertex2f((f32)dstX, (f32)(dstY + drawH));
    glTexCoord2f(u1, v1); glVertex2f((f32)(dstX + drawW), (f32)(dstY + drawH));
    glEnd();

    End2DDraw(this);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
}

void RendererGL::CopySurfaceRectToScreen(GLuint surfaceTex, i32 srcX, i32 srcY, i32 srcW, i32 srcH,
                                           i32 dstX, i32 dstY, i32 texW, i32 texH)
{
    if (surfaceTex == 0) return;

    f32 u0 = (f32)srcX / texW;
    f32 v0 = (f32)srcY / texH;
    f32 u1 = (f32)(srcX + srcW) / texW;
    f32 v1 = (f32)(srcY + srcH) / texH;

    Begin2DDraw(this);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, surfaceTex);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(u0, v0); glVertex2f((f32)dstX, (f32)dstY);
    glTexCoord2f(u1, v0); glVertex2f((f32)(dstX + srcW), (f32)dstY);
    glTexCoord2f(u0, v1); glVertex2f((f32)dstX, (f32)(dstY + srcH));
    glTexCoord2f(u1, v1); glVertex2f((f32)(dstX + srcW), (f32)(dstY + srcH));
    glEnd();

    End2DDraw(this);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
}

void RendererGL::TakeScreenshot(GLuint dstTex, i32 left, i32 top, i32 width, i32 height)
{
    if (dstTex == 0) return;

    // When using FBO, read from the FBO (640×480 game resolution) instead of
    // the default framebuffer (real screen resolution with letterbox).  The
    // FBO content persists after EndFrame/SwapWindow, so coordinates in game
    // space are correct.  Without this, glReadPixels would read from the
    // wrong position on the real-resolution default framebuffer, producing
    // corrupted pause-menu backgrounds with a blue color shift.
    GLint prevFbo = 0;
    if (this->fbo != 0)
    {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &prevFbo);
        glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, this->fbo);
    }

    u8 *pixels = new u8[width * height * 4];
    glReadPixels(left, this->screenHeight - top - height, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Flip vertically (OpenGL reads bottom-up)
    u8 *flipped = new u8[width * height * 4];
    for (i32 y = 0; y < height; y++)
    {
        memcpy(flipped + y * width * 4, pixels + (height - 1 - y) * width * 4, width * 4);
    }

    glBindTexture(GL_TEXTURE_2D, dstTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, flipped);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);

    // Restore previous framebuffer binding
    if (this->fbo != 0)
        glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, prevFbo);

    delete[] pixels;
    delete[] flipped;
}

void RendererGL::CopyAlphaChannel(GLuint dstTex, const u8 *srcData, i32 dataLen, i32 width, i32 height)
{
    if (dstTex == 0) return;

    SDL_RWops *rw = SDL_RWFromConstMem(srcData, dataLen);
    if (!rw) return;
    SDL_Surface *srcSurface = IMG_Load_RW(rw, 1);
    if (!srcSurface) return;

    SDL_Surface *srcRgba = SDL_ConvertSurfaceFormat(srcSurface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(srcSurface);
    if (!srcRgba) return;

    glBindTexture(GL_TEXTURE_2D, dstTex);

    // Query actual texture dimensions if caller passed 0
    if (width <= 0 || height <= 0)
    {
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    }
    if (width <= 0 || height <= 0)
    {
        SDL_FreeSurface(srcRgba);
        return;
    }

    // Read current texture pixels
    u8 *dstPixels = new u8[width * height * 4];
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dstPixels);

    // Copy blue channel of source as alpha channel of destination
    i32 copyW = srcRgba->w < width ? srcRgba->w : width;
    i32 copyH = srcRgba->h < height ? srcRgba->h : height;
    SDL_LockSurface(srcRgba);
    for (i32 y = 0; y < copyH; y++)
    {
        u8 *src = (u8 *)srcRgba->pixels + y * srcRgba->pitch;
        u8 *dst = dstPixels + y * width * 4;
        for (i32 x = 0; x < copyW; x++)
        {
            dst[x * 4 + 3] = src[x * 4 + 0]; // blue → alpha
        }
    }
    SDL_UnlockSurface(srcRgba);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dstPixels);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);

    delete[] dstPixels;
    SDL_FreeSurface(srcRgba);
}

void RendererGL::UpdateTextureSubImage(GLuint tex, i32 x, i32 y, i32 w, i32 h, const u8 *rgbaPixels)
{
    if (tex == 0 || !rgbaPixels) return;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels);
    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
}

void RendererGL::DrawTriangleStripTex(const VertexTex1Xyzrwh *verts, i32 count)
{
    Begin2DDraw(this);

    // Use texture factor as vertex color
    ApplyVertexColor(this->textureFactor);

    glBegin(GL_TRIANGLE_STRIP);
    for (i32 i = 0; i < count; i++)
    {
        glTexCoord2f(verts[i].textureUV.x, verts[i].textureUV.y);
        f32 x = verts[i].position.x;
        f32 y = verts[i].position.y;
        f32 z = verts[i].position.z;
        glVertex3f(x, y, z);
    }
    glEnd();

    End2DDraw(this);
}

void RendererGL::DrawVertexBuffer3D(const RenderVertexInfo *verts, i32 count)
{
    // Uses currently set world transform and texture transform
    // Projection already setup via glMatrixMode calls

    ApplyVertexColor(this->textureFactor);

    if (this->currentTexture != 0)
    {
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
        ApplySamplerFor3D();
    }

    glBegin(GL_TRIANGLE_STRIP);
    for (i32 i = 0; i < count; i++)
    {
        glTexCoord2f(verts[i].textureUV.x, verts[i].textureUV.y);
        glVertex3f(verts[i].position.x, verts[i].position.y, verts[i].position.z);
    }
    glEnd();
}

GLuint RendererGL::LoadSurfaceFromFile(const u8 *data, i32 dataLen, D3DXIMAGE_INFO *info)
{
    i32 w = 0, h = 0;
    GLuint result = LoadSurfaceFromFile(data, dataLen, &w, &h);
    if (info)
    {
        info->Width = (u32)w;
        info->Height = (u32)h;
    }
    return result;
}

void RendererGL::CopySurfaceToScreen(GLuint surfaceTex, i32 texW, i32 texH, i32 dstX, i32 dstY)
{
    CopySurfaceToScreen(surfaceTex, 0, 0, dstX, dstY, texW, texH, texW, texH);
}

void RendererGL::BeginFrame()
{
    if (this->fbo != 0)
    {
        glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, this->fbo);

        // Restore viewport/scissor to the values from the last SetViewport() call,
        // not full screen. D3D8's Present() does not alter render state, and the
        // game depends on CalcChain-set viewport (gameplay area 32,16,384,448)
        // persisting into the next frame's DrawChain to clip Stage's 3D background.
        glViewport(this->viewportX, this->screenHeight - this->viewportY - this->viewportH,
                   this->viewportW, this->viewportH);
        glScissor(this->viewportX, this->screenHeight - this->viewportY - this->viewportH,
                  this->viewportW, this->viewportH);
    }
}

// Blit the FBO color texture to the default framebuffer (screen) with
// letterbox/pillarbox scaling.  Called twice per frame: once before swap
// (for the front buffer) and once after swap (so the back buffer also
// holds the current frame — this is the root cause fix for screen capture
// tools reading stale data from the post-swap back buffer).
void RendererGL::BlitFBOToScreen()
{
    glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);

    i32 rw = this->realScreenWidth;
    i32 rh = this->realScreenHeight;

    // Clear to black with alpha=1 so DWM composites as fully opaque.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, rw, rh);
    glScissor(0, 0, rw, rh);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Compute centered 4:3 letterbox/pillarbox
    i32 scaledW, scaledH;
    if (rw * this->screenHeight > rh * this->screenWidth)
    {
        scaledH = rh;
        scaledW = rh * this->screenWidth / this->screenHeight;
    }
    else
    {
        scaledW = rw;
        scaledH = rw * this->screenHeight / this->screenWidth;
    }
    i32 offsetX = (rw - scaledW) / 2;
    i32 offsetY = (rh - scaledH) / 2;

    glViewport(offsetX, offsetY, scaledW, scaledH);
    glScissor(offsetX, offsetY, scaledW, scaledH);

    // Block alpha writes so the default framebuffer keeps alpha=1 from the
    // clear above — prevents DWM from compositing the window as transparent
    // when the FBO has alpha=0 areas (fog/sky clear).
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_FOG);
    glDisable(GL_SCISSOR_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, this->fboColorTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f(1, 0);
    glTexCoord2f(0, 1); glVertex2f(0, 1);
    glTexCoord2f(1, 1); glVertex2f(1, 1);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
}

void RendererGL::EndFrame()
{
    if (this->fbo != 0)
    {
        // Save the game's entire GL state so it persists unchanged across frames.
        // EndFrame modifies viewport, depth/color masks, blend, scissor, matrices,
        // etc. for ImGui rendering and the FBO-to-screen blit.  Without this
        // push/pop the cached render-state flags (currentZWriteDisable,
        // currentBlendMode, …) would desync from actual GL state, causing the
        // dirty-flag optimisation to skip necessary state changes next frame.
        glPushAttrib(GL_ALL_ATTRIB_BITS);

        // Render thprac ImGui overlay into the FBO at game resolution.
        glViewport(0, 0, this->screenWidth, this->screenHeight);
        glScissor(0, 0, this->screenWidth, this->screenHeight);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_FOG);
        // THPrac::THPracGuiRender();

        // Blit FBO → front buffer (via swap), then re-blit → back buffer.
        // Root cause fix: after SwapWindow the back buffer holds stale frame
        // N-1 data. Screen capture tools that read the back buffer (e.g. OBS
        // Game Capture hooking SwapBuffers post-swap, or Desktop Duplication)
        // would see an old frame. By immediately re-blitting the FBO texture
        // to the new back buffer, both front and back buffers hold the current
        // frame, eliminating stale-frame capture regardless of when the tool
        // samples the buffer.
        BlitFBOToScreen();
        SDL_GL_SwapWindow(this->window);
        BlitFBOToScreen();

        // Restore the game's GL state saved at the top of EndFrame.
        // FBO binding is NOT part of the attrib stack, so BeginFrame will
        // re-bind the FBO as usual.  All other state (depth mask, color mask,
        // blend mode, scissor, viewport, matrices, etc.) is restored here.
        glPopAttrib();
    }
    else
    {
        // Non-FBO path: render ImGui directly to the screen
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_FOG);
        // THPrac::THPracGuiRender();
        SDL_GL_SwapWindow(this->window);
    }
}

  
