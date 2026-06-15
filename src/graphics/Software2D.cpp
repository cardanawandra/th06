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
    Uint32 flags = SDL_HWSURFACE;
    #else
    Uint32 flags = 0;
    #endif

    if (g_Supervisor.cfg.windowed == 0)
    {
        flags |= SDL_FULLSCREEN_COMPAT;
    }

    g_GameWindow.CONFIGURE_INIT();

#ifdef __ANDROID__
    GetWindowSize(
        &g_GameWindow.GAME_WINDOW_WIDTH_REAL,
        &g_GameWindow.GAME_WINDOW_HEIGHT_REAL,
        &g_GameWindow.GAME_WINDOW_REFRESH_RATE
    );
#endif
    g_GameWindow.GAME_WINDOW_REFRESH_RATE = g_GameWindow.GAME_WINDOW_REFRESH_RATE / 2;

    g_GameWindow.CONFIGURE_VIEW();

    int width  = g_GameWindow.GAME_WINDOW_WIDTH_REAL;
    int height = g_GameWindow.GAME_WINDOW_HEIGHT_REAL;

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
    SDL_Texture* framebufferTexture = SDL_CreateTexture(gfx->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, g_GameWindow.GAME_WINDOW_WIDTH_REAL, g_GameWindow.GAME_WINDOW_HEIGHT_REAL);
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
    SDL_UpdateTexture(framebufferTexture, NULL, framebuffer, g_GameWindow.GAME_WINDOW_WIDTH_REAL * sizeof(u32));
    SDL_RenderCopy(renderer, framebufferTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
    #else
    SDL_LockSurface(screen);
    const int width  = g_GameWindow.GAME_WINDOW_WIDTH_REAL;
    const int height = g_GameWindow.GAME_WINDOW_HEIGHT_REAL;
    for (int y = 0; y < height; y++)
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
}

void Software::SetFogColor(ZunColor color)
{
    fogColor = color;
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
    textureFactor = factor;
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
    for (int i = 0; i < 4; i++) {
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

inline ZunColor ColorDataToZunColor(ColorData colorData) {
    return RGBAToZunColor(colorData.r, colorData.g, colorData.b, colorData.a);
}

void Software::SetClearColor(f32 r, f32 g, f32 b, f32 a) {
    clearColor = RGBAToZunColor((u8)(r * 255), (u8)(g * 255), (u8)(b * 255), (u8)(a * 255));
}

void Software::SetTextureFilter() {
}

void Software::SetClearDepth(f32 depth) {
    clearDepth = depth;
}

void Software::Clear(u32 clearBits) {
    if (clearBits & CLEAR_COLOR_BUFFER) {
        std::fill(framebuffer, framebuffer + g_GameWindow.GAME_WINDOW_WIDTH_REAL * g_GameWindow.GAME_WINDOW_HEIGHT_REAL, clearColor);
    }
    if (clearBits & CLEAR_DEPTH_BUFFER) {
        std::fill(depthBuffer, depthBuffer + g_GameWindow.GAME_WINDOW_WIDTH_REAL * g_GameWindow.GAME_WINDOW_HEIGHT_REAL, clearDepth);
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
    Texture* texture = new Texture();

    u32 id;
    if (!freeTextures.empty())
    {
        id = freeTextures.back();
        freeTextures.pop_back();
        textures[id] = texture;
    }
    else
    {
        id = textures.size();
        textures.push_back(texture);
    }

    return id;
}

void Software::BindTexture(GfxTextureHandle handle)
{
    if (handle.id >= textures.size())
        return;
    if (!textures[handle.id])
        return;
    boundTexture = textures[handle.id];
}

void Software::DeleteTexture(GfxTextureHandle handle)
{
    if (handle.id >= textures.size())
        return;
    if (!textures[handle.id])
        return;
    delete textures[handle.id];
    textures[handle.id]=NULL;
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
                u8 r = s[0];
                u8 g = s[1];
                u8 b = s[2];
                u8 a = s[3];

                dst[x] =
                    ((u32)a << 24) |
                    ((u32)r << 16) |
                    ((u32)g << 8)  |
                    ((u32)b);

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
                u8 r = s[0];
                u8 g = s[1];
                u8 b = s[2];

                dst[x] =
                    0xFF000000 |
                    ((u32)r << 16) |
                    ((u32)g << 8)  |
                    ((u32)b);

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
        const u8* src = (u8*)framebuffer + ((g_GameWindow.GAME_WINDOW_HEIGHT_REAL - 1 - (y + row)) * g_GameWindow.GAME_WINDOW_WIDTH_REAL + x) * 4;
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

inline ZunVec2 ProjectToNDCZunvec2(ZunVec3 vertex, ZunMatrix mv, ZunMatrix p) {
    ZunVec4 clip = mv * ZunVec4(vertex, 1.0f);
    clip = p * clip;

    if (clip.w > 0){
        clip.x /= clip.w;
        clip.y /= clip.w;
    }

    ZunVec2 v = { clip.x, clip.y };
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

inline ZunVec2 NDCToScreenZunVec2(ZunVec2 vertex,i32 viewport[4]) {
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

inline u8 LerpU8(u32 a, u32 b, u32 t)
{
    return (u8)((a * (255 - t) + b * t) >> 8);
}

inline u32 InterpZunColor(ZunColor src, ZunColor dst, u32 t) {
    return RGBAToZunColor(
        LerpU8(ZunR(src), ZunR(dst), t),
        LerpU8(ZunG(src), ZunG(dst), t),
        LerpU8(ZunB(src), ZunB(dst), t),
        ZunA(src)
    );
}

inline ZunColor ZunColorMul(u32 a, u32 b) {
    u32 R = (ZunR(a) * ZunR(b)) >> 8;
    u32 G = (ZunG(a) * ZunG(b)) >> 8;
    u32 B = (ZunB(a) * ZunB(b)) >> 8;
    u32 A = (ZunA(a) * ZunA(b)) >> 8;
    
    return RGBAToZunColor(R, G, B, A);
}

void Software::Draw(PrimitiveType type, i32 start, i32 count)
{
    if (count == 0) return;
    u32 increment = type == PRIM_TRIANGLE_STRIP ? 1 : 3;
    u32 index = start;
    u32 last_index = start + count;
    if(type == PRIM_TRIANGLE_STRIP) last_index -= 2;
    ZunMatrix modelview = view * model;

    //i move this outside, why this is inside?
    const f32 precompInvFogDif = 1.0f/(fogFar - fogNear);
    u32* texels;
    i32 texW, texH;
    if(boundTexture) {
        texels = &boundTexture->texels[0];
        texW = boundTexture->width;
        texH = boundTexture->height;
    }
    const u8* vData = (u8*)vertexData;
    const u8* tData = (u8*)texCoordData;
    const u8* dData = (u8*)diffuseData;

    while (index < last_index) {
        // Resurrection of Fog (killed by super msvc 6)
        // f32 invw0, invw1, invw2;
        // f32 viewZ0, viewZ1, viewZ2;
        // f32 ndcZ0, ndcZ1, ndcZ2;
        // ZunVec3 v0 = ProjectToNDC(*(ZunVec3*)((u8*)vertexData + vertexStride * index),modelview,projection,viewZ0,invw0);
        // ZunVec3 v1 = ProjectToNDC(*(ZunVec3*)((u8*)vertexData + vertexStride * (index+1)),modelview,projection,viewZ1,invw1);
        // ZunVec3 v2 = ProjectToNDC(*(ZunVec3*)((u8*)vertexData + vertexStride * (index+2)),modelview,projection,viewZ2,invw2);

        //pre calculate
        ZunVec2 v0 = ProjectToNDCZunvec2(*(ZunVec3*)(vData + vertexStride * index),modelview,projection);
        ZunVec2 v1 = ProjectToNDCZunvec2(*(ZunVec3*)(vData + vertexStride * (index+1)),modelview,projection);
        ZunVec2 v2 = ProjectToNDCZunvec2(*(ZunVec3*)(vData + vertexStride * (index+2)),modelview,projection);

        ZunVec2 tc0, tc1, tc2;
        Diffuse diffuse0, diffuse1, diffuse2;
        if(useTexCoord) {
            const ZunVec2 texDim = {(f32)(boundTexture ? boundTexture->width : 0), (f32)(boundTexture ? boundTexture->height : 0)};
            tc0 = ProjectTexCoordToNDC(*(ZunVec2*)(tData + texCoordStride * index), textureMatrix) * texDim;
            tc1 = ProjectTexCoordToNDC(*(ZunVec2*)(tData + texCoordStride * (index+1)), textureMatrix) * texDim;
            tc2 = ProjectTexCoordToNDC(*(ZunVec2*)(tData + texCoordStride * (index+2)), textureMatrix) * texDim;
        }

        if(useDiffuse) {
            diffuse0 = Diffuse(*(ColorData*)(dData + diffuseStride * index));
            diffuse1 = Diffuse(*(ColorData*)(dData + diffuseStride * (index+1)));
            diffuse2 = Diffuse(*(ColorData*)(dData + diffuseStride * (index+2)));
        }
        
        if (type == PRIM_TRIANGLE_STRIP && ((index - start) & 1))
        {
            std::swap(v0, v1);
            std::swap(tc0, tc1);
            // std::swap(invw0, invw1);
            // std::swap(viewZ0, viewZ1);
            std::swap(diffuse0, diffuse1);
        }

        if(EdgeFunctionZunVec2(v0, v1, v2) < 0) {
            std::swap(v1, v2);
            std::swap(tc1, tc2);
            // std::swap(invw1, invw2);
            // std::swap(viewZ1, viewZ2);
            std::swap(diffuse1, diffuse2);
        }
        // ndcZ0 = v0.z;
        // ndcZ1 = v1.z;
        // ndcZ2 = v2.z;
        v0 = NDCToScreenZunVec2(v0,viewport);
        v1 = NDCToScreenZunVec2(v1,viewport);
        v2 = NDCToScreenZunVec2(v2,viewport);

        i32 xmin = ZUN_MAX(viewport[0],(i32)floor(ZUN_MIN3(v0.x, v1.x, v2.x)));
        i32 xmax = ZUN_MIN(viewport[0] + viewport[2] - 1,(i32)ceil(ZUN_MAX3(v0.x, v1.x, v2.x)));
        i32 ymin = ZUN_MAX(viewport[1],(i32)floor(ZUN_MIN3(v0.y, v1.y, v2.y)));
        i32 ymax = ZUN_MIN(viewport[1] + viewport[3] - 1,(i32)ceil(ZUN_MAX3(v0.y, v1.y, v2.y)));

        const ZunVec2 vP = {xmin+0.5f, ymin+0.5f};

        ZunVec3 edges = {
            EdgeFunctionZunVec2(v1, v2, vP),
            EdgeFunctionZunVec2(v2, v0, vP),
            EdgeFunctionZunVec2(v0, v1, vP)
        };

        f32 area = EdgeFunctionZunVec2(v0, v1, v2);

        if (area == 0.0f)
        {
            index += increment;
            continue;
        }

        ZunVec3 e_dx = {
            v1.y - v2.y,
            v2.y - v0.y,
            v0.y - v1.y
        };

        ZunVec3 e_dy = {
            v2.x - v1.x,
            v0.x - v2.x,
            v1.x - v0.x
        };

        f32 invarea = 1.0f / area;

        ZunVec3 w0 = edges * invarea;
        const ZunVec3 w_dx = e_dx * invarea;
        const ZunVec3 w_dy = e_dy * invarea;

        const float invArea = 1.0f / area;

        // remove ZunVec2 struct usages (is this barycentrics?)
        const float w0_dx = w_dx.x;
        const float w1_dx = w_dx.y;
        const float w2_dx = w_dx.z;

        const float w0_dy = w_dy.x;
        const float w1_dy = w_dy.y;
        const float w2_dy = w_dy.z;

        // starting barycentrics
        float w0_row = edges.x * invArea;
        float w1_row = edges.y * invArea;
        float w2_row = edges.z * invArea;

        for (int y = ymin; y <= ymax; ++y,
            w0_row += w0_dy,
            w1_row += w1_dy,
            w2_row += w2_dy)
        {
            float w0 = w0_row;
            float w1 = w1_row;
            float w2 = w2_row;

            int rowOffset = y * g_GameWindow.GAME_WINDOW_WIDTH_REAL;

            for (int x = xmin; x <= xmax; ++x,
                w0 += w0_dx,
                w1 += w1_dx,
                w2 += w2_dx)
            {
                // barycentric inside test (fast reject first)
                if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
                {
                    const int pixel = rowOffset + x;

                    // INTERPOLATED UV (inline)
                    int u, v;

                    if (useTexCoord)
                    {
                        float iu =
                            tc0.x * w0 +
                            tc1.x * w1 +
                            tc2.x * w2;

                        float iv =
                            tc0.y * w0 +
                            tc1.y * w1 +
                            tc2.y * w2;

                        u = (int)iu;
                        v = (int)iv;
                    }

                    // diffuse... (but remove everything)
                    ZunColor src;

                    if (useTexCoord)
                    {
                        src = texels[(v & (texH - 1)) * texW + (u & (texW - 1))];
                    }
                    else
                    {
                        src = RGBAToZunColor(
                            (u8)((diffuse0.r * w0 + diffuse1.r * w1 + diffuse2.r * w2)),
                            (u8)((diffuse0.g * w0 + diffuse1.g * w1 + diffuse2.g * w2)),
                            (u8)((diffuse0.b * w0 + diffuse1.b * w1 + diffuse2.b * w2)),
                            255
                        );
                    }

                    // remove frags
                    ZunColor frag = src;

                    switch(colorOp){
                        case COLOR_OP_MODULATE:
                            frag = ZunColorMul(src, textureFactor);
                            break;
                        case COLOR_OP_ADD:
                            frag = RGBAToZunColor(
                                ZunR(src) + ZunR(textureFactor),
                                ZunG(src) + ZunG(textureFactor),
                                ZunB(src) + ZunB(textureFactor),
                                ZunA(src)
                            );
                            break;
                    }

                    if (ZunA(frag) >= alphaThreshold)
                    {
                        ZunColor dst = framebuffer[pixel];

                        u8 sa = ZunA(frag);
                        u8 da = 255 - sa;

                        framebuffer[pixel] = RGBAToZunColor(
                            (ZunR(frag) * sa + ZunR(dst) * da) >> 8,
                            (ZunG(frag) * sa + ZunG(dst) * da) >> 8,
                            (ZunB(frag) * sa + ZunB(dst) * da) >> 8,
                            sa
                        );
                    }
                }
            }
        }
        index += increment;
    }
}