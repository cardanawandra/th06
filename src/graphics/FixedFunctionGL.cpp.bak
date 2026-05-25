#include "FixedFunctionGL.hpp"
#include "GLFunc.hpp"
#include "Supervisor.hpp"

#include <SDL.h>
#include <SDL_opengl.h>
// #include <iostream>
// void fixedfunctiongllog(string msg){
//     cout<<"fixedfunctiongllog : "<<msg<<endl;
// }

// --------------------------------------------
// SDL1.2: no context flags like SDL2
// --------------------------------------------
void FixedFunctionGL::SetContextFlags()
{
    // Optional attributes only
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
}

// --------------------------------------------
// Init
// --------------------------------------------
GfxInterface *FixedFunctionGL::Init()
{
    g_glFuncTable.glEnable(GL_TEXTURE_2D);
    g_glFuncTable.glEnableClientState(GL_VERTEX_ARRAY);

    g_glFuncTable.glEnable(GL_ALPHA_TEST);
    g_glFuncTable.glAlphaFunc(GL_GEQUAL, 4 / 255.0f);

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
    // g_glFuncTable.glFogi(GL_FOG_MODE, GL_LINEAR); // 🔧 fixed type

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

    // -------- ALPHA --------
    if (((g_Supervisor.cfg.opts >> GCOS_NO_COLOR_COMP) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
    }
    else
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    }

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

    if (((g_Supervisor.cfg.opts >> GCOS_DONT_USE_VERTEX_BUF) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_CONSTANT);
    }
    else
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR);
    }

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

    // -------- RGB --------
    if (((g_Supervisor.cfg.opts >> GCOS_NO_COLOR_COMP) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    }
    else
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
    }

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

    if (((g_Supervisor.cfg.opts >> GCOS_DONT_USE_VERTEX_BUF) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
    }
    else
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR);
    }

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

    return new FixedFunctionGL();
}

// --------------------------------------------
// Fog
// --------------------------------------------
void FixedFunctionGL::SetFogRange(f32 nearPlane, f32 farPlane)
{
    g_glFuncTable.glFogf(GL_FOG_START, nearPlane);
    g_glFuncTable.glFogf(GL_FOG_END, farPlane);
}

void FixedFunctionGL::SetFogColor(ZunColor color)
{
    GLfloat c[4] = {
        ((color >> 16) & 0xFF) / 255.0f,
        ((color >> 8) & 0xFF) / 255.0f,
        (color & 0xFF) / 255.0f,
        ((color >> 24) & 0xFF) / 255.0f
    };

    g_glFuncTable.glFogfv(GL_FOG_COLOR, c);
}

// --------------------------------------------
// Vertex attributes
// --------------------------------------------
void FixedFunctionGL::ToggleVertexAttribute(u8 attr, bool enable)
{
    if (attr & VERTEX_ATTR_TEX_COORD)
    {
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
            g_glFuncTable.glEnableClientState(GL_COLOR_ARRAY);
        else
            g_glFuncTable.glDisableClientState(GL_COLOR_ARRAY);
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

// --------------------------------------------
// Texture ops
// --------------------------------------------
void FixedFunctionGL::SetColorOp(TextureOpComponent component, ColorOp op)
{
    const GLenum ops[] = {GL_MODULATE, GL_ADD, GL_REPLACE};

    if (component > COMPONENT_ALPHA || op > COLOR_OP_REPLACE)
        return;

    GLenum target = (component == COMPONENT_ALPHA) ? GL_COMBINE_ALPHA : GL_COMBINE_RGB;

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, target, ops[op]);
}

void FixedFunctionGL::SetTextureFactor(ZunColor factor)
{
    GLfloat c[4] = {
        ((factor >> 16) & 0xFF) / 255.0f,
        ((factor >> 8) & 0xFF) / 255.0f,
        (factor & 0xFF) / 255.0f,
        ((factor >> 24) & 0xFF) / 255.0f
    };

    g_glFuncTable.glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, c);
}

// --------------------------------------------
// Matrices
// --------------------------------------------
void FixedFunctionGL::SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix)
{
    GLenum modes[4] = {GL_MODELVIEW, GL_MODELVIEW, GL_PROJECTION, GL_TEXTURE};

    g_glFuncTable.glMatrixMode(modes[type]);
    g_glFuncTable.glLoadMatrixf((const GLfloat *)&matrix.m); // 🔧 fixed
}

// --------------------------------------------
void FixedFunctionGL::Draw()
{
}