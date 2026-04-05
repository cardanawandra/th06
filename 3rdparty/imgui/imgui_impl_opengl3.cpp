// dear imgui: Renderer Backend for OpenGL3/ES2 (programmable pipeline)
// Minimal implementation for th06 GLES path.
// Supports: GL 2.0+, GLES 2.0+. Uses runtime-loaded function pointers.

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#ifdef __ANDROID__
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#ifndef APIENTRY
#define APIENTRY GL_APIENTRY
#endif
#else
#include <SDL_opengl.h>
#endif
#include <SDL.h>
#include <cstdio>
#include <cstring>

// GL function pointer types
typedef GLuint (APIENTRY *PFN_glCreateShader)(GLenum);
typedef void   (APIENTRY *PFN_glShaderSource)(GLuint, GLsizei, const GLchar *const *, const GLint *);
typedef void   (APIENTRY *PFN_glCompileShader)(GLuint);
typedef void   (APIENTRY *PFN_glGetShaderiv)(GLuint, GLenum, GLint *);
typedef GLuint (APIENTRY *PFN_glCreateProgram)(void);
typedef void   (APIENTRY *PFN_glAttachShader)(GLuint, GLuint);
typedef void   (APIENTRY *PFN_glLinkProgram)(GLuint);
typedef void   (APIENTRY *PFN_glGetProgramiv)(GLuint, GLenum, GLint *);
typedef void   (APIENTRY *PFN_glUseProgram)(GLuint);
typedef GLint  (APIENTRY *PFN_glGetAttribLocation)(GLuint, const GLchar *);
typedef GLint  (APIENTRY *PFN_glGetUniformLocation)(GLuint, const GLchar *);
typedef void   (APIENTRY *PFN_glUniform1i)(GLint, GLint);
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

#define GL_ARRAY_BUFFER        0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STREAM_DRAW         0x88E0
#define GL_FRAGMENT_SHADER     0x8B30
#define GL_VERTEX_SHADER       0x8B31
#define GL_COMPILE_STATUS      0x8B81
#define GL_LINK_STATUS         0x8B82

static PFN_glCreateShader            g_glCreateShader;
static PFN_glShaderSource            g_glShaderSource;
static PFN_glCompileShader           g_glCompileShader;
static PFN_glGetShaderiv             g_glGetShaderiv;
static PFN_glCreateProgram           g_glCreateProgram;
static PFN_glAttachShader            g_glAttachShader;
static PFN_glLinkProgram             g_glLinkProgram;
static PFN_glGetProgramiv            g_glGetProgramiv;
static PFN_glUseProgram              g_glUseProgram;
static PFN_glGetAttribLocation       g_glGetAttribLocation;
static PFN_glGetUniformLocation      g_glGetUniformLocation;
static PFN_glUniform1i               g_glUniform1i;
static PFN_glUniformMatrix4fv        g_glUniformMatrix4fv;
static PFN_glEnableVertexAttribArray  g_glEnableVertexAttribArray;
static PFN_glDisableVertexAttribArray g_glDisableVertexAttribArray;
static PFN_glVertexAttribPointer     g_glVertexAttribPointer;
static PFN_glDeleteShader            g_glDeleteShader;
static PFN_glDeleteProgram           g_glDeleteProgram;
static PFN_glGenBuffers              g_glGenBuffers;
static PFN_glDeleteBuffers           g_glDeleteBuffers;
static PFN_glBindBuffer              g_glBindBuffer;
static PFN_glBufferData              g_glBufferData;

static bool LoadImGuiGL3Functions()
{
#define LOAD(n) g_##n = (PFN_##n)SDL_GL_GetProcAddress(#n); if (!g_##n) return false
    LOAD(glCreateShader); LOAD(glShaderSource); LOAD(glCompileShader);
    LOAD(glGetShaderiv); LOAD(glCreateProgram); LOAD(glAttachShader);
    LOAD(glLinkProgram); LOAD(glGetProgramiv); LOAD(glUseProgram);
    LOAD(glGetAttribLocation); LOAD(glGetUniformLocation);
    LOAD(glUniform1i); LOAD(glUniformMatrix4fv);
    LOAD(glEnableVertexAttribArray); LOAD(glDisableVertexAttribArray);
    LOAD(glVertexAttribPointer); LOAD(glDeleteShader); LOAD(glDeleteProgram);
    LOAD(glGenBuffers); LOAD(glDeleteBuffers); LOAD(glBindBuffer); LOAD(glBufferData);
#undef LOAD
    return true;
}

// Shader sources (GLSL ES 1.00 compatible)
static const char* g_VertexShaderSrc =
    "attribute vec2 Position;\n"
    "attribute vec2 UV;\n"
    "attribute vec4 Color;\n"
    "uniform mat4 ProjMtx;\n"
    "varying vec2 Frag_UV;\n"
    "varying vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
    "    Frag_UV = UV;\n"
    "    Frag_Color = Color;\n"
    "    gl_Position = ProjMtx * vec4(Position.xy, 0.0, 1.0);\n"
    "}\n";

static const char* g_FragmentShaderSrc =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "uniform sampler2D Texture;\n"
    "varying vec2 Frag_UV;\n"
    "varying vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);\n"
    "}\n";

// Backend state
static GLuint g_ShaderHandle = 0;
static GLint  g_AttribLocationTex = 0;
static GLint  g_AttribLocationProjMtx = 0;
static GLuint g_AttribLocationVtxPos = 0;
static GLuint g_AttribLocationVtxUV = 0;
static GLuint g_AttribLocationVtxColor = 0;
static GLuint g_VboHandle = 0;
static GLuint g_ElementsHandle = 0;
static GLuint g_FontTexture = 0;

bool ImGui_ImplOpenGL3_Init(const char* /*glsl_version*/)
{
    if (!LoadImGuiGL3Functions())
        return false;
    return true;
}

void ImGui_ImplOpenGL3_Shutdown()
{
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
}

void ImGui_ImplOpenGL3_NewFrame()
{
    if (!g_ShaderHandle)
        ImGui_ImplOpenGL3_CreateDeviceObjects();
}

void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data)
{
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    // Save GL state
    GLint last_program; glGetIntegerv(0x8B8D, &last_program); // GL_CURRENT_PROGRAM
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; glGetIntegerv(0x0C10, last_scissor_box); // GL_SCISSOR_BOX
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor = glIsEnabled(GL_SCISSOR_TEST);
    GLint last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_ALPHA, &last_blend_src_rgb);
    GLint last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_ALPHA, &last_blend_dst_rgb);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    // Setup orthographic projection
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    const float ortho[4][4] = {
        { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
        { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
        { 0.0f,         0.0f,        -1.0f,   0.0f },
        { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
    };
    g_glUseProgram(g_ShaderHandle);
    g_glUniform1i(g_AttribLocationTex, 0);
    g_glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho[0][0]);

    glViewport(0, 0, fb_width, fb_height);

    ImVec2 clip_off = draw_data->DisplayPos;
    ImVec2 clip_scale = draw_data->FramebufferScale;

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        g_glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
        g_glBufferData(GL_ARRAY_BUFFER,
            (GLsizeiptr)cmd_list->VtxBuffer.Size * (int)sizeof(ImDrawVert),
            (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

        g_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
        g_glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            (GLsizeiptr)cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx),
            (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        g_glEnableVertexAttribArray(g_AttribLocationVtxPos);
        g_glEnableVertexAttribArray(g_AttribLocationVtxUV);
        g_glEnableVertexAttribArray(g_AttribLocationVtxColor);
        g_glVertexAttribPointer(g_AttribLocationVtxPos,   2, GL_FLOAT,         GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
        g_glVertexAttribPointer(g_AttribLocationVtxUV,    2, GL_FLOAT,         GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
        g_glVertexAttribPointer(g_AttribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;
                if (clip_rect.x < fb_width && clip_rect.y < fb_height &&
                    clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    glScissor((int)clip_rect.x, (int)(fb_height - clip_rect.w),
                              (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));
                    glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                    glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                        sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                        (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
                }
            }
        }

        g_glDisableVertexAttribArray(g_AttribLocationVtxPos);
        g_glDisableVertexAttribArray(g_AttribLocationVtxUV);
        g_glDisableVertexAttribArray(g_AttribLocationVtxColor);
    }

    // Restore GL state
    g_glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glViewport(last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], last_scissor_box[2], last_scissor_box[3]);
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glBlendFunc(last_blend_src_rgb, last_blend_dst_rgb);
    g_glBindBuffer(GL_ARRAY_BUFFER, 0);
    g_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

bool ImGui_ImplOpenGL3_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    io.Fonts->SetTexID((ImTextureID)(intptr_t)g_FontTexture);
    return true;
}

void ImGui_ImplOpenGL3_DestroyFontsTexture()
{
    if (g_FontTexture)
    {
        glDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->SetTexID(0);
        g_FontTexture = 0;
    }
}

static GLuint CompileImGuiShader(GLenum type, const char* src)
{
    GLuint s = g_glCreateShader(type);
    g_glShaderSource(s, 1, &src, NULL);
    g_glCompileShader(s);
    GLint ok = 0;
    g_glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    return s;
}

bool ImGui_ImplOpenGL3_CreateDeviceObjects()
{
    GLuint vs = CompileImGuiShader(GL_VERTEX_SHADER, g_VertexShaderSrc);
    GLuint fs = CompileImGuiShader(GL_FRAGMENT_SHADER, g_FragmentShaderSrc);
    g_ShaderHandle = g_glCreateProgram();
    g_glAttachShader(g_ShaderHandle, vs);
    g_glAttachShader(g_ShaderHandle, fs);
    g_glLinkProgram(g_ShaderHandle);
    g_glDeleteShader(vs);
    g_glDeleteShader(fs);

    g_AttribLocationTex      = g_glGetUniformLocation(g_ShaderHandle, "Texture");
    g_AttribLocationProjMtx  = g_glGetUniformLocation(g_ShaderHandle, "ProjMtx");
    g_AttribLocationVtxPos   = (GLuint)g_glGetAttribLocation(g_ShaderHandle, "Position");
    g_AttribLocationVtxUV    = (GLuint)g_glGetAttribLocation(g_ShaderHandle, "UV");
    g_AttribLocationVtxColor = (GLuint)g_glGetAttribLocation(g_ShaderHandle, "Color");

    g_glGenBuffers(1, &g_VboHandle);
    g_glGenBuffers(1, &g_ElementsHandle);

    ImGui_ImplOpenGL3_CreateFontsTexture();
    return true;
}

void ImGui_ImplOpenGL3_DestroyDeviceObjects()
{
    if (g_VboHandle)      { g_glDeleteBuffers(1, &g_VboHandle);      g_VboHandle = 0; }
    if (g_ElementsHandle) { g_glDeleteBuffers(1, &g_ElementsHandle); g_ElementsHandle = 0; }
    if (g_ShaderHandle)   { g_glDeleteProgram(g_ShaderHandle);       g_ShaderHandle = 0; }
    ImGui_ImplOpenGL3_DestroyFontsTexture();
}
