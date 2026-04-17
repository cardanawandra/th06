#pragma once
// =============================================================================
// gles_shaders.h - Embedded GLSL ES 1.00 shader source for RendererGLES
// Replaces fixed-function pipeline: modelviewprojection, vertex color,
// texture modulate/add, alpha test, linear fog.
// =============================================================================
// ---------- Vertex Shader (shared by all modes) ----------
// Inputs:  a_Position (vec3), a_Color (vec4), a_TexCoord (vec2)
// Uniforms: u_MVP (mat4), u_TexMatrix (mat4)
// Outputs:  v_Color, v_TexCoord, v_FogFactor

static const char *kGLES_VertexShader = R"glsl(
attribute vec3 a_Position;
attribute vec4 a_Color;
attribute vec2 a_TexCoord;

uniform mat4 u_MVP;
uniform mat4 u_TexMatrix;

// Fog: linear, computed in eye-space (needs modelview z)
uniform int  u_FogEnabled;
uniform float u_FogStart;
uniform float u_FogEnd;
uniform mat4 u_ModelView;

varying vec4 v_Color;
varying vec2 v_TexCoord;
varying float v_FogFactor;

void main()
{
    gl_Position = u_MVP * vec4(a_Position, 1.0);
    v_Color     = a_Color;
    v_TexCoord  = (u_TexMatrix * vec4(a_TexCoord, 0.0, 1.0)).xy;

    // Linear fog: factor = (end - |eye_z|) / (end - start), clamped [0,1]
    if (u_FogEnabled != 0)
    {
        float eyeZ = -(u_ModelView * vec4(a_Position, 1.0)).z;
        v_FogFactor = clamp((u_FogEnd - eyeZ) / (u_FogEnd - u_FogStart), 0.0, 1.0);
    }
    else
    {
        v_FogFactor = 1.0; // no fog
    }
}
)glsl";

// ---------- Fragment Shader ----------
// Mode 0: GL_MODULATE  outColor = texColor * v_Color
// Mode 1: GL_ADD       outColor.rgb = texColor.rgb + v_Color.rgb; outColor.a = texColor.a * v_Color.a
// u_TextureEnabled: if 0, output = v_Color (no texture)
// Alpha test: discard if final alpha < u_AlphaRef

static const char *kGLES_FragmentShader = R"glsl(
#ifdef GL_ES
precision mediump float;
#endif

varying vec4  v_Color;
varying vec2  v_TexCoord;
varying float v_FogFactor;

uniform sampler2D u_Texture;
uniform int   u_TextureEnabled;
uniform int   u_ColorOp;       // 0=modulate, 1=add
uniform float u_AlphaRef;      // alpha test threshold (e.g. 4/255)
uniform int   u_FogEnabled;
uniform vec4  u_FogColor;

void main()
{
    vec4 color;
    if (u_TextureEnabled != 0)
    {
        vec4 tex = texture2D(u_Texture, v_TexCoord);
        if (u_ColorOp == 0)
        {
            // GL_MODULATE
            color = tex * v_Color;
        }
        else
        {
            // GL_ADD
            color = vec4(tex.rgb + v_Color.rgb, tex.a * v_Color.a);
        }
    }
    else
    {
        color = v_Color;
    }

    // Alpha test (discard instead of GL_ALPHA_TEST)
    if (color.a < u_AlphaRef)
        discard;

    // Linear fog
    if (u_FogEnabled != 0)
    {
        color.rgb = mix(u_FogColor.rgb, color.rgb, v_FogFactor);
    }

    gl_FragColor = color;
}
)glsl";

  
