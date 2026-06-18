#include "Software.hpp"
#include "Supervisor.hpp"
#include "GameWindow.hpp"
#include "i18n.hpp"
#include <algorithm>
#include <stddef.h>
#include "utils.hpp"
#include "compat/Compat.hpp"
#include <math.h>

u8 alphaThreshold = 4;

GfxInterface *Software::Init()
{
    Software* gfx = new Software;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        delete gfx;
        return NULL;
    }

    #if SDL_MAJOR_VERSION == 1
    u32 flags = SDL_SWSURFACE;
    #else
    u32 flags = 0;
    #endif

    if (g_Supervisor.cfg.windowed == 0)
    {
        flags |= SDL_FULLSCREEN_COMPAT;
    }

    g_GameWindow.ConfigureInit();

#ifdef __ANDROID__
    GetWindowSize(
        &GAME_WINDOW_WIDTH_REAL,
        &GAME_WINDOW_HEIGHT_REAL,
        &GAME_WINDOW_REFRESH_RATE
    );
#endif
    g_GameWindow.ConfigureView();

    i32 width  = GAME_WINDOW_WIDTH_REAL;
    i32 height = GAME_WINDOW_HEIGHT_REAL;

    #if SDL_MAJOR_VERSION >= 2
    i32 x = SDL_WINDOWPOS_UNDEFINED_COMPAT;
    i32 y = SDL_WINDOWPOS_UNDEFINED_COMPAT;
    gfx->window = SDL_CreateWindowCompat(TH_WINDOW_TITLE, x, y, width, height, flags);

    if (gfx->window == NULL)
    {
        delete gfx;
        return NULL;
    }

    //SDL 2 are on 3th variable... i'm trying SDL_RENDERER_SOFTWARE
    //SDL 3 are on 4th variable... i'm trying "direct3d12"
    //set 4th var into NULL to get the fastest SDL rendering
    gfx->renderer = SDL_CreateRendererCompat(
        gfx->window, 
        -1, 
        SDL_RENDERER_ACCELERATED, 
        NULL
    );
    if (gfx->renderer == NULL)
    {
        delete gfx;
        return NULL;
    }
    SDL_Texture* framebufferTexture = SDL_CreateTexture(gfx->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GAME_WINDOW_WIDTH_REAL, GAME_WINDOW_HEIGHT_REAL);
    gfx->framebufferTexture = framebufferTexture;
    if (framebufferTexture == NULL)    {
        delete gfx;
        return NULL;
    }
    #else

    gfx->screen = SDL_SetVideoMode(
        width,
        height,
        32,
        flags
    );

    if (!gfx->screen)
    {
        delete gfx;
        return NULL;
    }
    #endif

    // Can't init on header, init on creation instead
    gfx->boundTexture=NULL;
    gfx->clearDepth = 1.0f;
    gfx->useTexCoord = false;
    gfx->useDiffuse = false;

    gfx->model.Identity();
    gfx->view.Identity();
    gfx->projection.Identity();
    gfx->textureMatrix.Identity();

    gfx->textures.reserve(1024);
    gfx->freeTextures.reserve(1024);

    gfx->framebuffer = new u32[width * height];
    gfx->depthBuffer = new f32[width * height];

    gfx->noVertexBuffer =
        g_Supervisor.cfg.opts & (1 << GCOS_DONT_USE_VERTEX_BUF);

    gfx->noFog =
        g_Supervisor.cfg.opts & (1 << GCOS_DONT_USE_FOG);

    return gfx;
}

bool Software::GameLoop(){
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
        {
            return false;
        }
    }
    return true;
}


void Software::Exit()
{
    #if SDL_MAJOR_VERSION >= 2
    if (this->renderer)
    {
        SDL_DestroyRenderer(this->renderer);
        this->renderer = NULL;
    }
    if (this->window)
    {
        SDL_DestroyWindow(this->window);
        this->window = NULL;
    }
    if (this->framebufferTexture)
    {
        SDL_DestroyTexture(this->framebufferTexture);
        this->framebufferTexture = NULL;
    }
    if (this->framebuffer)    {
        delete[] this->framebuffer;
        this->framebuffer = NULL;
    }
    if (this->depthBuffer)
    {
        delete[] this->depthBuffer;
        this->depthBuffer = NULL;
    }
    #else
    delete[] framebuffer;
    framebuffer = NULL;

    delete[] depthBuffer;
    depthBuffer = NULL;

    SDL_Quit();
    #endif
}

void Software::SwapBuffers()
{
    #if SDL_MAJOR_VERSION >= 2
    SDL_UpdateTexture(framebufferTexture, NULL, framebuffer, GAME_WINDOW_WIDTH_REAL * sizeof(u32));
    SDL_RenderCopy(renderer, framebufferTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
    #else
    SDL_LockSurface(screen);
    const i32 width  = GAME_WINDOW_WIDTH_REAL;
    const i32 height = GAME_WINDOW_HEIGHT_REAL;
    for (i32 y = 0; y < height; y++)
    {
        memcpy(
            (u8*)screen->pixels + y * screen->pitch,
            framebuffer + y * width,
            width * sizeof(u32)
        );
    }
    SDL_UnlockSurface(screen);
    SDL_Flip(screen);
    #endif
}

void Software::SetFogRange(f32 nearPlane, f32 farPlane)
{
    fogNear = nearPlane;
    fogFar = farPlane;
    //move precomp here (also change precompInvFogDif)
    precompFogScale = 255.0f / (fogFar - fogNear);
    precompFogBias  = 255.0f - fogFar * precompFogScale;
}

void Software::SetFogColor(ZunColor color)
{
    fogColor = ZunRGBAGet(color);
}

void Software::ToggleVertexAttribute(u8 attr, bool enable)
{
    if (attr & VERTEX_ATTR_TEX_COORD)
    {
        useTexCoord = enable;
    }
    if (attr & VERTEX_ATTR_DIFFUSE)
    {
        useDiffuse = enable;
    }
}

void Software::SetAttributePointer(VertexAttributeArrays attr, size_t stride, void *ptr)
{
    switch (attr)
    {
    case VERTEX_ARRAY_POSITION:
        this->vertexData = ptr;
        this->vertexStride = stride;
        break;
    case VERTEX_ARRAY_TEX_COORD:
        this->texCoordData = ptr;
        this->texCoordStride = stride;
        break;
    case VERTEX_ARRAY_DIFFUSE:
        this->diffuseData = ptr;
        this->diffuseStride = stride;
        break;
    }
}

void Software::SetColorOp(TextureOpComponent component, ColorOp op)
{

    if (component == COMPONENT_ALPHA)
    {
        return;
    }
    colorOp = op;
}

void Software::SetTextureFactor(ZunColor factor)
{
    textureFactor = ZunRGBAGet(factor);
}

void Software::SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix)
{
    switch (type) {
        case MATRIX_MODEL:
            model = matrix;
            break;
        case MATRIX_VIEW:
            view = matrix;
            break;
        case MATRIX_PROJECTION:
            projection = matrix;
            mvp = projection * (view * model);
            break;
        case MATRIX_TEXTURE:
            textureMatrix = matrix;
            break;
    }
}


void Software::Enable(Capabilities cap) {
    if(cap == CAPS_DEPTH_TEST) {
        useDepthTest = true;
    }
}

void Software::SetBlendMode(BlendMode mode) {
    blendMode = mode;
}

void Software::SetViewport(i32 x, i32 y, i32 width, i32 height) {
    viewport[0] = x;
    viewport[1] = y;
    viewport[2] = width;
    viewport[3] = height;
}

void Software::GetViewport(u32* viewport) {
    for (i8 i = 0; i < 4; i++) {
        viewport[i] = this->viewport[i];
    }
}

void Software::GetDepthRange(f32* depthRange) {
    depthRange[0] = this->depthNear;
    depthRange[1] = this->depthFar;
}


inline ZunColor RGBAToZunColor(u8 r, u8 g, u8 b, u8 a) {
    return ((ZunColor)a << 24) | ((ZunColor)r << 16) | ((ZunColor)g << 8) | (ZunColor)b;
}

inline ZunColor RGBAToZunColor2(ZunRGBA c) {
    return (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
}

inline ZunColor ColorDataToZunColor(ColorData colorData) {
    return RGBAToZunColor(colorData.r, colorData.g, colorData.b, colorData.a);
}

void Software::SetClearColor(f32 r, f32 g, f32 b, f32 a) {
    clearColor = RGBAToZunColor((u8)(r * 255), (u8)(g * 255), (u8)(b * 255), (u8)(a * 255));
}

void Software::SetTextureFilter() {
    #ifndef NO_SDL
    SDL_SetHintCompat(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    #endif
}

void Software::SetClearDepth(f32 depth) {
    clearDepth = depth;
}

void Software::Clear(u32 clearBits) {
    if (clearBits & CLEAR_COLOR_BUFFER) {
        std::fill(framebuffer, framebuffer + GAME_WINDOW_WIDTH_REAL * GAME_WINDOW_HEIGHT_REAL, clearColor);
    }
    if (clearBits & CLEAR_DEPTH_BUFFER) {
        std::fill(depthBuffer, depthBuffer + GAME_WINDOW_WIDTH_REAL * GAME_WINDOW_HEIGHT_REAL, clearDepth);
    }
}

void Software::SetDepthRange(f32 nearPlane, f32 farPlane) {
    depthNear = nearPlane;
    depthFar = farPlane;
}

void Software::SetDepthMask(bool enable) {
    depthMask = enable;
}

void Software::SetDepthFunc(DepthFunc func) {
    depthFunc = func;
}

GfxTextureHandle Software::CreateTexture()
{
    u32 id;
    if (!freeTextures.empty())
    {
        id = freeTextures.back();
        freeTextures.pop_back();
        textures[id] = Texture();
    }
    else
    {
        id = textures.size();
        textures.push_back(Texture());
    }

    return id;
}

void Software::BindTexture(GfxTextureHandle handle)
{
    if(handle.id >= textures.size())
        return;

    boundTexture = &textures[handle.id];
}

void Software::DeleteTexture(GfxTextureHandle handle)
{
    if(handle.id >= textures.size())
        return;

    textures[handle.id]=Texture();

    freeTextures.push_back(handle.id);
}

// inline SDL_PIXEL_FORMAT_COMPAT GetSDLPixelFormat(PixelFormat fmt, PixelDataType type) {
//     switch(type) {
//         case PIXEL_UNSIGNED_BYTE:
//             if(fmt == PIXEL_RGB) return SDL_PIXELFORMAT_RGB24;
//             else return SDL_PIXELFORMAT_RGBA32;
//         case PIXEL_UNSIGNED_SHORT_4_4_4_4:
//             return SDL_PIXELFORMAT_RGBA4444;
//         case PIXEL_UNSIGNED_SHORT_5_5_5_1:
//             return SDL_PIXELFORMAT_RGBA5551;
//         case PIXEL_UNSIGNED_SHORT_5_6_5:
//             return SDL_PIXELFORMAT_RGB565;
//     }
// }

static void ConvertToARGB8888Pitch(
    u32 width,
    u32 height,
    PixelFormat fmt,
    const void* srcData,
    u32 srcPitchBytes,
    u32* dstData,
    u32 dstPitchBytes)
{
    const u8* srcBase = (const u8*)srcData;

    if (fmt == PIXEL_RGBA)
    {
        for (u32 y = 0; y < height; ++y)
        {
            const u8* src = srcBase + y * srcPitchBytes;
            u32* dst = (u32*)((u8*)dstData + y * dstPitchBytes);
            const u8* s = src;

            for (u32 x = 0; x < width; ++x)
            {
                dst[x] =
                    (s[3] << 24) |
                    (s[0] << 16) |
                    (s[1] << 8)  |
                    (s[2]);

                s += 4;
            }
        }
    }
    else if (fmt == PIXEL_RGB)
    {
        for (u32 y = 0; y < height; ++y)
        {
            const u8* src = srcBase + y * srcPitchBytes;
            u32* dst = (u32*)((u8*)dstData + y * dstPitchBytes);
            const u8* s = src;

            for (u32 x = 0; x < width; ++x)
            {
                dst[x] =
                    (255 << 24) |
                    (s[0] << 16) |
                    (s[1] << 8)  |
                    (s[2]);
                s += 3;
            }
        }
    }
}

void Software::SetTextureImage(u32 width, u32 height, PixelFormat fmt, PixelDataType type, const void* data) {
    if (boundTexture) {
        u32 bpp = 2;
        if(type == PIXEL_UNSIGNED_BYTE) {
            if(fmt == PIXEL_RGB) bpp = 3;
            else bpp = 4;
        }

        boundTexture->texels.resize(width * height);

        if (data)
        {
            LOG_COMPAT("&boundTexture->texels[0]");
            ConvertToARGB8888Pitch(
                width,
                height,
                fmt,
                data,
                width * bpp,
                &boundTexture->texels[0],
                width * 4 //sizeof(u32)
            );
        }
        boundTexture->width = width;
        boundTexture->height = height;
        boundTexture->format = fmt;
        boundTexture->type = type;
    }
}

void Software::SetTextureSubImage(i32 xoffset, i32 yoffset, i32 width, i32 height, const void *data)
{
    if (boundTexture) {
        ConvertToARGB8888Pitch(
            width,
            height,
            PIXEL_RGB,
            data,
            width * 3, // bpp always 3
            &boundTexture->texels[0]
                +
                (yoffset * boundTexture->width) +
                xoffset,
            boundTexture->width * 4//sizeof(u32)
        );
    }
}

void Software::ReadPixels(i32 x, i32 y, i32 width, i32 height, const void* pixels) {
    u8* dst = (u8*)pixels;
    i32 pitch = width * 4;
    for (i32 row = 0; row < height; row++) {
        const u8* src = (u8*)framebuffer + ((GAME_WINDOW_HEIGHT_REAL - 1 - (y + row)) * GAME_WINDOW_WIDTH_REAL + x) * 4;
        memcpy(dst + row * pitch, src, pitch);
    }
}

inline ZunVec3 Software::ProjectToNDC(ZunVec3 vertex, ZunMatrix mv, ZunMatrix p, f32 &viewZ, f32 &W) {
    ZunVec4 clip = mv * ZunVec4(vertex, 1.0f);
    clip = p * clip;

    if (clip.w != 0.0f)
    {
        clip.x /= clip.w;
        clip.y /= clip.w;
    }

    viewZ = 0.0f;
    W = 1.0f;
    ZunVec3 v = { clip.x, clip.y, 0.0f };
    return v;
}

inline ZunVec2 Software::ProjectToNDCZunVec2(ZunVec3 vertex, f32 &z) {

    //i bring calculation matrix here, remove z usages
    //change this calculation=>ZunVec4 clip = mv * ZunVec4(vertex, 1.0f);
    ZunVec2 v;
    v.x =
        mvp.m[0][0] * vertex.x +
        mvp.m[1][0] * vertex.y +
        mvp.m[2][0] * vertex.z +
        mvp.m[3][0];

    v.y =
        mvp.m[0][1] * vertex.x +
        mvp.m[1][1] * vertex.y +
        mvp.m[2][1] * vertex.z +
        mvp.m[3][1];

    z =
        mvp.m[0][2] * vertex.x +
        mvp.m[1][2] * vertex.y +
        mvp.m[2][2] * vertex.z +
        mvp.m[3][2];

    f32 w =
        mvp.m[0][3] * vertex.x +
        mvp.m[1][3] * vertex.y +
        mvp.m[2][3] * vertex.z +
        mvp.m[3][3];

    if (w > 0){
        v.x /= w;
        v.y /= w;
    }
    return v;
}

inline ZunVec2 Software::ProjectTexCoordToNDC(ZunVec2 texCoord, ZunMatrix textureMatrix) {
    ZunVec4 clip = textureMatrix * ZunVec4(ZunParseVec3(texCoord.x, texCoord.y, 1.0f), 1.0f);
    ZunVec2 ndc = {clip.x, clip.y};
    return ndc;
}

inline ZunVec3 Software::NDCToScreen(ZunVec3 vertex) {
    ZunVec3 screen;
    screen.x = (vertex.x + 1) / 2.0f * viewport[2] + viewport[0];
    screen.y = (1 - (vertex.y + 1) / 2.0f) * viewport[3] + viewport[1];
    screen.z = vertex.z;
    return screen;
}

inline ZunVec2 Software::NDCToScreenZunVec2(ZunVec2 vertex) {
    ZunVec2 screen;
    screen.x = (vertex.x + 1) / 2.0f * viewport[2] + viewport[0];
    screen.y = (1 - (vertex.y + 1) / 2.0f) * viewport[3] + viewport[1];
    return screen;
}

inline f32 EdgeFunction(ZunVec3 v0, ZunVec3 v1, ZunVec3 v2) {
    return (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x);
}

inline f32 EdgeFunctionZunVec2(ZunVec2 v0, ZunVec2 v1, ZunVec2 v2) {
    return (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x);
}

inline u8 AlphaBlendU8(u8 src, u8 dst, u8 a, u8 ia) {
    return (u8)ZUN_MIN((((u32)src * a + (u32)dst * ia + 128) >> 8), 255);
}

inline u8 LerpU8(u8 a, u8 b, u8 t)
{
    return (u8)((a * (255 - t) + b * t) >> 8);
}

inline u32 InterpZunColor(ZunColor src, ZunColor dst, u8 t) {
    return RGBAToZunColor(
        LerpU8(ZunR(src), ZunR(dst), t),
        LerpU8(ZunG(src), ZunG(dst), t),
        LerpU8(ZunB(src), ZunB(dst), t),
        ZunA(src)
    );
}

// use pointer, i think it'll faster
void InterpZunRGBA(ZunRGBA& src, ZunRGBA& dst, u8 t) {
    u32 invT = 255 - t;
    src.r = (u8)((src.r * invT + dst.r * t) >> 8);
    src.g = (u8)((src.g * invT + dst.g * t) >> 8);
    src.b = (u8)((src.b * invT + dst.b * t) >> 8);
}

inline ZunColor ZunColorMul(u32 a, u32 b) {
    u32 R = (ZunR(a) * ZunR(b)) >> 8;
    u32 G = (ZunG(a) * ZunG(b)) >> 8;
    u32 B = (ZunB(a) * ZunB(b)) >> 8;
    u32 A = (ZunA(a) * ZunA(b)) >> 8;
    
    return RGBAToZunColor(R, G, B, A);
}

void ZunRGBAMul(ZunRGBA& src, ZunRGBA& dst) {
    src.r = (src.r * dst.r) >> 8;
    src.g = (src.g * dst.g) >> 8;
    src.b = (src.b * dst.b) >> 8;
    src.a = (src.a * dst.a) >> 8;
}

void Software::Draw(PrimitiveType type, i32 start, i32 count)
{
    if (count == 0) return;
    u32 increment = type == PRIM_TRIANGLE_STRIP ? 1 : 3;
    u32 index = start;
    u32 last_index = start + count;
    if(type == PRIM_TRIANGLE_STRIP) last_index -= 2;

    const u8* vData = (u8*)vertexData;
    #define s2dblock1\
        f32 viewZ0, viewZ1, viewZ2;\
        ZunVec2 v0 = ProjectToNDCZunVec2(*(ZunVec3*)(vData + vertexStride * index),viewZ0);\
        ZunVec2 v1 = ProjectToNDCZunVec2(*(ZunVec3*)(vData + vertexStride * (index+1)),viewZ1);\
        ZunVec2 v2 = ProjectToNDCZunVec2(*(ZunVec3*)(vData + vertexStride * (index+2)),viewZ2);

    #define s2dblock2\
        v0 = NDCToScreenZunVec2(v0);\
        v1 = NDCToScreenZunVec2(v1);\
        v2 = NDCToScreenZunVec2(v2);\
        f32 area = EdgeFunctionZunVec2(v0, v1, v2);\
        if (area == 0.0f)\
        {\
            index += increment;\
            continue;\
        }\
        i32 xmin = ZUN_MAX(viewport[0],(i32)floor(ZUN_MIN3(v0.x, v1.x, v2.x)));\
        i32 xmax = ZUN_MIN(viewport[0] + viewport[2] - 1,(i32)ceil(ZUN_MAX3(v0.x, v1.x, v2.x)));\
        i32 ymin = ZUN_MAX(viewport[1],(i32)floor(ZUN_MIN3(v0.y, v1.y, v2.y)));\
        i32 ymax = ZUN_MIN(viewport[1] + viewport[3] - 1,(i32)ceil(ZUN_MAX3(v0.y, v1.y, v2.y)));\
        const ZunVec2 vP = {xmin+0.5f, ymin+0.5f};\
        ZunVec3 edges = {\
            EdgeFunctionZunVec2(v1, v2, vP),\
            EdgeFunctionZunVec2(v2, v0, vP),\
            EdgeFunctionZunVec2(v0, v1, vP)\
        };\
        ZunVec3 e_dx = {\
            v1.y - v2.y,\
            v2.y - v0.y,\
            v0.y - v1.y\
        };\
        ZunVec3 e_dy = {\
            v2.x - v1.x,\
            v0.x - v2.x,\
            v1.x - v0.x\
        };\
        const f32 invArea = 1.0f / area;\
        const ZunVec3 w_dx = e_dx * invArea;\
        const ZunVec3 w_dy = e_dy * invArea;

    //barycentrics
    #define s2dblock3\
        const f32 w0_dx = w_dx.x;\
        const f32 w1_dx = w_dx.y;\
        const f32 w2_dx = w_dx.z;\
        const f32 w0_dy = w_dy.x;\
        const f32 w1_dy = w_dy.y;\
        const f32 w2_dy = w_dy.z;\
        f32 w0_row = edges.x * invArea;\
        f32 w1_row = edges.y * invArea;\
        f32 w2_row = edges.z * invArea;

    //fog declare
    #define fogdeclare1\
        f32 fog_row,fog_dx,fog_dy;\
        if(!noFog){\
            fog_row =\
                viewZ0 * w0_row +\
                viewZ1 * w1_row +\
                viewZ2 * w2_row;\
            fog_dx =\
                viewZ0 * w0_dx +\
                viewZ1 * w1_dx +\
                viewZ2 * w2_dx;\
            fog_dy =\
                viewZ0 * w0_dy +\
                viewZ1 * w1_dy +\
                viewZ2 * w2_dy;\
        }
    //per pixels
    #define s2dblock5\
        switch(colorOp){\
        case COLOR_OP_MODULATE:\
            frag_r = (frag_r * textureFactor.r) >> 8;\
            frag_g = (frag_g * textureFactor.g) >> 8;\
            frag_b = (frag_b * textureFactor.b) >> 8;\
            frag_a = (frag_a * textureFactor.a) >> 8;\
            break;\
        case COLOR_OP_ADD:\
            frag_r += textureFactor.r;\
            frag_g += textureFactor.g;\
            frag_b += textureFactor.b;\
            frag_a += textureFactor.a;\
            break;\
    }
 
    #define fogdeclare5\
    if(!noFog) {\
        const u8 t = (u8)ZUN_MIN(\
                ZUN_MAX(\
                    precompFogBias + fog * precompFogScale,\
                    0.0f\
                ),\
                255.0f\
            );\
        const u8 invT = 255-t;\
        frag_r = (u8)((frag_r * invT + fogColor.r * t) >> 8);\
        frag_g = (u8)((frag_g * invT + fogColor.g * t) >> 8);\
        frag_b = (u8)((frag_b * invT + fogColor.b * t) >> 8);\
    }

    #define s2dblock7\
    ZunColor dst = framebuffer[pixel];\
    u8 da = 255;\
    if(blendMode == BLEND_INV_SRC_ALPHA) {\
        da -= frag_a;\
    }\
    framebuffer[pixel] = RGBAToZunColor(\
        AlphaBlendU8(frag_r,ZunR(dst),frag_a,da),\
        AlphaBlendU8(frag_g,ZunG(dst),frag_a,da),\
        AlphaBlendU8(frag_b,ZunB(dst),frag_a,da),\
        frag_a\
    );

    //fog
    #define fogdeclare2 fog_row += fog_dy,
    #define fogdeclare3 f32 fog = fog_row;
    #define fogdeclare4 fog += fog_dx,

    #define fogdeclare1
    #define fogdeclare2
    #define fogdeclare3
    #define fogdeclare4
    #define fogdeclare5
    if(useTexCoord){
        const u8* tData = (u8*)texCoordData;
        //i move this outside, why this is inside?
        u32* texels;
        i32 texW, texH,texMaskX, texMaskY;
        if(boundTexture) {
            texels = &boundTexture->texels[0];
            texW = boundTexture->width;
            texH = boundTexture->height;
            texMaskX = texW-1;
            texMaskY = texH-1;
        }
        while (index < last_index) {
            s2dblock1

            const ZunVec2 texDim = {(f32)(texW), (f32)(texH)};
            ZunVec2 tc0 = ProjectTexCoordToNDC(*(ZunVec2*)(tData + texCoordStride * index), textureMatrix) * texDim;
            ZunVec2 tc1 = ProjectTexCoordToNDC(*(ZunVec2*)(tData + texCoordStride * (index+1)), textureMatrix) * texDim;
            ZunVec2 tc2 = ProjectTexCoordToNDC(*(ZunVec2*)(tData + texCoordStride * (index+2)), textureMatrix) * texDim;

            if (type == PRIM_TRIANGLE_STRIP && ((index - start) & 1))
            {
                std::swap(v0, v1);
                std::swap(tc0, tc1);
                std::swap(viewZ0, viewZ1);
            }

            if(EdgeFunctionZunVec2(v0, v1, v2) < 0) {
                std::swap(v1, v2);
                std::swap(tc1, tc2);
                std::swap(viewZ1, viewZ2);
            }

            s2dblock2
            s2dblock3

            f32 u_row =
                tc0.x*w0_row +
                tc1.x*w1_row +
                tc2.x*w2_row;

            f32 v_row =
                tc0.y*w0_row +
                tc1.y*w1_row +
                tc2.y*w2_row;

            const f32 u_dx =
                tc0.x*w0_dx +
                tc1.x*w1_dx +
                tc2.x*w2_dx;

            const f32 u_dy =
                tc0.x*w0_dy +
                tc1.x*w1_dy +
                tc2.x*w2_dy;

            const f32 v_dx =
                tc0.y*w0_dx +
                tc1.y*w1_dx +
                tc2.y*w2_dx;

            const f32 v_dy =
                tc0.y*w0_dy +
                tc1.y*w1_dy +
                tc2.y*w2_dy;

            fogdeclare1
            for (i32 y = ymin; y <= ymax; ++y,
                u_row += u_dy,
                v_row += v_dy,
                fogdeclare2
                w0_row += w0_dy,
                w1_row += w1_dy,
                w2_row += w2_dy)
            {
                
                f32 u = u_row;
                f32 v = v_row;

                fogdeclare3

                f32 w0 = w0_row;
                f32 w1 = w1_row;
                f32 w2 = w2_row;

                i32 rowOffset = y * GAME_WINDOW_WIDTH_REAL;

                for (i32 x = xmin; x <= xmax; ++x,
                    u += u_dx,
                    v += v_dx,
                    fogdeclare4
                    w0 += w0_dx,
                    w1 += w1_dx,
                    w2 += w2_dx)
                {
                    // barycentric inside test (fast reject first)
                    if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
                    {
                        const i32 pixel = rowOffset + x;
                        //directly inside
                        const ZunColor frag = texels[
                            ((i32)v & texMaskY) * texW +
                            ((i32)u & texMaskX)
                        ];
                        u8 frag_a=ZunA(frag);
                        if (frag_a+textureFactor.a < alphaThreshold){
                            continue;
                        }
                        u8 frag_r=ZunR(frag);
                        u8 frag_g=ZunG(frag);
                        u8 frag_b=ZunB(frag);
                
                        s2dblock5
                        fogdeclare5
                        s2dblock7
                    }
                }
            }
            index += increment;
        }
    }else if(useDiffuse){
        const u8* dData = (u8*)diffuseData;
        while (index < last_index) {
            s2dblock1

            Diffuse diffuse0 = Diffuse(*(ColorData*)(dData + diffuseStride * index));
            Diffuse diffuse1 = Diffuse(*(ColorData*)(dData + diffuseStride * (index+1)));
            Diffuse diffuse2 = Diffuse(*(ColorData*)(dData + diffuseStride * (index+2)));
            
            if (type == PRIM_TRIANGLE_STRIP && ((index - start) & 1))
            {
                std::swap(v0, v1);
                std::swap(viewZ0, viewZ1);
                std::swap(diffuse0, diffuse1);
            }

            if(EdgeFunctionZunVec2(v0, v1, v2) < 0) {
                std::swap(v1, v2);
                std::swap(viewZ1, viewZ2);
                std::swap(diffuse1, diffuse2);
            }

            s2dblock2
            s2dblock3

            fogdeclare1
            for (i32 y = ymin; y <= ymax; ++y,
                fogdeclare2
                w0_row += w0_dy,
                w1_row += w1_dy,
                w2_row += w2_dy)
            {
                
                fogdeclare3
                f32 w0 = w0_row;
                f32 w1 = w1_row;
                f32 w2 = w2_row;

                i32 rowOffset = y * GAME_WINDOW_WIDTH_REAL;

                for (i32 x = xmin; x <= xmax; ++x,
                    fogdeclare4
                    w0 += w0_dx,
                    w1 += w1_dx,
                    w2 += w2_dx)
                {
                    // barycentric inside test (fast reject first)
                    if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
                    {
                        const i32 pixel = rowOffset + x;
                        // diffuse... (but remove everything)
                        u8 frag_a = (u8)(diffuse0.a * w0 +
                                diffuse1.a * w1 +
                                diffuse2.a * w2);
                        if (frag_a+textureFactor.a < alphaThreshold){
                            continue;
                        }
                        u8 frag_r = (u8)(diffuse0.r * w0 +
                                diffuse1.r * w1 +
                                diffuse2.r * w2);
                        u8 frag_g = (u8)(diffuse0.g * w0 +
                                diffuse1.g * w1 +
                                diffuse2.g * w2);
                        u8 frag_b = (u8)(diffuse0.b * w0 +
                                diffuse1.b * w1 +
                                diffuse2.b * w2);

                        s2dblock5
                        fogdeclare5
                        s2dblock7
                    }
                }
            }
            index += increment;
        }
    }
}