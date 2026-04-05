// =============================================================================
// RendererGLES.cpp - OpenGL ES 2.0 / GL 2.0+ shader-based rendering backend
// Implements IRenderer using shaders + VBO. No fixed-function calls.
// =============================================================================

#include "RendererGLES.hpp"
#include "AnmManager.hpp"
#include "Supervisor.hpp"
#include "thprac_gui_integration.h"
#include "gles_shaders.h"
#include <SDL_image.h>
#include <cstring>
#include <cstdio>

// ---------------------------------------------------------------------------
// GL 2.0 shader / FBO function pointers (loaded via SDL_GL_GetProcAddress)
// On desktop GL these are extensions; on real GLES 2.0 they are core.
// We load them at runtime so the same binary works on both paths.
// ---------------------------------------------------------------------------
typedef GLuint (APIENTRY *PFN_glCreateShader)(GLenum);
typedef void   (APIENTRY *PFN_glShaderSource)(GLuint, GLsizei, const GLchar *const *, const GLint *);
typedef void   (APIENTRY *PFN_glCompileShader)(GLuint);
typedef void   (APIENTRY *PFN_glGetShaderiv)(GLuint, GLenum, GLint *);
typedef void   (APIENTRY *PFN_glGetShaderInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
typedef GLuint (APIENTRY *PFN_glCreateProgram)(void);
typedef void   (APIENTRY *PFN_glAttachShader)(GLuint, GLuint);
typedef void   (APIENTRY *PFN_glLinkProgram)(GLuint);
typedef void   (APIENTRY *PFN_glGetProgramiv)(GLuint, GLenum, GLint *);
typedef void   (APIENTRY *PFN_glGetProgramInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
typedef void   (APIENTRY *PFN_glUseProgram)(GLuint);
typedef GLint  (APIENTRY *PFN_glGetAttribLocation)(GLuint, const GLchar *);
typedef GLint  (APIENTRY *PFN_glGetUniformLocation)(GLuint, const GLchar *);
typedef void   (APIENTRY *PFN_glUniform1i)(GLint, GLint);
typedef void   (APIENTRY *PFN_glUniform1f)(GLint, GLfloat);
typedef void   (APIENTRY *PFN_glUniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void   (APIENTRY *PFN_glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat *);
typedef void   (APIENTRY *PFN_glEnableVertexAttribArray)(GLuint);
typedef void   (APIENTRY *PFN_glDisableVertexAttribArray)(GLuint);
typedef void   (APIENTRY *PFN_glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);
typedef void   (APIENTRY *PFN_glDeleteShader)(GLuint);
typedef void   (APIENTRY *PFN_glDeleteProgram)(GLuint);
typedef void   (APIENTRY *PFN_glGenBuffers)(GLsizei, GLuint *);
typedef void   (APIENTRY *PFN_glDeleteBuffers)(GLsizei, const GLuint *);
typedef void   (APIENTRY *PFN_glBindBuffer)(GLenum, GLuint);
typedef void   (APIENTRY *PFN_glBufferData)(GLenum, GLsizeiptr, const void *, GLenum);
// FBO (core in GL 3.0 / GLES 2.0, ARB extension on GL 2.x desktop)
typedef void   (APIENTRY *PFN_glGenFramebuffers)(GLsizei, GLuint *);
typedef void   (APIENTRY *PFN_glDeleteFramebuffers)(GLsizei, const GLuint *);
typedef void   (APIENTRY *PFN_glBindFramebuffer)(GLenum, GLuint);
typedef void   (APIENTRY *PFN_glFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef void   (APIENTRY *PFN_glGenRenderbuffers)(GLsizei, GLuint *);
typedef void   (APIENTRY *PFN_glDeleteRenderbuffers)(GLsizei, const GLuint *);
typedef void   (APIENTRY *PFN_glBindRenderbuffer)(GLenum, GLuint);
typedef void   (APIENTRY *PFN_glRenderbufferStorage)(GLenum, GLenum, GLsizei, GLsizei);
typedef void   (APIENTRY *PFN_glFramebufferRenderbuffer)(GLenum, GLenum, GLenum, GLuint);
typedef GLenum (APIENTRY *PFN_glCheckFramebufferStatus)(GLenum);

static PFN_glCreateShader            pglCreateShader;
static PFN_glShaderSource            pglShaderSource;
static PFN_glCompileShader           pglCompileShader;
static PFN_glGetShaderiv             pglGetShaderiv;
static PFN_glGetShaderInfoLog        pglGetShaderInfoLog;
static PFN_glCreateProgram           pglCreateProgram;
static PFN_glAttachShader            pglAttachShader;
static PFN_glLinkProgram             pglLinkProgram;
static PFN_glGetProgramiv            pglGetProgramiv;
static PFN_glGetProgramInfoLog       pglGetProgramInfoLog;
static PFN_glUseProgram              pglUseProgram;
static PFN_glGetAttribLocation       pglGetAttribLocation;
static PFN_glGetUniformLocation      pglGetUniformLocation;
static PFN_glUniform1i               pglUniform1i;
static PFN_glUniform1f               pglUniform1f;
static PFN_glUniform4f               pglUniform4f;
static PFN_glUniformMatrix4fv        pglUniformMatrix4fv;
static PFN_glEnableVertexAttribArray  pglEnableVertexAttribArray;
static PFN_glDisableVertexAttribArray pglDisableVertexAttribArray;
static PFN_glVertexAttribPointer     pglVertexAttribPointer;
static PFN_glDeleteShader            pglDeleteShader;
static PFN_glDeleteProgram           pglDeleteProgram;
static PFN_glGenBuffers              pglGenBuffers;
static PFN_glDeleteBuffers           pglDeleteBuffers;
static PFN_glBindBuffer              pglBindBuffer;
static PFN_glBufferData              pglBufferData;
static PFN_glGenFramebuffers         pglGenFramebuffers;
static PFN_glDeleteFramebuffers      pglDeleteFramebuffers;
static PFN_glBindFramebuffer         pglBindFramebuffer;
static PFN_glFramebufferTexture2D    pglFramebufferTexture2D;
static PFN_glGenRenderbuffers        pglGenRenderbuffers;
static PFN_glDeleteRenderbuffers     pglDeleteRenderbuffers;
static PFN_glBindRenderbuffer        pglBindRenderbuffer;
static PFN_glRenderbufferStorage     pglRenderbufferStorage;
static PFN_glFramebufferRenderbuffer pglFramebufferRenderbuffer;
static PFN_glCheckFramebufferStatus  pglCheckFramebufferStatus;

#define GL_FRAMEBUFFER                0x8D40
#define GL_RENDERBUFFER               0x8D41
#define GL_COLOR_ATTACHMENT0          0x8CE0
#define GL_DEPTH_ATTACHMENT           0x8D00
#define GL_DEPTH_COMPONENT16          0x81A5
#define GL_FRAMEBUFFER_COMPLETE       0x8CD5
#define GL_FRAMEBUFFER_BINDING        0x8CA6
#define GL_ARRAY_BUFFER               0x8892
#define GL_STREAM_DRAW                0x88E0
#define GL_FRAGMENT_SHADER            0x8B30
#define GL_VERTEX_SHADER              0x8B31
#define GL_COMPILE_STATUS             0x8B81
#define GL_LINK_STATUS                0x8B82
#define GL_INFO_LOG_LENGTH            0x8B84

static bool LoadGLES2Functions()
{
#define LOAD(name) p##name = (PFN_##name)SDL_GL_GetProcAddress(#name); if (!p##name) return false
    LOAD(glCreateShader);
    LOAD(glShaderSource);
    LOAD(glCompileShader);
    LOAD(glGetShaderiv);
    LOAD(glGetShaderInfoLog);
    LOAD(glCreateProgram);
    LOAD(glAttachShader);
    LOAD(glLinkProgram);
    LOAD(glGetProgramiv);
    LOAD(glGetProgramInfoLog);
    LOAD(glUseProgram);
    LOAD(glGetAttribLocation);
    LOAD(glGetUniformLocation);
    LOAD(glUniform1i);
    LOAD(glUniform1f);
    LOAD(glUniform4f);
    LOAD(glUniformMatrix4fv);
    LOAD(glEnableVertexAttribArray);
    LOAD(glDisableVertexAttribArray);
    LOAD(glVertexAttribPointer);
    LOAD(glDeleteShader);
    LOAD(glDeleteProgram);
    LOAD(glGenBuffers);
    LOAD(glDeleteBuffers);
    LOAD(glBindBuffer);
    LOAD(glBufferData);
#undef LOAD
    // FBO — try core names first, then ARB suffix
#define LOADFBO(name) p##name = (PFN_##name)SDL_GL_GetProcAddress(#name); \
    if (!p##name) p##name = (PFN_##name)SDL_GL_GetProcAddress(#name "ARB"); \
    if (!p##name) p##name = (PFN_##name)SDL_GL_GetProcAddress(#name "EXT")
    LOADFBO(glGenFramebuffers);
    LOADFBO(glDeleteFramebuffers);
    LOADFBO(glBindFramebuffer);
    LOADFBO(glFramebufferTexture2D);
    LOADFBO(glGenRenderbuffers);
    LOADFBO(glDeleteRenderbuffers);
    LOADFBO(glBindRenderbuffer);
    LOADFBO(glRenderbufferStorage);
    LOADFBO(glFramebufferRenderbuffer);
    LOADFBO(glCheckFramebufferStatus);
#undef LOADFBO
    return pglGenFramebuffers && pglBindFramebuffer && pglFramebufferTexture2D;
}

// ---------------------------------------------------------------------------
// Shader compilation helpers
// ---------------------------------------------------------------------------
static GLuint CompileShader(GLenum type, const char *src)
{
    GLuint s = pglCreateShader(type);
    pglShaderSource(s, 1, &src, NULL);
    pglCompileShader(s);
    GLint ok = 0;
    pglGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char buf[1024];
        pglGetShaderInfoLog(s, sizeof(buf), NULL, buf);
        fprintf(stderr, "[RendererGLES] Shader compile error:\n%s\n", buf);
    }
    return s;
}

static GLuint BuildProgram(const char *vsSrc, const char *fsSrc)
{
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSrc);
    GLuint prog = pglCreateProgram();
    pglAttachShader(prog, vs);
    pglAttachShader(prog, fs);
    pglLinkProgram(prog);
    GLint ok = 0;
    pglGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char buf[1024];
        pglGetProgramInfoLog(prog, sizeof(buf), NULL, buf);
        fprintf(stderr, "[RendererGLES] Program link error:\n%s\n", buf);
    }
    pglDeleteShader(vs);
    pglDeleteShader(fs);
    return prog;
}

// ---------------------------------------------------------------------------
// Helper: upload RGBA surface to a GL texture (same as RendererGL version)
// ---------------------------------------------------------------------------
static void UploadRgbaSurface(GLuint tex, SDL_Surface *rgba)
{
    const i32 bpp = 4;
    const i32 tightPitch = rgba->w * bpp;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (rgba->pitch == tightPitch)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
        return;
    }
    std::vector<u8> packed((size_t)tightPitch * (size_t)rgba->h);
    const u8 *src = (const u8 *)rgba->pixels;
    for (i32 y = 0; y < rgba->h; ++y)
        memcpy(&packed[(size_t)y * tightPitch], src + (size_t)y * rgba->pitch, tightPitch);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, packed.data());
}

// =========================================================================
// Init / InitDevice / lifecycle
// =========================================================================

void RendererGLES::Init(SDL_Window *win, SDL_GLContext ctx, i32 w, i32 h)
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
    this->textureEnabled = 1;

    D3DXMatrixIdentity(&this->viewMatrix);
    D3DXMatrixIdentity(&this->projectionMatrix);
    D3DXMatrixIdentity(&this->modelviewMatrix);
    D3DXMatrixIdentity(&this->worldMatrix);
    D3DXMatrixIdentity(&this->textureMatrix);

    i32 drawW, drawH;
    SDL_GL_GetDrawableSize(win, &drawW, &drawH);
    this->realScreenWidth = drawW;
    this->realScreenHeight = drawH;

    // Load GL2/GLES2 functions
    if (!LoadGLES2Functions())
    {
        fprintf(stderr, "[RendererGLES] Failed to load required GL functions\n");
        return;
    }

    // Debug: identify renderer in log
    const char *glVer = (const char *)glGetString(GL_VERSION);
    const char *glRen = (const char *)glGetString(GL_RENDERER);
    fprintf(stderr, "[RendererGLES] GL_VERSION:  %s\n", glVer ? glVer : "?");
    fprintf(stderr, "[RendererGLES] GL_RENDERER: %s\n", glRen ? glRen : "?");

    // Build shader program
    this->shaderProgram = BuildProgram(kGLES_VertexShader, kGLES_FragmentShader);
    pglUseProgram(this->shaderProgram);

    // Cache attribute/uniform locations
    this->loc_a_Position      = pglGetAttribLocation(shaderProgram, "a_Position");
    this->loc_a_Color         = pglGetAttribLocation(shaderProgram, "a_Color");
    this->loc_a_TexCoord      = pglGetAttribLocation(shaderProgram, "a_TexCoord");
    this->loc_u_MVP           = pglGetUniformLocation(shaderProgram, "u_MVP");
    this->loc_u_TexMatrix     = pglGetUniformLocation(shaderProgram, "u_TexMatrix");
    this->loc_u_ModelView     = pglGetUniformLocation(shaderProgram, "u_ModelView");
    this->loc_u_Texture       = pglGetUniformLocation(shaderProgram, "u_Texture");
    this->loc_u_TextureEnabled = pglGetUniformLocation(shaderProgram, "u_TextureEnabled");
    this->loc_u_ColorOp       = pglGetUniformLocation(shaderProgram, "u_ColorOp");
    this->loc_u_AlphaRef      = pglGetUniformLocation(shaderProgram, "u_AlphaRef");
    this->loc_u_FogEnabled    = pglGetUniformLocation(shaderProgram, "u_FogEnabled");
    this->loc_u_FogStart      = pglGetUniformLocation(shaderProgram, "u_FogStart");
    this->loc_u_FogEnd        = pglGetUniformLocation(shaderProgram, "u_FogEnd");
    this->loc_u_FogColor      = pglGetUniformLocation(shaderProgram, "u_FogColor");

    // Set texture unit 0
    pglUniform1i(this->loc_u_Texture, 0);
    pglUniform1f(this->loc_u_AlphaRef, 4.0f / 255.0f);
    pglUniform1i(this->loc_u_TextureEnabled, 1);
    pglUniform1i(this->loc_u_ColorOp, 0);

    // Create dynamic VBO
    pglGenBuffers(1, &this->vbo);
    this->attribsEnabled = false;

    // Enable core GL state (no fixed-function)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_SCISSOR_TEST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
#ifdef __ANDROID__
    glDepthRangef(0.0f, 1.0f);
#else
    glDepthRange(0.0, 1.0);
#endif

    // Upload identity texture matrix
    D3DXMATRIX ident;
    D3DXMatrixIdentity(&ident);
    pglUniformMatrix4fv(this->loc_u_TexMatrix, 1, GL_FALSE, &ident.m[0][0]);
    pglUniformMatrix4fv(this->loc_u_ModelView, 1, GL_FALSE, &ident.m[0][0]);

    // Create FBO if screen differs from game size
    this->fbo = 0;
    this->fboColorTex = 0;
    this->fboDepthRb = 0;
    if (drawW != w || drawH != h)
    {
        glGenTextures(1, &this->fboColorTex);
        glBindTexture(GL_TEXTURE_2D, this->fboColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        pglGenRenderbuffers(1, &this->fboDepthRb);
        pglBindRenderbuffer(GL_RENDERBUFFER, this->fboDepthRb);
        pglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);

        pglGenFramebuffers(1, &this->fbo);
        pglBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
        pglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->fboColorTex, 0);
        pglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, this->fboDepthRb);

        GLenum status = pglCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            pglBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteTextures(1, &this->fboColorTex);
            pglDeleteRenderbuffers(1, &this->fboDepthRb);
            pglDeleteFramebuffers(1, &this->fbo);
            this->fbo = 0;
            this->fboColorTex = 0;
            this->fboDepthRb = 0;
        }
        else
        {
            pglBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    SetViewport(0, 0, w, h);
    BeginFrame();
}

void RendererGLES::InitDevice(u32 opts)
{
    pglUseProgram(this->shaderProgram);

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Alpha test threshold via shader uniform
    pglUniform1f(this->loc_u_AlphaRef, 4.0f / 255.0f);

    // Fog
    if (((opts >> GCOS_DONT_USE_FOG) & 1) == 0)
    {
        this->fogEnabled = 1;
    }
    else
    {
        this->fogEnabled = 0;
    }
    this->fogColor = 0xffa0a0a0;
    this->fogStart = 1000.0f;
    this->fogEnd = 5000.0f;
    pglUniform1i(this->loc_u_FogEnabled, this->fogEnabled);
    pglUniform1f(this->loc_u_FogStart, this->fogStart);
    pglUniform1f(this->loc_u_FogEnd, this->fogEnd);
    pglUniform4f(this->loc_u_FogColor, 0.627f, 0.627f, 0.627f, 1.0f);

    // Default texture mode: modulate
    pglUniform1i(this->loc_u_ColorOp, 0);
    pglUniform1i(this->loc_u_TextureEnabled, 1);
    this->textureEnabled = 1;
}

void RendererGLES::Release()
{
    if (this->fbo != 0)
    {
        pglBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteTextures(1, &this->fboColorTex);
        pglDeleteRenderbuffers(1, &this->fboDepthRb);
        pglDeleteFramebuffers(1, &this->fbo);
        this->fbo = 0;
        this->fboColorTex = 0;
        this->fboDepthRb = 0;
    }
    if (this->vbo != 0)
    {
        pglDeleteBuffers(1, &this->vbo);
        this->vbo = 0;
    }
    if (this->shaderProgram != 0)
    {
        pglDeleteProgram(this->shaderProgram);
        this->shaderProgram = 0;
    }
    this->window = nullptr;
    this->glContext = nullptr;
}

void RendererGLES::ResizeTarget()
{
    // Clean up old FBO resources
    if (this->fbo != 0)
    {
        pglBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteTextures(1, &this->fboColorTex);
        pglDeleteRenderbuffers(1, &this->fboDepthRb);
        pglDeleteFramebuffers(1, &this->fbo);
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
        glGenTextures(1, &this->fboColorTex);
        glBindTexture(GL_TEXTURE_2D, this->fboColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        pglGenRenderbuffers(1, &this->fboDepthRb);
        pglBindRenderbuffer(GL_RENDERBUFFER, this->fboDepthRb);
        pglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);

        pglGenFramebuffers(1, &this->fbo);
        pglBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
        pglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->fboColorTex, 0);
        pglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, this->fboDepthRb);

        GLenum status = pglCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            pglBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteTextures(1, &this->fboColorTex);
            pglDeleteRenderbuffers(1, &this->fboDepthRb);
            pglDeleteFramebuffers(1, &this->fbo);
            this->fbo = 0;
            this->fboColorTex = 0;
            this->fboDepthRb = 0;
        }
        else
        {
            pglBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    SetViewport(0, 0, w, h);
    BeginFrame();
}

void RendererGLES::BeginScene() {}
void RendererGLES::EndScene() {}

void RendererGLES::BeginFrame()
{
    pglUseProgram(this->shaderProgram);
    if (this->fbo != 0)
    {
        pglBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
        glViewport(this->viewportX,
                   this->screenHeight - this->viewportY - this->viewportH,
                   this->viewportW, this->viewportH);
        glScissor(this->viewportX,
                  this->screenHeight - this->viewportY - this->viewportH,
                  this->viewportW, this->viewportH);
    }
}

// continued below — Clear, SetViewport, state setters

void RendererGLES::Clear(D3DCOLOR color, i32 clearColor, i32 clearDepth)
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

void RendererGLES::SetViewport(i32 x, i32 y, i32 w, i32 h, f32 minZ, f32 maxZ)
{
    this->viewportX = x;
    this->viewportY = y;
    this->viewportW = w;
    this->viewportH = h;
    glViewport(x, this->screenHeight - y - h, w, h);
    glScissor(x, this->screenHeight - y - h, w, h);
#ifdef __ANDROID__
    glDepthRangef(minZ, maxZ);
#else
    glDepthRange(minZ, maxZ);
#endif
}

void RendererGLES::SetBlendMode(u8 mode)
{
    if (mode == this->currentBlendMode)
        return;
    this->currentBlendMode = mode;
    if (mode == BLEND_MODE_ADD)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RendererGLES::SetColorOp(u8 colorOp)
{
    if (colorOp == this->currentColorOp)
        return;
    this->currentColorOp = colorOp;
    pglUniform1i(this->loc_u_ColorOp, colorOp);
}

void RendererGLES::SetTexture(u32 tex)
{
    if (tex == this->currentTexture)
        return;
    this->currentTexture = tex;
    if (tex != 0)
    {
        glBindTexture(GL_TEXTURE_2D, tex);
        if (!this->textureEnabled)
        {
            this->textureEnabled = 1;
            pglUniform1i(this->loc_u_TextureEnabled, 1);
        }
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        if (this->textureEnabled)
        {
            this->textureEnabled = 0;
            pglUniform1i(this->loc_u_TextureEnabled, 0);
        }
    }
}

void RendererGLES::SetTextureFactor(D3DCOLOR factor)
{
    this->textureFactor = factor;
}

void RendererGLES::SetZWriteDisable(u8 disable)
{
    if (disable == this->currentZWriteDisable)
        return;
    this->currentZWriteDisable = disable;
    glDepthMask(disable ? GL_FALSE : GL_TRUE);
}

void RendererGLES::SetDepthFunc(i32 alwaysPass)
{
    glDepthFunc(alwaysPass ? GL_ALWAYS : GL_LEQUAL);
}

void RendererGLES::SetDestBlendInvSrcAlpha()
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RendererGLES::SetDestBlendOne()
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void RendererGLES::SetTextureStageSelectDiffuse()
{
    this->textureEnabled = 0;
    pglUniform1i(this->loc_u_TextureEnabled, 0);
    this->currentTexture = 0;
}

void RendererGLES::SetTextureStageModulateTexture()
{
    this->textureEnabled = 1;
    pglUniform1i(this->loc_u_TextureEnabled, 1);
}

void RendererGLES::SetFog(i32 enable, D3DCOLOR color, f32 start, f32 end)
{
    this->fogEnabled = enable;
    this->fogColor = color;
    this->fogStart = start;
    this->fogEnd = end;
    pglUniform1i(this->loc_u_FogEnabled, enable);
    if (enable)
    {
        pglUniform1f(this->loc_u_FogStart, start);
        pglUniform1f(this->loc_u_FogEnd, end);
        pglUniform4f(this->loc_u_FogColor,
                     D3DCOLOR_R(color) / 255.0f,
                     D3DCOLOR_G(color) / 255.0f,
                     D3DCOLOR_B(color) / 255.0f,
                     D3DCOLOR_A(color) / 255.0f);
    }
}

// continued below — transforms

void RendererGLES::SetViewTransform(const D3DXMATRIX *matrix)
{
    this->viewMatrix = *matrix;
    D3DXMatrixMultiply(&this->modelviewMatrix, &this->worldMatrix, &this->viewMatrix);
}

void RendererGLES::SetProjectionTransform(const D3DXMATRIX *matrix)
{
    this->projectionMatrix = *matrix;
}

void RendererGLES::SetWorldTransform(const D3DXMATRIX *matrix)
{
    this->worldMatrix = *matrix;
    D3DXMatrixMultiply(&this->modelviewMatrix, matrix, &this->viewMatrix);
}

void RendererGLES::SetTextureTransform(const D3DXMATRIX *matrix)
{
    this->textureMatrix = *matrix;
    pglUniformMatrix4fv(this->loc_u_TexMatrix, 1, GL_FALSE, &matrix->m[0][0]);
}

// =========================================================================
// UploadMVP — compute MVP = projection * modelview, upload to shader
// =========================================================================
void RendererGLES::UploadMVP()
{
    D3DXMATRIX mvp;
    D3DXMatrixMultiply(&mvp, &this->modelviewMatrix, &this->projectionMatrix);
    pglUniformMatrix4fv(this->loc_u_MVP, 1, GL_FALSE, &mvp.m[0][0]);
    pglUniformMatrix4fv(this->loc_u_ModelView, 1, GL_FALSE, &this->modelviewMatrix.m[0][0]);
}

void RendererGLES::UploadUniforms()
{
    pglUniform1i(this->loc_u_FogEnabled, this->fogEnabled);
    pglUniform1f(this->loc_u_FogStart, this->fogStart);
    pglUniform1f(this->loc_u_FogEnd, this->fogEnd);
    pglUniform4f(this->loc_u_FogColor,
                 D3DCOLOR_R(this->fogColor) / 255.0f,
                 D3DCOLOR_G(this->fogColor) / 255.0f,
                 D3DCOLOR_B(this->fogColor) / 255.0f,
                 D3DCOLOR_A(this->fogColor) / 255.0f);
}

// =========================================================================
// DrawArrays — generic vertex submission via VBO
// Interleaved layout: pos(3f) + color(4f) + texcoord(2f) = 9 floats/vert
// =========================================================================
void RendererGLES::DrawArrays(GLenum mode, const f32 *positions, const f32 *colors,
                               const f32 *texcoords, i32 vertCount)
{
    const i32 stride = 9 * sizeof(f32); // pos(3) + col(4) + tc(2)
    const size_t needed = (size_t)vertCount * 9;

    // If positions is null, caller already filled drawScratch directly
    if (positions)
    {
        if (this->drawScratch.size() < needed)
            this->drawScratch.resize(needed);
        f32 *buf = this->drawScratch.data();

        if (texcoords)
        {
            for (i32 i = 0; i < vertCount; i++)
            {
                f32 *dst = buf + i * 9;
                dst[0] = positions[i * 3 + 0];
                dst[1] = positions[i * 3 + 1];
                dst[2] = positions[i * 3 + 2];
                dst[3] = colors[i * 4 + 0];
                dst[4] = colors[i * 4 + 1];
                dst[5] = colors[i * 4 + 2];
                dst[6] = colors[i * 4 + 3];
                dst[7] = texcoords[i * 2 + 0];
                dst[8] = texcoords[i * 2 + 1];
            }
        }
        else
        {
            for (i32 i = 0; i < vertCount; i++)
            {
                f32 *dst = buf + i * 9;
                dst[0] = positions[i * 3 + 0];
                dst[1] = positions[i * 3 + 1];
                dst[2] = positions[i * 3 + 2];
                dst[3] = colors[i * 4 + 0];
                dst[4] = colors[i * 4 + 1];
                dst[5] = colors[i * 4 + 2];
                dst[6] = colors[i * 4 + 3];
                dst[7] = 0.0f;
                dst[8] = 0.0f;
            }
        }
    }

    pglBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    pglBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(needed * sizeof(f32)),
                  this->drawScratch.data(), GL_STREAM_DRAW);

    if (!this->attribsEnabled)
    {
        pglEnableVertexAttribArray(this->loc_a_Position);
        pglEnableVertexAttribArray(this->loc_a_Color);
        pglEnableVertexAttribArray(this->loc_a_TexCoord);
        this->attribsEnabled = true;
    }

    pglVertexAttribPointer(this->loc_a_Position, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    pglVertexAttribPointer(this->loc_a_Color, 4, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(f32)));
    pglVertexAttribPointer(this->loc_a_TexCoord, 2, GL_FLOAT, GL_FALSE, stride, (void *)(7 * sizeof(f32)));

    glDrawArrays(mode, 0, vertCount);
}

// continued below — draw methods

// Helper: unpack D3DCOLOR ARGB to float RGBA
static inline void ColorToFloat(D3DCOLOR c, f32 *out)
{
    out[0] = D3DCOLOR_R(c) / 255.0f;
    out[1] = D3DCOLOR_G(c) / 255.0f;
    out[2] = D3DCOLOR_B(c) / 255.0f;
    out[3] = D3DCOLOR_A(c) / 255.0f;
}

// Helper: set up 2D ortho MVP for pre-transformed (XYZRHW) vertices.
// Saves/restores fog, depth, viewport, texture matrix state.
struct Begin2DState
{
    RendererGLES *r;
    i32 prevFog;
    GLboolean prevDepthTest;
    GLboolean prevDepthMask;
    GLint prevScissor[4];
    D3DXMATRIX prevTexMatrix;

    void Begin(RendererGLES *rr)
    {
        r = rr;
        prevFog = r->fogEnabled;
        prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
        glGetIntegerv(GL_SCISSOR_BOX, prevScissor);
        prevTexMatrix = r->textureMatrix;

        glViewport(0, 0, r->screenWidth, r->screenHeight);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        // Ortho MVP for screen-space coords
        D3DXMATRIX ortho;
        memset(&ortho, 0, sizeof(ortho));
        ortho._11 = 2.0f / r->screenWidth;
        ortho._22 = -2.0f / r->screenHeight;
        ortho._33 = -1.0f;   // map z [0,1] to [-1,1] (near/far)
        ortho._44 = 1.0f;
        ortho._41 = -1.0f;
        ortho._42 = 1.0f;
        ortho._43 = 0.0f;
        pglUniformMatrix4fv(r->loc_u_MVP, 1, GL_FALSE, &ortho.m[0][0]);

        // Identity modelview for fog calculation (disable fog for 2D)
        D3DXMATRIX ident;
        D3DXMatrixIdentity(&ident);
        pglUniformMatrix4fv(r->loc_u_ModelView, 1, GL_FALSE, &ident.m[0][0]);
        pglUniformMatrix4fv(r->loc_u_TexMatrix, 1, GL_FALSE, &ident.m[0][0]);
        pglUniform1i(r->loc_u_FogEnabled, 0);
    }

    void End()
    {
        glViewport(r->viewportX, r->screenHeight - r->viewportY - r->viewportH,
                   r->viewportW, r->viewportH);
        glScissor(prevScissor[0], prevScissor[1], prevScissor[2], prevScissor[3]);
        if (prevDepthTest) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
        glDepthMask(prevDepthMask);
        pglUniform1i(r->loc_u_FogEnabled, prevFog);
        pglUniformMatrix4fv(r->loc_u_TexMatrix, 1, GL_FALSE, &prevTexMatrix.m[0][0]);
    }
};

void RendererGLES::DrawTriangleStrip(const VertexDiffuseXyzrwh *verts, i32 count)
{
    u8 prevTexEnabled = this->textureEnabled;
    if (prevTexEnabled) { this->textureEnabled = 0; pglUniform1i(loc_u_TextureEnabled, 0); }

    Begin2DState s; s.Begin(this);

    const size_t needed = (size_t)count * 9;
    if (this->drawScratch.size() < needed)
        this->drawScratch.resize(needed);
    f32 *buf = this->drawScratch.data();
    for (i32 i = 0; i < count; i++)
    {
        f32 *dst = buf + i * 9;
        dst[0] = verts[i].position.x;
        dst[1] = verts[i].position.y;
        dst[2] = verts[i].position.z;
        ColorToFloat(verts[i].diffuse, dst + 3);
        dst[7] = 0.0f;
        dst[8] = 0.0f;
    }
    DrawArrays(GL_TRIANGLE_STRIP, nullptr, nullptr, nullptr, count);

    s.End();
    if (prevTexEnabled) { this->textureEnabled = 1; pglUniform1i(loc_u_TextureEnabled, 1); }
}

void RendererGLES::DrawTriangleStripTex(const VertexTex1Xyzrwh *verts, i32 count)
{
    Begin2DState s; s.Begin(this);

    f32 tfCol[4];
    ColorToFloat(this->textureFactor, tfCol);
    const size_t needed = (size_t)count * 9;
    if (this->drawScratch.size() < needed)
        this->drawScratch.resize(needed);
    f32 *buf = this->drawScratch.data();
    for (i32 i = 0; i < count; i++)
    {
        f32 *dst = buf + i * 9;
        dst[0] = verts[i].position.x;
        dst[1] = verts[i].position.y;
        dst[2] = verts[i].position.z;
        memcpy(dst + 3, tfCol, sizeof(tfCol));
        dst[7] = verts[i].textureUV.x;
        dst[8] = verts[i].textureUV.y;
    }
    DrawArrays(GL_TRIANGLE_STRIP, nullptr, nullptr, nullptr, count);

    s.End();
}

void RendererGLES::DrawTriangleStripTextured(const VertexTex1DiffuseXyzrwh *verts, i32 count)
{
    Begin2DState s; s.Begin(this);

    const size_t needed = (size_t)count * 9;
    if (this->drawScratch.size() < needed)
        this->drawScratch.resize(needed);
    f32 *buf = this->drawScratch.data();
    for (i32 i = 0; i < count; i++)
    {
        f32 *dst = buf + i * 9;
        dst[0] = verts[i].position.x;
        dst[1] = verts[i].position.y;
        dst[2] = verts[i].position.z;
        ColorToFloat(verts[i].diffuse, dst + 3);
        dst[7] = verts[i].textureUV.x;
        dst[8] = verts[i].textureUV.y;
    }
    DrawArrays(GL_TRIANGLE_STRIP, nullptr, nullptr, nullptr, count);

    s.End();
}

void RendererGLES::DrawTriangleFanTextured(const VertexTex1DiffuseXyzrwh *verts, i32 count)
{
    Begin2DState s; s.Begin(this);

    const size_t needed = (size_t)count * 9;
    if (this->drawScratch.size() < needed)
        this->drawScratch.resize(needed);
    f32 *buf = this->drawScratch.data();
    for (i32 i = 0; i < count; i++)
    {
        f32 *dst = buf + i * 9;
        dst[0] = verts[i].position.x;
        dst[1] = verts[i].position.y;
        dst[2] = verts[i].position.z;
        ColorToFloat(verts[i].diffuse, dst + 3);
        dst[7] = verts[i].textureUV.x;
        dst[8] = verts[i].textureUV.y;
    }
    DrawArrays(GL_TRIANGLE_FAN, nullptr, nullptr, nullptr, count);

    s.End();
}

// continued below — 3D draw methods

void RendererGLES::DrawTriangleStripTextured3D(const VertexTex1DiffuseXyz *verts, i32 count)
{
    UploadMVP();
    UploadUniforms();

    const size_t needed = (size_t)count * 9;
    if (this->drawScratch.size() < needed)
        this->drawScratch.resize(needed);
    f32 *buf = this->drawScratch.data();
    for (i32 i = 0; i < count; i++)
    {
        f32 *dst = buf + i * 9;
        dst[0] = verts[i].position.x;
        dst[1] = verts[i].position.y;
        dst[2] = verts[i].position.z;
        ColorToFloat(verts[i].diffuse, dst + 3);
        dst[7] = verts[i].textureUV.x;
        dst[8] = verts[i].textureUV.y;
    }
    DrawArrays(GL_TRIANGLE_STRIP, nullptr, nullptr, nullptr, count);
}

void RendererGLES::DrawTriangleFanTextured3D(const VertexTex1DiffuseXyz *verts, i32 count)
{
    UploadMVP();
    UploadUniforms();

    const size_t needed = (size_t)count * 9;
    if (this->drawScratch.size() < needed)
        this->drawScratch.resize(needed);
    f32 *buf = this->drawScratch.data();
    for (i32 i = 0; i < count; i++)
    {
        f32 *dst = buf + i * 9;
        dst[0] = verts[i].position.x;
        dst[1] = verts[i].position.y;
        dst[2] = verts[i].position.z;
        ColorToFloat(verts[i].diffuse, dst + 3);
        dst[7] = verts[i].textureUV.x;
        dst[8] = verts[i].textureUV.y;
    }
    DrawArrays(GL_TRIANGLE_FAN, nullptr, nullptr, nullptr, count);
}

void RendererGLES::DrawVertexBuffer3D(const RenderVertexInfo *verts, i32 count)
{
    UploadMVP();
    UploadUniforms();

    f32 tfCol[4];
    ColorToFloat(this->textureFactor, tfCol);

    const size_t needed = (size_t)count * 9;
    if (this->drawScratch.size() < needed)
        this->drawScratch.resize(needed);
    f32 *buf = this->drawScratch.data();
    for (i32 i = 0; i < count; i++)
    {
        f32 *dst = buf + i * 9;
        dst[0] = verts[i].position.x;
        dst[1] = verts[i].position.y;
        dst[2] = verts[i].position.z;
        memcpy(dst + 3, tfCol, sizeof(tfCol));
        dst[7] = verts[i].textureUV.x;
        dst[8] = verts[i].textureUV.y;
    }
    DrawArrays(GL_TRIANGLE_STRIP, nullptr, nullptr, nullptr, count);
}

// continued below — texture management

u32 RendererGLES::CreateTextureFromMemory(const u8 *data, i32 dataLen, D3DCOLOR colorKey,
                                           i32 *outWidth, i32 *outHeight)
{
    SDL_RWops *rw = SDL_RWFromConstMem(data, dataLen);
    if (!rw) return 0;
    SDL_Surface *surface = IMG_Load_RW(rw, 1);
    if (!surface) return 0;

    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    if (!rgba) return 0;

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
                p[3] = 0;
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

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
    return (u32)tex;
}

u32 RendererGLES::CreateEmptyTexture(i32 width, i32 height)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
    return (u32)tex;
}

void RendererGLES::DeleteTexture(u32 tex)
{
    if (tex != 0)
    {
        if (this->currentTexture == tex)
        {
            this->currentTexture = 0;
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        GLuint t = (GLuint)tex;
        glDeleteTextures(1, &t);
    }
}

void RendererGLES::CopyAlphaChannel(u32 dstTex, const u8 *srcData, i32 dataLen, i32 width, i32 height)
{
    if (dstTex == 0) return;

    SDL_RWops *rw = SDL_RWFromConstMem(srcData, dataLen);
    if (!rw) return;
    SDL_Surface *srcSurface = IMG_Load_RW(rw, 1);
    if (!srcSurface) return;
    SDL_Surface *srcRgba = SDL_ConvertSurfaceFormat(srcSurface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(srcSurface);
    if (!srcRgba) return;

    // GLES has no glGetTexImage — render texture to FBO, then readback
    GLuint tmpFbo = 0;
    pglGenFramebuffers(1, &tmpFbo);
    pglBindFramebuffer(GL_FRAMEBUFFER, tmpFbo);
    pglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, (GLuint)dstTex, 0);

    if (width <= 0 || height <= 0)
    {
        // Query via FBO readback — read 1x1 first to confirm attachment works.
        // In practice, callers always pass valid w/h; this is a safety fallback.
        width = srcRgba->w;
        height = srcRgba->h;
    }

    u8 *dstPixels = new u8[width * height * 4];
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dstPixels);

    // Copy blue channel of source as alpha channel of destination
    i32 copyW = srcRgba->w < width ? srcRgba->w : width;
    i32 copyH = srcRgba->h < height ? srcRgba->h : height;
    SDL_LockSurface(srcRgba);
    for (i32 y = 0; y < copyH; y++)
    {
        u8 *src = (u8 *)srcRgba->pixels + y * srcRgba->pitch;
        u8 *dst = dstPixels + y * width * 4;
        for (i32 x = 0; x < copyW; x++)
            dst[x * 4 + 3] = src[x * 4 + 0]; // blue -> alpha
    }
    SDL_UnlockSurface(srcRgba);

    glBindTexture(GL_TEXTURE_2D, (GLuint)dstTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dstPixels);

    // Cleanup
    pglBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
    pglDeleteFramebuffers(1, &tmpFbo);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
    delete[] dstPixels;
    SDL_FreeSurface(srcRgba);
}

void RendererGLES::UpdateTextureSubImage(u32 tex, i32 x, i32 y, i32 w, i32 h, const u8 *rgbaPixels)
{
    if (tex == 0 || !rgbaPixels) return;
    glBindTexture(GL_TEXTURE_2D, (GLuint)tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels);
    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
}

// continued below — surface operations

u32 RendererGLES::LoadSurfaceFromFile(const u8 *data, i32 dataLen, i32 *outWidth, i32 *outHeight)
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
    return (u32)tex;
}

u32 RendererGLES::LoadSurfaceFromFile(const u8 *data, i32 dataLen, D3DXIMAGE_INFO *info)
{
    i32 w = 0, h = 0;
    u32 result = LoadSurfaceFromFile(data, dataLen, &w, &h);
    if (info) { info->Width = (u32)w; info->Height = (u32)h; }
    return result;
}

// Helper: draw a textured fullscreen-aligned quad via shader (for surface blit)
void RendererGLES::CopySurfaceToScreen(u32 surfaceTex, i32 srcX, i32 srcY,
                                        i32 dstX, i32 dstY, i32 w, i32 h,
                                        i32 texW, i32 texH)
{
    if (surfaceTex == 0) return;
    f32 u0 = (f32)srcX / texW;
    f32 v0 = (f32)srcY / texH;
    f32 u1 = (f32)(srcX + (w > 0 ? w : texW)) / texW;
    f32 v1 = (f32)(srcY + (h > 0 ? h : texH)) / texH;
    i32 drawW = w > 0 ? w : texW;
    i32 drawH = h > 0 ? h : texH;

    Begin2DState s; s.Begin(this);

    u8 prevTexEnabled = this->textureEnabled;
    if (!prevTexEnabled) { this->textureEnabled = 1; pglUniform1i(loc_u_TextureEnabled, 1); }
    glBindTexture(GL_TEXTURE_2D, (GLuint)surfaceTex);

    f32 white[4] = {1, 1, 1, 1};
    f32 pos[] = {
        (f32)dstX, (f32)dstY, 0,
        (f32)(dstX + drawW), (f32)dstY, 0,
        (f32)dstX, (f32)(dstY + drawH), 0,
        (f32)(dstX + drawW), (f32)(dstY + drawH), 0
    };
    f32 col[] = { white[0],white[1],white[2],white[3],
                  white[0],white[1],white[2],white[3],
                  white[0],white[1],white[2],white[3],
                  white[0],white[1],white[2],white[3] };
    f32 tc[] = { u0,v0, u1,v0, u0,v1, u1,v1 };
    DrawArrays(GL_TRIANGLE_STRIP, pos, col, tc, 4);

    s.End();
    if (!prevTexEnabled) { this->textureEnabled = 0; pglUniform1i(loc_u_TextureEnabled, 0); }
    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
}

void RendererGLES::CopySurfaceToScreen(u32 surfaceTex, i32 texW, i32 texH, i32 dstX, i32 dstY)
{
    CopySurfaceToScreen(surfaceTex, 0, 0, dstX, dstY, texW, texH, texW, texH);
}

void RendererGLES::CopySurfaceRectToScreen(u32 surfaceTex, i32 srcX, i32 srcY, i32 srcW, i32 srcH,
                                             i32 dstX, i32 dstY, i32 texW, i32 texH)
{
    if (surfaceTex == 0) return;
    f32 u0 = (f32)srcX / texW;
    f32 v0 = (f32)srcY / texH;
    f32 u1 = (f32)(srcX + srcW) / texW;
    f32 v1 = (f32)(srcY + srcH) / texH;

    Begin2DState s; s.Begin(this);

    u8 prevTexEnabled = this->textureEnabled;
    if (!prevTexEnabled) { this->textureEnabled = 1; pglUniform1i(loc_u_TextureEnabled, 1); }
    glBindTexture(GL_TEXTURE_2D, (GLuint)surfaceTex);

    f32 pos[] = {
        (f32)dstX, (f32)dstY, 0,
        (f32)(dstX + srcW), (f32)dstY, 0,
        (f32)dstX, (f32)(dstY + srcH), 0,
        (f32)(dstX + srcW), (f32)(dstY + srcH), 0
    };
    f32 col[] = { 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1 };
    f32 tc[] = { u0,v0, u1,v0, u0,v1, u1,v1 };
    DrawArrays(GL_TRIANGLE_STRIP, pos, col, tc, 4);

    s.End();
    if (!prevTexEnabled) { this->textureEnabled = 0; pglUniform1i(loc_u_TextureEnabled, 0); }
    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
}

void RendererGLES::TakeScreenshot(u32 dstTex, i32 left, i32 top, i32 width, i32 height)
{
    if (dstTex == 0) return;

    GLint prevFbo = 0;
    if (this->fbo != 0)
    {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
        pglBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
    }

    u8 *pixels = new u8[width * height * 4];
    glReadPixels(left, this->screenHeight - top - height, width, height,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Flip vertically (GL reads bottom-up)
    u8 *flipped = new u8[width * height * 4];
    for (i32 y = 0; y < height; y++)
        memcpy(flipped + y * width * 4, pixels + (height - 1 - y) * width * 4, width * 4);

    glBindTexture(GL_TEXTURE_2D, (GLuint)dstTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, flipped);

    if (this->currentTexture != 0)
        glBindTexture(GL_TEXTURE_2D, this->currentTexture);
    if (this->fbo != 0)
        pglBindFramebuffer(GL_FRAMEBUFFER, prevFbo);

    delete[] pixels;
    delete[] flipped;
}

// continued below — BlitFBOToScreen / EndFrame

void RendererGLES::BlitFBOToScreen()
{
    pglBindFramebuffer(GL_FRAMEBUFFER, 0);

    i32 rw = this->realScreenWidth;
    i32 rh = this->realScreenHeight;

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

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);

    // Blit FBO texture via shader as a fullscreen quad
    pglUniform1i(this->loc_u_TextureEnabled, 1);
    pglUniform1i(this->loc_u_FogEnabled, 0);
    pglUniform1f(this->loc_u_AlphaRef, 0.0f); // no discard

    // Ortho [0,1]x[0,1] for normalized quad
    D3DXMATRIX ortho;
    memset(&ortho, 0, sizeof(ortho));
    ortho._11 = 2.0f; ortho._22 = 2.0f; ortho._33 = 1.0f; ortho._44 = 1.0f;
    ortho._41 = -1.0f; ortho._42 = -1.0f;
    pglUniformMatrix4fv(this->loc_u_MVP, 1, GL_FALSE, &ortho.m[0][0]);

    D3DXMATRIX ident;
    D3DXMatrixIdentity(&ident);
    pglUniformMatrix4fv(this->loc_u_TexMatrix, 1, GL_FALSE, &ident.m[0][0]);
    pglUniformMatrix4fv(this->loc_u_ModelView, 1, GL_FALSE, &ident.m[0][0]);

    glBindTexture(GL_TEXTURE_2D, this->fboColorTex);
    // Filter/wrap params already set at FBO texture creation time

    f32 pos[] = { 0,0,0, 1,0,0, 0,1,0, 1,1,0 };
    f32 col[] = { 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1 };
    f32 tc[]  = { 0,0, 1,0, 0,1, 1,1 };
    DrawArrays(GL_TRIANGLE_STRIP, pos, col, tc, 4);

    // Restore alpha ref
    pglUniform1f(this->loc_u_AlphaRef, 4.0f / 255.0f);
}

void RendererGLES::EndFrame()
{
    if (this->fbo != 0)
    {
        // Render ImGui overlay into FBO at game resolution
        glViewport(0, 0, this->screenWidth, this->screenHeight);
        glScissor(0, 0, this->screenWidth, this->screenHeight);
        glDisable(GL_DEPTH_TEST);
        pglUniform1i(this->loc_u_FogEnabled, 0);
        THPrac::THPracGuiRender();
        this->attribsEnabled = false;

        // Re-activate game shader (ImGui may have switched the active program)
        pglUseProgram(this->shaderProgram);

        // Blit FBO -> screen, swap
        BlitFBOToScreen();
        SDL_GL_SwapWindow(this->window);
#ifndef __ANDROID__
        // Post-swap blit for desktop capture tools (OBS, etc.)
        BlitFBOToScreen();
#endif

        // Restore state for next frame
        pglBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
        glEnable(GL_SCISSOR_TEST);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        pglUniform1i(this->loc_u_FogEnabled, this->fogEnabled);
        pglUniform1f(this->loc_u_AlphaRef, 4.0f / 255.0f);
        pglUniform1i(this->loc_u_TextureEnabled, this->textureEnabled);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(this->currentZWriteDisable ? GL_FALSE : GL_TRUE);

        // Invalidate dirty-flag caches so next frame's SetTexture /
        // SetBlendMode / SetColorOp / SetZWriteDisable calls always
        // go through to GL (BlitFBOToScreen changed GL state behind
        // the cache's back).
        this->currentTexture = (u32)-1;
        this->currentBlendMode = 0xff;
        this->currentColorOp = 0xff;
        this->currentZWriteDisable = 0xff;
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
        pglUniform1i(this->loc_u_FogEnabled, 0);
        THPrac::THPracGuiRender();
        this->attribsEnabled = false;
        SDL_GL_SwapWindow(this->window);
    }
}

  

#if defined(TH06_USE_GLES)
 
static RendererGLES s_RendererGLES;
IRenderer *GetRendererGLES() { return &s_RendererGLES; }
  
#endif
