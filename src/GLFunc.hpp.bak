#pragma once

#include <SDL.h>
#include <SDL_opengl.h>

// Function pointers for OpenGL functions.
// Works with SDL 1.2 using SDL_GL_GetProcAddress.

struct GLFuncTable
{
    void ResolveFunctions()
    {
        #ifdef _WIN32
            #define LOAD(name) this->name = ::name
        #else
            #define LOAD(name) *(void**)&this->name = SDL_GL_GetProcAddress(#name)
        #endif
        
        LOAD(glAlphaFunc);
        LOAD(glBindTexture);
        LOAD(glBlendFunc);
        LOAD(glClear);
        LOAD(glClearColor);
        LOAD(glColorPointer);
        LOAD(glDeleteTextures);
        LOAD(glDepthFunc);
        LOAD(glDepthMask);
        LOAD(glDisable);
        LOAD(glDisableClientState);
        LOAD(glDrawArrays);
        LOAD(glEnable);
        LOAD(glEnableClientState);
        LOAD(glFogf);
        LOAD(glFogfv);
        // LOAD(glFogi);
        LOAD(glGenTextures);
        LOAD(glGetError);
        LOAD(glGetFloatv);
        LOAD(glGetIntegerv);
        LOAD(glLoadIdentity);
        LOAD(glLoadMatrixf);
        LOAD(glMatrixMode);
        LOAD(glMultMatrixf);
        LOAD(glPopMatrix);
        LOAD(glPushMatrix);
        LOAD(glReadPixels);
        LOAD(glShadeModel);
        LOAD(glTexCoordPointer);
        LOAD(glTexEnvfv);
        LOAD(glTexEnvi);
        LOAD(glTexImage2D);
        LOAD(glTexParameteri);
        LOAD(glTexSubImage2D);
        LOAD(glVertexPointer);
        LOAD(glViewport);
        LOAD(glClearDepth);
        LOAD(glDepthRange);

        #undef LOAD
    }

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
};

extern GLFuncTable g_glFuncTable;