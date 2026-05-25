#pragma once

#include <SDL.h>
#include <SDL_opengl.h>
#include "inttypes.hpp"
#include "SDLCompat.hpp"

// Function pointers for OpenGL functions.
// Works with SDL 1.2 using SDL_GL_GetProcAddress.

struct GLFuncTable
{
    bool isGles;
    void ResolveFunctions(bool isGlesContext);

    void (APIENTRY *glAlphaFunc)(GLenum, GLclampf);
    void (APIENTRY *glBindTexture)(GLenum, GLuint);
    void (APIENTRY *glBlendFunc)(GLenum, GLenum);
    void (APIENTRY *glClear)(GLbitfield);
    void (APIENTRY *glClearColor)(GLclampf, GLclampf, GLclampf, GLclampf);
    void (APIENTRY *glColorPointer)(GLint, GLenum, GLsizei, const GLvoid*);
    void (APIENTRY *glDeleteTextures)(GLsizei, const GLuint*);
    void (APIENTRY *glDepthFunc)(GLenum);
    void (APIENTRY *glDepthMask)(GLboolean);
    void (APIENTRY *glDisable)(GLenum);
    void (APIENTRY *glDisableClientState)(GLenum);
    void (APIENTRY *glDrawArrays)(GLenum, GLint, GLsizei);
    void (APIENTRY *glEnable)(GLenum);
    void (APIENTRY *glEnableClientState)(GLenum);
    void (APIENTRY *glFogf)(GLenum, GLfloat);
    void (APIENTRY *glFogfv)(GLenum, const GLfloat*);
    void (APIENTRY *glFogi)(GLenum, GLint);
    void (APIENTRY *glGenTextures)(GLsizei, GLuint*);
    GLenum (APIENTRY *glGetError)(void);
    void (APIENTRY *glGetFloatv)(GLenum, GLfloat*);
    void (APIENTRY *glGetIntegerv)(GLenum, GLint*);
    void (APIENTRY *glLoadIdentity)(void);
    void (APIENTRY *glLoadMatrixf)(const GLfloat*);
    void (APIENTRY *glMatrixMode)(GLenum);
    void (APIENTRY *glMultMatrixf)(const GLfloat*);
    void (APIENTRY *glPopMatrix)(void);
    void (APIENTRY *glPushMatrix)(void);
    void (APIENTRY *glReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*);
    void (APIENTRY *glShadeModel)(GLenum);
    void (APIENTRY *glTexCoordPointer)(GLint, GLenum, GLsizei, const GLvoid*);
    void (APIENTRY *glTexEnvfv)(GLenum, GLenum, const GLfloat*);
    void (APIENTRY *glTexEnvi)(GLenum, GLenum, GLint);
    void (APIENTRY *glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*);
    void (APIENTRY *glTexParameteri)(GLenum, GLenum, GLint);
    void (APIENTRY *glTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);
    void (APIENTRY *glVertexPointer)(GLint, GLenum, GLsizei, const GLvoid*);
    void (APIENTRY *glViewport)(GLint, GLint, GLsizei, GLsizei);

    void (APIENTRY *glClearDepth)(GLclampd);
    void (APIENTRY *glDepthRange)(GLclampd, GLclampd);
    void (APIENTRY *glClearDepthf)(GLclampf);
    void (APIENTRY *glDepthRangef)(GLclampf, GLclampf);

    // GL(ES) 2.X / WebGL
    #if SDL_MAJOR_VERSION >= 2
    PFNGLATTACHSHADERPROC glAttachShader;
    PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
    PFNGLCOMPILESHADERPROC glCompileShader;
    PFNGLCREATEPROGRAMPROC glCreateProgram;
    PFNGLCREATESHADERPROC glCreateShader;
    PFNGLDELETEPROGRAMPROC glDeleteProgram;
    PFNGLDELETESHADERPROC glDeleteShader;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
    PFNGLGETPROGRAMIVPROC glGetProgramiv;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
    PFNGLGETSHADERIVPROC glGetShaderiv;
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
    PFNGLLINKPROGRAMPROC glLinkProgram;
    PFNGLSHADERSOURCEPROC glShaderSource;
    PFNGLUNIFORM1FPROC glUniform1f;
    PFNGLUNIFORM1IPROC glUniform1i;
    PFNGLUNIFORM4FPROC glUniform4f;
    PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
    PFNGLUSEPROGRAMPROC glUseProgram;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
    #endif

    void GLClearDepthCompat(f32 a);
    void GLDepthRangeCompat(f32 a, f32 b);
};

extern GLFuncTable g_glFuncTable;