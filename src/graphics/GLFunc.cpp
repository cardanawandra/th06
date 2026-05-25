#include "graphics/GLFunc.hpp"

#include <SDL_video.h>
#include "inttypes.hpp"

GLFuncTable g_glFuncTable;

void GLFuncTable::ResolveFunctions(bool isGlesContext)
{
    this->isGles = isGlesContext;
    
    TRY_RESOLVE_FUNCTION(glAlphaFunc);
    TRY_RESOLVE_FUNCTION(glBindTexture);
    TRY_RESOLVE_FUNCTION(glBlendFunc);
    TRY_RESOLVE_FUNCTION(glClear);
    TRY_RESOLVE_FUNCTION(glClearColor);
    TRY_RESOLVE_FUNCTION(glColorPointer);
    TRY_RESOLVE_FUNCTION(glDeleteTextures);
    TRY_RESOLVE_FUNCTION(glDepthFunc);
    TRY_RESOLVE_FUNCTION(glDepthMask);
    TRY_RESOLVE_FUNCTION(glDisable);
    TRY_RESOLVE_FUNCTION(glDisableClientState);
    TRY_RESOLVE_FUNCTION(glDrawArrays);
    TRY_RESOLVE_FUNCTION(glEnable);
    TRY_RESOLVE_FUNCTION(glEnableClientState);
    TRY_RESOLVE_FUNCTION(glFogf);
    TRY_RESOLVE_FUNCTION(glFogfv);

    TRY_RESOLVE_FUNCTION(glFogi);

    TRY_RESOLVE_FUNCTION(glGenTextures);
    TRY_RESOLVE_FUNCTION(glGetError);
    TRY_RESOLVE_FUNCTION(glGetFloatv);
    TRY_RESOLVE_FUNCTION(glGetIntegerv);
    TRY_RESOLVE_FUNCTION(glLoadIdentity);
    TRY_RESOLVE_FUNCTION(glLoadMatrixf);
    TRY_RESOLVE_FUNCTION(glMatrixMode);
    TRY_RESOLVE_FUNCTION(glMultMatrixf);
    TRY_RESOLVE_FUNCTION(glPopMatrix);
    TRY_RESOLVE_FUNCTION(glPushMatrix);
    TRY_RESOLVE_FUNCTION(glReadPixels);
    TRY_RESOLVE_FUNCTION(glShadeModel);
    TRY_RESOLVE_FUNCTION(glTexCoordPointer);
    TRY_RESOLVE_FUNCTION(glTexEnvfv);
    TRY_RESOLVE_FUNCTION(glTexEnvi);
    TRY_RESOLVE_FUNCTION(glTexImage2D);
    TRY_RESOLVE_FUNCTION(glTexParameteri);
    TRY_RESOLVE_FUNCTION(glTexSubImage2D);
    TRY_RESOLVE_FUNCTION(glVertexPointer);
    TRY_RESOLVE_FUNCTION(glViewport);

    #if SDL_MAJOR_VERSION >= 2
    if (isGlesContext)
    {
        TRY_RESOLVE_FUNCTION(glClearDepthf)
        TRY_RESOLVE_FUNCTION(glDepthRangef)
    }
    else
    {
        TRY_RESOLVE_FUNCTION(glClearDepth)
        TRY_RESOLVE_FUNCTION(glDepthRange)
    }
    #else
    this->isGles = false;
    TRY_RESOLVE_FUNCTION(glClearDepth)
    TRY_RESOLVE_FUNCTION(glDepthRange)
    #endif

    #if SDL_MAJOR_VERSION >= 2
    TRY_RESOLVE_FUNCTION(glAttachShader)
    TRY_RESOLVE_FUNCTION(glBindAttribLocation)
    TRY_RESOLVE_FUNCTION(glCompileShader)
    TRY_RESOLVE_FUNCTION(glCreateProgram)
    TRY_RESOLVE_FUNCTION(glCreateShader)
    TRY_RESOLVE_FUNCTION(glDeleteProgram)
    TRY_RESOLVE_FUNCTION(glDeleteShader)
    TRY_RESOLVE_FUNCTION(glDisableVertexAttribArray)
    TRY_RESOLVE_FUNCTION(glEnableVertexAttribArray)
    TRY_RESOLVE_FUNCTION(glGetProgramInfoLog)
    TRY_RESOLVE_FUNCTION(glGetProgramiv)
    TRY_RESOLVE_FUNCTION(glGetShaderInfoLog)
    TRY_RESOLVE_FUNCTION(glGetShaderiv)
    TRY_RESOLVE_FUNCTION(glGetUniformLocation)
    TRY_RESOLVE_FUNCTION(glLinkProgram)
    TRY_RESOLVE_FUNCTION(glShaderSource)
    TRY_RESOLVE_FUNCTION(glUniform1f)
    TRY_RESOLVE_FUNCTION(glUniform1i)
    TRY_RESOLVE_FUNCTION(glUniform4f)
    TRY_RESOLVE_FUNCTION(glUniformMatrix4fv)
    TRY_RESOLVE_FUNCTION(glUseProgram)
    TRY_RESOLVE_FUNCTION(glVertexAttribPointer)
    #endif
}

void GLFuncTable::GLClearDepthCompat(f32 a){
    if(this->isGles){
        this->glClearDepthf((GLclampf) a);
    }
    else{
        this->glClearDepth((GLclampd) a);
    }
}

void GLFuncTable::GLDepthRangeCompat(f32 a,f32 b){
    if(this->isGles){
        this->glDepthRangef((GLclampf) a,(GLclampf) b);
    }
    else{
        this->glDepthRange((GLclampd) a,(GLclampd) b);
    }
}