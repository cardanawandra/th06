#include "FixedFunctionGLWIN32.hpp"
#include "Supervisor.hpp"
#include "GameWindow.hpp"
#include "i18n.hpp"

#include <windows.h>
#include <gl/GL.h>

static LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (msg)
    {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void FixedFunctionGLWIN32::SetContextFlags()
{
}

GfxInterface* FixedFunctionGLWIN32::Init()
{
    LOG_COMPAT("FixedFunctionGL::Init 1\n");
    FixedFunctionGLWIN32* gfx = new FixedFunctionGLWIN32();

    HINSTANCE instance = GetModuleHandleA(NULL);

    WNDCLASSA wc;

    ZeroMemory(&wc, sizeof(wc));

    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = instance;
    wc.lpszClassName = "TouhouWin32GL";
    wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);

    UnregisterClassA("TouhouWin32GL", instance);

    if (!RegisterClassA(&wc))
    {
        char buf[128];

        wsprintfA(buf,
            "RegisterClassA failed: %lu\n",
            GetLastError());

        LOG_COMPAT(buf);

        delete gfx;
        return NULL;
    }

    LOG_COMPAT("FixedFunctionGL::Init 2\n");

    g_GameWindow.CONFIGURE_INIT();
    g_GameWindow.CONFIGURE_VIEW();

    int width  = g_GameWindow.GAME_WINDOW_WIDTH_REAL;
    int height = g_GameWindow.GAME_WINDOW_HEIGHT_REAL;

    DWORD style = WS_OVERLAPPEDWINDOW;

    if (g_Supervisor.cfg.windowed == 0)
    {
        style = WS_POPUP;
    }

    HWND hwnd = CreateWindowExA(
        0,
        "TouhouWin32GL",
        TH_WINDOW_TITLE,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        NULL,
        NULL,
        instance,
        NULL
    );

    if (!hwnd)
    {
        char buf[128];

        wsprintfA(buf,
            "CreateWindowExA failed: %lu\n",
            GetLastError());

        LOG_COMPAT(buf);

        delete gfx;
        return NULL;
    }

    LOG_COMPAT("FixedFunctionGL::Init 3\n");

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    HDC hdc = GetDC(hwnd);

    if (!hdc)
    {
        LOG_COMPAT("GetDC failed\n");

        DestroyWindow(hwnd);

        delete gfx;
        return NULL;
    }

    PIXELFORMATDESCRIPTOR pfd;

    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize      = sizeof(pfd);
    pfd.nVersion   = 1;
    pfd.dwFlags    =
        PFD_DRAW_TO_WINDOW |
        PFD_SUPPORT_OPENGL |
        PFD_DOUBLEBUFFER;

    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);

    if (!pixelFormat)
    {
        LOG_COMPAT("ChoosePixelFormat failed\n");

        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);

        delete gfx;
        return NULL;
    }

    if (!SetPixelFormat(hdc, pixelFormat, &pfd))
    {
        LOG_COMPAT("SetPixelFormat failed\n");

        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);

        delete gfx;
        return NULL;
    }

    HGLRC glrc = wglCreateContext(hdc);

    if (!glrc)
    {
        LOG_COMPAT("wglCreateContext failed\n");

        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);

        delete gfx;
        return NULL;
    }

    if (!wglMakeCurrent(hdc, glrc))
    {
        LOG_COMPAT("wglMakeCurrent failed\n");

        wglDeleteContext(glrc);

        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);

        delete gfx;
        return NULL;
    }

    gfx->window        = hwnd;
    gfx->deviceContext = hdc;
    gfx->glContext     = glrc;

    LOG_COMPAT("FixedFunctionGL::Init 4\n");
    LOG_COMPAT("FixedFunctionGL::Init 9\n");
    g_glFuncTable.ResolveFunctions(false);

    LOG_COMPAT("FixedFunctionGL::Init 10\n");
    g_glFuncTable.glEnable(GL_TEXTURE_2D);
    g_glFuncTable.glEnableClientState(GL_VERTEX_ARRAY);

    LOG_COMPAT("FixedFunctionGL::Init 11\n");
    g_glFuncTable.glEnable(GL_ALPHA_TEST);
    g_glFuncTable.glAlphaFunc(GL_GEQUAL, 4 / 255.0f);

    LOG_COMPAT("FixedFunctionGLWIN32::Init 12\n");
    if (((g_Supervisor.cfg.opts >> GCOS_SUPPRESS_USE_OF_GOROUD_SHADING) & 1) == 1)
    {
        g_glFuncTable.glShadeModel(GL_FLAT);
    }

    if (((g_Supervisor.cfg.opts >> GCOS_DONT_USE_FOG) & 1) == 0)
    {
        g_glFuncTable.glEnable(GL_FOG);
    }

    LOG_COMPAT("FixedFunctionGLWIN32::Init 13\n");
    g_glFuncTable.glFogf(GL_FOG_DENSITY, 1.0f);
    g_glFuncTable.glFogf(GL_FOG_MODE, GL_LINEAR);

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

    LOG_COMPAT("FixedFunctionGLWIN32::Init 14\n");
    if (((g_Supervisor.cfg.opts >> GCOS_NO_COLOR_COMP) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
    }
    else
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    }

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

    LOG_COMPAT("FixedFunctionGLWIN32::Init 15\n");
    if (((g_Supervisor.cfg.opts >> GCOS_DONT_USE_VERTEX_BUF) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_CONSTANT);
    }
    else
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR);
    }

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

    LOG_COMPAT("FixedFunctionGLWIN32::Init 16\n");
    if (((g_Supervisor.cfg.opts >> GCOS_NO_COLOR_COMP) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    }
    else
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
    }

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

    LOG_COMPAT("FixedFunctionGLWIN32::Init 17\n");
    if (((g_Supervisor.cfg.opts >> GCOS_DONT_USE_VERTEX_BUF) & 1) == 0)
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
    }
    else
    {
        g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR);
    }

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

    LOG_COMPAT("FixedFunctionGLWIN32::Init finish\n");
    return gfx;
}

bool FixedFunctionGLWIN32::GameLoop(){
    MSG msg;

    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            return false;
        }

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return true;
}


void FixedFunctionGLWIN32::Exit()
{
    if (glContext)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext((HGLRC)glContext);
        glContext = NULL;
    }

    if (deviceContext && window)
    {
        ReleaseDC((HWND)window, (HDC)deviceContext);
        deviceContext = NULL;
    }

    if (window)
    {
        DestroyWindow((HWND)window);
        window = NULL;
    }
}

void FixedFunctionGLWIN32::SetFogRange(f32 nearPlane, f32 farPlane)
{
    g_glFuncTable.glFogf(GL_FOG_START, nearPlane);
    g_glFuncTable.glFogf(GL_FOG_END, farPlane);
}

void FixedFunctionGLWIN32::SetFogColor(ZunColor color)
{
    GLfloat normalizedFogColor[4] = {((color >> 16) & 0xFF) / 255.0f, ((color >> 8) & 0xFF) / 255.0f,
                                     (color & 0xFF) / 255.0f, ((color >> 24) & 0xFF) / 255.0f};

    g_glFuncTable.glFogfv(GL_FOG_COLOR, normalizedFogColor);
}

void FixedFunctionGLWIN32::ToggleVertexAttribute(u8 attr, bool enable)
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

void FixedFunctionGLWIN32::SetAttributePointer(VertexAttributeArrays attr, size_t stride, void *ptr)
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

void FixedFunctionGLWIN32::SetColorOp(TextureOpComponent component, ColorOp op)
{
    const GLenum opEnums[3] = {GL_MODULATE, GL_ADD, GL_REPLACE};

    if (component > COMPONENT_ALPHA || op > COLOR_OP_REPLACE)
    {
        return;
    }

    GLenum componentEnum = component == COMPONENT_ALPHA ? GL_COMBINE_ALPHA : GL_COMBINE_RGB;

    g_glFuncTable.glTexEnvi(GL_TEXTURE_ENV, componentEnum, opEnums[op]);
}

void FixedFunctionGLWIN32::SetTextureFactor(ZunColor factor)
{
    GLfloat tfactorColor[4] = {((factor >> 16) & 0xFF) / 255.0f, ((factor >> 8) & 0xFF) / 255.0f,
                               (factor & 0xFF) / 255.0f, ((factor >> 24) & 0xFF) / 255.0f};

    g_glFuncTable.glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, tfactorColor);
}

void FixedFunctionGLWIN32::SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix)
{
    // This is not going to work for modelview
    GLenum matrixEnum[4] = {GL_MODELVIEW, GL_MODELVIEW, GL_PROJECTION, GL_TEXTURE};

    g_glFuncTable.glMatrixMode(matrixEnum[type]);
    g_glFuncTable.glLoadMatrixf((const GLfloat *)&matrix);
}


void FixedFunctionGLWIN32::Enable(Capabilities cap) {
    switch (cap) {
        case CAPS_BLEND:
            g_glFuncTable.glEnable(GL_BLEND);
            break;
        case CAPS_DEPTH_TEST:
            g_glFuncTable.glEnable(GL_DEPTH_TEST);
            break;
    }
}

bool FixedFunctionGLWIN32::HasError() {
    return g_glFuncTable.glGetError() != GL_NO_ERROR;
}

void FixedFunctionGLWIN32::SetBlendMode(BlendMode mode) {
    if (mode == BLEND_INV_SRC_ALPHA)
    {
        g_glFuncTable.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        g_glFuncTable.glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
}

void FixedFunctionGLWIN32::SetViewport(i32 x, i32 y, i32 width, i32 height) {
    g_glFuncTable.glViewport(x, y, width, height);
}

void FixedFunctionGLWIN32::GetViewport(u32* viewport) {
    g_glFuncTable.glGetIntegerv(GL_VIEWPORT, (GLint*)viewport);
}

void FixedFunctionGLWIN32::GetDepthRange(f32* depthRange) {
    g_glFuncTable.glGetFloatv(GL_DEPTH_RANGE, depthRange);
}

void FixedFunctionGLWIN32::SetClearColor(f32 r, f32 g, f32 b, f32 a) {
    g_glFuncTable.glClearColor(r, g, b, a);
}

void FixedFunctionGLWIN32::SetTextureFilter() {
    g_glFuncTable.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void FixedFunctionGLWIN32::SetClearDepth(f32 depth) {
    g_glFuncTable.GLClearDepthCompat(depth);
}

void FixedFunctionGLWIN32::Clear(u32 clearBits) {
    GLbitfield mask = 0;

    if (clearBits & CLEAR_COLOR_BUFFER) mask |= GL_COLOR_BUFFER_BIT;
    if (clearBits & CLEAR_DEPTH_BUFFER) mask |= GL_DEPTH_BUFFER_BIT;

    g_glFuncTable.glClear(mask);
}

void FixedFunctionGLWIN32::SetDepthRange(f32 nearPlane, f32 farPlane) {
    g_glFuncTable.GLDepthRangeCompat(nearPlane, farPlane);
}

void FixedFunctionGLWIN32::SetDepthMask(bool enable) {
    g_glFuncTable.glDepthMask(enable);
}

void FixedFunctionGLWIN32::SetDepthFunc(DepthFunc func) {
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

GfxTextureHandle FixedFunctionGLWIN32::CreateTexture() {
    GLuint texture;
    g_glFuncTable.glGenTextures(1, &texture);
    return texture;
}

void FixedFunctionGLWIN32::BindTexture(GfxTextureHandle handle) {
    g_glFuncTable.glBindTexture(GL_TEXTURE_2D, handle);
}

void FixedFunctionGLWIN32::DeleteTexture(GfxTextureHandle handle) {
    g_glFuncTable.glDeleteTextures(1, (GLuint*)&handle);
}

void FixedFunctionGLWIN32::SetTextureImage(u32 width, u32 height, PixelFormat fmt, PixelDataType type, const void* data) {
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

void FixedFunctionGLWIN32::SetTextureSubImage(i32 xoffset, i32 yoffset, i32 width, i32 height, const void *data)
{
    g_glFuncTable.glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
}

void FixedFunctionGLWIN32::ReadPixels(i32 x, i32 y, i32 width, i32 height, const void* pixels) {
    g_glFuncTable.glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (void*)pixels);
}

void FixedFunctionGLWIN32::Draw(PrimitiveType type, i32 start, i32 count)
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

void FixedFunctionGLWIN32::SwapBuffers()
{
    ::SwapBuffers((HDC)deviceContext);
}