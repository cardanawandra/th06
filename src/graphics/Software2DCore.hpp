
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

    //precompute viewport
    screenScaleX = viewport[2] * 0.5f;
    screenBiasX  = viewport[0] + screenScaleX;
    screenScaleY = -viewport[3] * 0.5f;
    screenBiasY  = viewport[1] + viewport[3] * 0.5f;
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
    return (ZunColor)(a << 24 | r << 16 | g << 8 | b);
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
}

inline ZunVec3 Software::ProjectToNDC(ZunVec3 vertex, ZunMatrix mv, ZunMatrix p, f32 &viewZ, f32 &W) {
    ZunVec3 v;
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
    //i bring calculation matrix here, remove z and w usages
    //change this ZunVec4 clip = textureMatrix * ZunVec4(ZunParseVec3(texCoord.x, texCoord.y, 1.0f), 1.0f);
    ZunVec2 ndc = {
        textureMatrix.m[0][0] * texCoord.x +
        textureMatrix.m[0][1] * texCoord.y +
        textureMatrix.m[0][2] +
        textureMatrix.m[0][3],

        textureMatrix.m[1][0] * texCoord.x +
        textureMatrix.m[1][1] * texCoord.y +
        textureMatrix.m[1][2] +
        textureMatrix.m[1][3]
    };
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
    screen.x = vertex.x * screenScaleX + screenBiasX;
    screen.y = vertex.y * screenScaleY + screenBiasY;
    return screen;
}

inline f32 EdgeFunction(ZunVec3 v0, ZunVec3 v1, ZunVec3 v2) {
    return (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x);
}

inline float EdgeFunctionZunVec2(
    const ZunVec2& v0,
    const ZunVec2& v1,
    const ZunVec2& v2)
{
    return (v1.x - v0.x) * (v2.y - v0.y)
         - (v1.y - v0.y) * (v2.x - v0.x);
}

#define EdgeFunctionXY(x0,y0,x1,y1,x2,y2) (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0)

inline u8 AlphaBlendU8(u8 src, u8 dst, u8 a, u8 ia)
{
    return (u8)ZUN_MIN((((u32)src * a + (u32)dst * ia + 128) >> 8), 255);
    // return (u8)((src * a +
    //              dst * ia +
    //              128) >> 8);
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

    //NDCToScreenZunVec2 value fetched to value_x, value_y
    // v0 = NDCToScreenZunVec2(v0); became
    // f32 v0_x = v0.x * screenScaleX + screenBiasX;
    // f32 v0_y = v0.y * screenScaleY + screenBiasY;
    // then EdgeFunctionZunVec2 became EdgeFunctionXY
    #define s2dblock2\
        const f32 v0_x = v0.x * screenScaleX + screenBiasX;\
        const f32 v0_y = v0.y * screenScaleY + screenBiasY;\
        const f32 v1_x = v1.x * screenScaleX + screenBiasX;\
        const f32 v1_y = v1.y * screenScaleY + screenBiasY;\
        const f32 v2_x = v2.x * screenScaleX + screenBiasX;\
        const f32 v2_y = v2.y * screenScaleY + screenBiasY;\
        const f32 area = EdgeFunctionXY(v0_x, v0_y, v1_x, v1_y, v2_x, v2_y);\
        if (area == 0.0f)\
        {\
            index += increment;\
            continue;\
        }\
        const i32 xmin = ZUN_MAX(viewport[0],(i32)floor(ZUN_MIN3(v0_x, v1_x, v2_x)));\
        const i32 xmax = ZUN_MIN(viewport[0] + viewport[2] - 1,(i32)ceil(ZUN_MAX3(v0_x, v1_x, v2_x)));\
        const i32 ymin = ZUN_MAX(viewport[1],(i32)floor(ZUN_MIN3(v0_y, v1_y, v2_y)));\
        const i32 ymax = ZUN_MIN(viewport[1] + viewport[3] - 1,(i32)ceil(ZUN_MAX3(v0_y, v1_y, v2_y)));\
        const f32 vP_x = xmin+0.5f;\
        const f32 vP_y = ymin+0.5f;\
        const ZunVec3 edges = {\
            EdgeFunctionXY(v1_x, v1_y, v2_x, v2_y, vP_x, vP_y),\
            EdgeFunctionXY(v2_x, v2_y, v0_x, v0_y, vP_x, vP_y),\
            EdgeFunctionXY(v0_x, v0_y, v1_x, v1_y, vP_x, vP_y)\
        };\
        const ZunVec3 e_dx = {\
            v1_y - v2_y,\
            v2_y - v0_y,\
            v0_y - v1_y\
        };\
        const ZunVec3 e_dy = {\
            v2_x - v1_x,\
            v0_x - v2_x,\
            v1_x - v0_x\
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

    //unused
    #define s2dblock7_Ex\
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

    #define s2dblock7\
    if(frag_a == 255){\
        framebuffer[pixel] = RGBAToZunColor(frag_r,frag_g,frag_b,frag_a);\
        continue;\
    }\
    ZunColor dst = framebuffer[pixel];\
    u8 da = 255-frag_a;\
    framebuffer[pixel] =\
    (frag_a << 24) |\
    ((((frag_r * frag_a) +\
       (((dst >> 16) & 255) * da) +\
       128) >> 8) << 16) |\
    ((((frag_g * frag_a) +\
       (((dst >> 8) & 255) * da) +\
       128) >> 8) << 8) |\
    (((frag_b * frag_a) +\
       ((dst & 255) * da) +\
       128) >> 8);

    //fog
    #define fogdeclare2 fog_row += fog_dy,
    #define fogdeclare3 f32 fog = fog_row;
    #define fogdeclare4 fog += fog_dx,

    // #define fogdeclare1
    // #define fogdeclare2
    // #define fogdeclare3
    // #define fogdeclare4
    // #define fogdeclare5
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