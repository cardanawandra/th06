#include "WebGL.hpp"
#include "Supervisor.hpp"
#include "utils.hpp"

#include <SDL.h>
#include <SDL_opengl.h>

void WebGL::SetContextFlags()
{
    // No GL version forcing in SDL1.2
}

GfxInterface *WebGL::Create()
{
    WebGL *interface = new WebGL;

    if (!interface->Init())
    {
        delete interface;
        return NULL;
    }

    return interface;
}

bool WebGL::Init()
{
    while (glGetError() != GL_NO_ERROR) {}

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_FOG);

    return true;
}

void WebGL::SetFogRange(f32 nearPlane, f32 farPlane)
{
    glFogf(GL_FOG_START, nearPlane);
    glFogf(GL_FOG_END, farPlane);
}

void WebGL::SetFogColor(ZunColor color)
{
    GLfloat c[4] = {
        ((color >> 16) & 0xFF) / 255.0f,
        ((color >> 8) & 0xFF) / 255.0f,
        (color & 0xFF) / 255.0f,
        ((color >> 24) & 0xFF) / 255.0f
    };

    glFogfv(GL_FOG_COLOR, c);
}

void WebGL::ToggleVertexAttribute(u8 attr, bool enable)
{
    if (attr & VERTEX_ATTR_TEX_COORD)
    {
        if (enable)
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        else
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    if (attr & VERTEX_ATTR_DIFFUSE)
    {
        if (enable)
            glEnableClientState(GL_COLOR_ARRAY);
        else
            glDisableClientState(GL_COLOR_ARRAY);
    }

    // Position always enabled
    glEnableClientState(GL_VERTEX_ARRAY);
}

void WebGL::SetAttributePointer(VertexAttributeArrays attr, size_t stride, void *ptr)
{
    switch (attr)
    {
    case VERTEX_ARRAY_POSITION:
        glVertexPointer(3, GL_FLOAT, stride, ptr);
        break;

    case VERTEX_ARRAY_TEX_COORD:
        glTexCoordPointer(2, GL_FLOAT, stride, ptr);
        break;

    case VERTEX_ARRAY_DIFFUSE:
        glColorPointer(4, GL_UNSIGNED_BYTE, stride, ptr);
        break;
    }
}

void WebGL::SetColorOp(TextureOpComponent component, ColorOp op)
{
    if (component == COMPONENT_ALPHA)
        return;

    // Simple approximation
    switch (op)
    {
    case COLOR_OP_MODULATE:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        break;

    case COLOR_OP_ADD:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
        break;

    default:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        break;
    }
}

void WebGL::SetTextureFactor(ZunColor factor)
{
    GLfloat c[4] = {
        ((factor >> 16) & 0xFF) / 255.0f,
        ((factor >> 8) & 0xFF) / 255.0f,
        (factor & 0xFF) / 255.0f,
        ((factor >> 24) & 0xFF) / 255.0f
    };

    glColor4fv(c);
}
void WebGL::SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix)
{
    /*
    switch (type)
    {
    case TRANSFORM_MODEL:
    case TRANSFORM_VIEW:
        glMatrixMode(GL_MODELVIEW);
        break;

    case TRANSFORM_PROJECTION:
        glMatrixMode(GL_PROJECTION);
        break;

    case TRANSFORM_TEXTURE:
        glMatrixMode(GL_TEXTURE);
        break;
    }

    glLoadMatrixf((const GLfloat *)&matrix.m);
    */
}

void WebGL::Draw()
{
    // Draw happens via glDrawArrays / glDrawElements elsewhere
}