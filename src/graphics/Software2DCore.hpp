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
    fogColor_r = (color >> 16) & 0xFF;
    fogColor_g = (color >> 8) & 0xFF;
    fogColor_b = color & 0xFF;
    fogColor_a = color >> 24;
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
    textureFactor_r = (factor >> 16) & 0xFF;
    textureFactor_g = (factor >> 8) & 0xFF;
    textureFactor_b = factor & 0xFF;
    textureFactor_a = factor >> 24;
}

void Software::SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix)
{
    switch (type) {
        case MATRIX_MODEL:
            modelm00 = matrix.m[0][0]; modelm01 = matrix.m[0][1]; modelm02 = matrix.m[0][2]; modelm03 = matrix.m[0][3];
            modelm10 = matrix.m[1][0]; modelm11 = matrix.m[1][1]; modelm12 = matrix.m[1][2]; modelm13 = matrix.m[1][3];
            modelm20 = matrix.m[2][0]; modelm21 = matrix.m[2][1]; modelm22 = matrix.m[2][2]; modelm23 = matrix.m[2][3];
            modelm30 = matrix.m[3][0]; modelm31 = matrix.m[3][1]; modelm32 = matrix.m[3][2]; modelm33 = matrix.m[3][3];
            break;

        case MATRIX_VIEW:
            viewm00 = matrix.m[0][0]; viewm01 = matrix.m[0][1]; viewm02 = matrix.m[0][2]; viewm03 = matrix.m[0][3];
            viewm10 = matrix.m[1][0]; viewm11 = matrix.m[1][1]; viewm12 = matrix.m[1][2]; viewm13 = matrix.m[1][3];
            viewm20 = matrix.m[2][0]; viewm21 = matrix.m[2][1]; viewm22 = matrix.m[2][2]; viewm23 = matrix.m[2][3];
            viewm30 = matrix.m[3][0]; viewm31 = matrix.m[3][1]; viewm32 = matrix.m[3][2]; viewm33 = matrix.m[3][3];
            break;
        case MATRIX_PROJECTION:
            vm00 = viewm00*modelm00 + viewm10*modelm01 + viewm20*modelm02 + viewm30*modelm03;
            vm01 = viewm01*modelm00 + viewm11*modelm01 + viewm21*modelm02 + viewm31*modelm03;
            vm02 = viewm02*modelm00 + viewm12*modelm01 + viewm22*modelm02 + viewm32*modelm03;
            vm03 = viewm03*modelm00 + viewm13*modelm01 + viewm23*modelm02 + viewm33*modelm03;

            vm10 = viewm00*modelm10 + viewm10*modelm11 + viewm20*modelm12 + viewm30*modelm13;
            vm11 = viewm01*modelm10 + viewm11*modelm11 + viewm21*modelm12 + viewm31*modelm13;
            vm12 = viewm02*modelm10 + viewm12*modelm11 + viewm22*modelm12 + viewm32*modelm13;
            vm13 = viewm03*modelm10 + viewm13*modelm11 + viewm23*modelm12 + viewm33*modelm13;

            vm20 = viewm00*modelm20 + viewm10*modelm21 + viewm20*modelm22 + viewm30*modelm23;
            vm21 = viewm01*modelm20 + viewm11*modelm21 + viewm21*modelm22 + viewm31*modelm23;
            vm22 = viewm02*modelm20 + viewm12*modelm21 + viewm22*modelm22 + viewm32*modelm23;
            vm23 = viewm03*modelm20 + viewm13*modelm21 + viewm23*modelm22 + viewm33*modelm23;

            vm30 = viewm00*modelm30 + viewm10*modelm31 + viewm20*modelm32 + viewm30*modelm33;
            vm31 = viewm01*modelm30 + viewm11*modelm31 + viewm21*modelm32 + viewm31*modelm33;
            vm32 = viewm02*modelm30 + viewm12*modelm31 + viewm22*modelm32 + viewm32*modelm33;
            vm33 = viewm03*modelm30 + viewm13*modelm31 + viewm23*modelm32 + viewm33*modelm33;

            mvpm00 = matrix.m[0][0]*vm00 + matrix.m[1][0]*vm01 + matrix.m[2][0]*vm02 + matrix.m[3][0]*vm03;
            mvpm01 = matrix.m[0][1]*vm00 + matrix.m[1][1]*vm01 + matrix.m[2][1]*vm02 + matrix.m[3][1]*vm03;
            mvpm02 = matrix.m[0][2]*vm00 + matrix.m[1][2]*vm01 + matrix.m[2][2]*vm02 + matrix.m[3][2]*vm03;
            mvpm03 = matrix.m[0][3]*vm00 + matrix.m[1][3]*vm01 + matrix.m[2][3]*vm02 + matrix.m[3][3]*vm03;

            mvpm10 = matrix.m[0][0]*vm10 + matrix.m[1][0]*vm11 + matrix.m[2][0]*vm12 + matrix.m[3][0]*vm13;
            mvpm11 = matrix.m[0][1]*vm10 + matrix.m[1][1]*vm11 + matrix.m[2][1]*vm12 + matrix.m[3][1]*vm13;
            mvpm12 = matrix.m[0][2]*vm10 + matrix.m[1][2]*vm11 + matrix.m[2][2]*vm12 + matrix.m[3][2]*vm13;
            mvpm13 = matrix.m[0][3]*vm10 + matrix.m[1][3]*vm11 + matrix.m[2][3]*vm12 + matrix.m[3][3]*vm13;

            mvpm20 = matrix.m[0][0]*vm20 + matrix.m[1][0]*vm21 + matrix.m[2][0]*vm22 + matrix.m[3][0]*vm23;
            mvpm21 = matrix.m[0][1]*vm20 + matrix.m[1][1]*vm21 + matrix.m[2][1]*vm22 + matrix.m[3][1]*vm23;
            mvpm22 = matrix.m[0][2]*vm20 + matrix.m[1][2]*vm21 + matrix.m[2][2]*vm22 + matrix.m[3][2]*vm23;
            mvpm23 = matrix.m[0][3]*vm20 + matrix.m[1][3]*vm21 + matrix.m[2][3]*vm22 + matrix.m[3][3]*vm23;

            mvpm30 = matrix.m[0][0]*vm30 + matrix.m[1][0]*vm31 + matrix.m[2][0]*vm32 + matrix.m[3][0]*vm33;
            mvpm31 = matrix.m[0][1]*vm30 + matrix.m[1][1]*vm31 + matrix.m[2][1]*vm32 + matrix.m[3][1]*vm33;
            mvpm32 = matrix.m[0][2]*vm30 + matrix.m[1][2]*vm31 + matrix.m[2][2]*vm32 + matrix.m[3][2]*vm33;
            mvpm33 = matrix.m[0][3]*vm30 + matrix.m[1][3]*vm31 + matrix.m[2][3]*vm32 + matrix.m[3][3]*vm33;
            break;
        case MATRIX_TEXTURE:
            tm00 = matrix.m[0][0];
            tm01 = matrix.m[0][1];
            tm02 = matrix.m[0][2] + matrix.m[0][3];
            tm10 = matrix.m[1][0];
            tm11 = matrix.m[1][1];
            tm12 = matrix.m[1][2] + matrix.m[1][3];
            break;
    }
}
void Software::Set2D(bool is2D){
    render2D = is2D;
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

void Software::ConvertToARGB8888Pitch(
    u32 width,
    u32 height,
    PixelFormat fmt,
	PixelDataType type,
    const void* srcData,
    u32 srcPitchBytes,
    u32* dstData,
    u32 dstPitchBytes)
{
    const u8* srcBase = (const u8*)srcData;

    if (fmt == PIXEL_RGBA)
    {
		if (type == PIXEL_UNSIGNED_SHORT_4_4_4_4)
		{
			for (u32 y = 0; y < height; ++y)
			{
				const u16* src = (u16*)(srcBase + y * srcPitchBytes);
				u32* dst = (u32*)((u8*)dstData + y * dstPitchBytes);
				const u16* s = src;

				for (u32 x = 0; x < width; ++x, s++)
				{
					u8 r = (((*s)>>12)&0xF)*0x10;
					u8 g = (((*s)>>8)&0xF)*0x10;
					u8 b = (((*s)>>4)&0xF)*0x10;
					u8 a = ((*s)&0xF)*0x10;

					*dst++ = ((u32)a << 24) | ((u32)r << 16) | ((u32)g << 8) | b;
				}
			}
		}
		else
		{
			for (u32 y = 0; y < height; ++y)
			{
				const u8* src = srcBase + y * srcPitchBytes;
				u32* dst = (u32*)((u8*)dstData + y * dstPitchBytes);
				const u8* s = src;

				for (u32 x = 0; x < width; ++x, s += 4)
				{
					*dst++ = (s[3] << 24) | (s[0] << 16) | (s[1] << 8) | s[2];
				}
			}
		}
    }
    else if (fmt == PIXEL_RGB)
    {
		if (type == PIXEL_UNSIGNED_SHORT_5_6_5)
		{
			for (u32 y = 0; y < height; ++y)
			{
				const u16* src = (u16*)(srcBase + y * srcPitchBytes);
				u32* dst = (u32*)((u8*)dstData + y * dstPitchBytes);
				const u16* s = src;

				for (u32 x = 0; x < width; ++x, s++)
				{
					u8 r5 = ((*s)&0xF800)>>11;
					u8 g6 = ((*s)&0x07E0)>>5;
					u8 b5 = ((*s)&0x001F);
					u8 r8 = (r5<<3)|(r5>>2);
					u8 g8 = (g6<<2)|(g6>>4);
					u8 b8 = (b5<<3)|(b5>>2);
					*dst++ = (255 << 24) | (r8 << 16) | (g8 << 8) | b8;
				}
			}
		}
		else
		{
			for (u32 y = 0; y < height; ++y)
			{
				const u8* src = srcBase + y * srcPitchBytes;
				u32* dst = (u32*)((u8*)dstData + y * dstPitchBytes);
				const u8* s = src;

				for (u32 x = 0; x < width; ++x, s += 3)
				{
					*dst++ = (255 << 24) | (s[0] << 16) | (s[1] << 8) | s[2];
				}
				// for (u32 x = 0; x < width; ++x)
				// {
				//     dst[x] =
				//         (255 << 24) |
				//         (s[0] << 16) |
				//         (s[1] << 8)  |
				//         (s[2]);
				//     s += 3;
				// }
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
                type,
                data,
                width * bpp,
                &boundTexture->texels[0],
                width * 4 //sizeof(u32)
            );
        }
        boundTexture->width = width;
        boundTexture->shift = 0;
        for (u32 t = width; t > 1; t >>= 1)
            ++boundTexture->shift;
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
            PIXEL_UNSIGNED_BYTE,
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

inline u8 AlphaBlendU8(u8 src, u8 dst, u8 a, u8 ia)
{
    u32 ret = (src * a + dst * ia + 128) >> 8;
    return ret | -(ret > 255);
}

#define EdgeFunctionXY(x0,y0,x1,y1,x2,y2) ((x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0))

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
 
    #define fogdeclare2 fog_row += fog_dy;
    #define fogdeclare3 f32 fog = fog_row;
    #define fogdeclare4 fog += fog_dx;
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
        frag_r = (u8)((frag_r * invT + fogColor_r * t) >> 8);\
        frag_g = (u8)((frag_g * invT + fogColor_g * t) >> 8);\
        frag_b = (u8)((frag_b * invT + fogColor_b * t) >> 8);\
    }
    //disable it
    #undef fogdeclare1
    #undef fogdeclare2
    #undef fogdeclare3
    #undef fogdeclare4
    #undef fogdeclare5
    #define fogdeclare1
    #define fogdeclare2
    #define fogdeclare3
    #define fogdeclare4
    #define fogdeclare5

void Software::Draw(PrimitiveType type, i32 start, i32 count)
{
    if (count == 0) return;
    u32 increment = type == PRIM_TRIANGLE_STRIP ? 1 : 3;
    u32 index = start;
    u32 last_index = start + count;
    if(type == PRIM_TRIANGLE_STRIP) last_index -= 2;

    if(useTexCoord&&boundTexture){
        //precompute colorOp
        const u8 *tableR = ColorOpTable[colorOp][textureFactor_r];
        const u8 *tableG = ColorOpTable[colorOp][textureFactor_g];
        const u8 *tableB = ColorOpTable[colorOp][textureFactor_b];
        const u8 *tableA = ColorOpTable[colorOp][textureFactor_a];
        const u8 *tableDA = ColorDA[blendMode]; 

        const u8* tData = (u8*)texCoordData;
        const u8* vData = (u8*)vertexData;

        const u32* texels = &boundTexture->texels[0];
        const f32 ftexW = (f32)boundTexture->width;
        const f32 ftexH = (f32)boundTexture->height;
        const i32 texMaskX = boundTexture->width-1;
        const i32 texMaskY = boundTexture->height-1;
        const u8 texShift = boundTexture->shift;

        while (index < last_index) {
            const u8* vp = vData + vertexStride * index;
            const ZunVec3* v0 = (const ZunVec3*)(vData + vertexStride * index);
            const ZunVec3* v1 = (const ZunVec3*)((const u8*)v0 + vertexStride);
            const ZunVec3* v2 = (const ZunVec3*)((const u8*)v1 + vertexStride);

            //completely different takes for ProjectToNDCZunVec2
            f32 vx, vy, vz;
            #define vGetx mvpm00 * vx + mvpm10 * vy + mvpm20 * vz + mvpm30
            #define vGety mvpm01 * vx + mvpm11 * vy + mvpm21 * vz + mvpm31
            #define vGetz mvpm02 * vx + mvpm12 * vy + mvpm22 * vz + mvpm32
            #define vGetw mvpm03 * vx + mvpm13 * vy + mvpm23 * vz + mvpm33
            vx = v0->x;vy = v0->y;vz = v0->z;
            f32 v0_x = vGetx;
            f32 v0_y = vGety;
            // f32 viewZ0 = vGetz;
            const f32 v0_w = vGetw;
            if (v0_w > 0.0f) {const f32 invw = 1.0f / v0_w; v0_x *= invw; v0_y *= invw; }

            vx = v1->x;vy = v1->y;vz = v1->z;
            f32 v1_x = vGetx;
            f32 v1_y = vGety;
            // f32 viewZ1 = vGetz;
            const f32 v1_w = vGetw;
            if (v1_w > 0.0f) {const f32 invw = 1.0f / v1_w; v1_x *= invw; v1_y *= invw; }

            vx = v2->x;vy = v2->y;vz = v2->z;
            f32 v2_x = vGetx;
            f32 v2_y = vGety;
            // f32 viewZ2 = vGetz;
            const f32 v2_w = vGetw;
            if (v2_w > 0.0f) {const f32 invw = 1.0f / v2_w; v2_x *= invw; v2_y *= invw; }

            const u8* tcp = tData + texCoordStride * index;
            const ZunVec2* ptc = (const ZunVec2*)tcp;
            #define tcGetx (tm00 * ptc->x + tm01 * ptc->y + tm02) * ftexW
            #define tcGety (tm10 * ptc->x + tm11 * ptc->y + tm12) * ftexH
            f32 tc0x = tcGetx;
            f32 tc0y = tcGety;
            tcp += texCoordStride;
            ptc = (const ZunVec2*)tcp;
            f32 tc1x = tcGetx;
            f32 tc1y = tcGety;
            tcp += texCoordStride;
            ptc = (const ZunVec2*)tcp;
            f32 tc2x = tcGetx;
            f32 tc2y = tcGety;
            if (type == PRIM_TRIANGLE_STRIP && ((index - start) & 1))
            {
                f32 tf;
                tf = v0_x; v0_x = v1_x; v1_x = tf;
                tf = v0_y; v0_y = v1_y; v1_y = tf;
                tf = tc0x; tc0x = tc1x; tc1x = tf;
                tf = tc0y; tc0y = tc1y; tc1y = tf;
                // tf = viewZ0; viewZ0 = viewZ1; viewZ1 = tf;
            }

            if (EdgeFunctionXY(v0_x, v0_y, v1_x, v1_y, v2_x, v2_y) < 0)
            {
                f32 tf;
                tf = v1_x; v1_x = v2_x; v2_x = tf;
                tf = v1_y; v1_y = v2_y; v2_y = tf;
                tf = tc1x; tc1x = tc2x; tc2x = tf;
                tf = tc1y; tc1y = tc2y; tc2y = tf;
                // tf = viewZ1; viewZ1 = viewZ2; viewZ2 = tf;
            }
            //NDCToScreenZunVec2 value fetched to value_x, value_y
            // v0 = NDCToScreenZunVec2(v0); became
            // f32 v0_x = v0.x * screenScaleX + screenBiasX;
            // f32 v0_y = v0.y * screenScaleY + screenBiasY;
            // then EdgeFunctionZunVec2 became EdgeFunctionXY
            v0_x = v0_x * screenScaleX + screenBiasX;\
            v0_y = v0_y * screenScaleY + screenBiasY;\
            v1_x = v1_x * screenScaleX + screenBiasX;\
            v1_y = v1_y * screenScaleY + screenBiasY;\
            v2_x = v2_x * screenScaleX + screenBiasX;\
            v2_y = v2_y * screenScaleY + screenBiasY;\
            const f32 area = EdgeFunctionXY(v0_x, v0_y, v1_x, v1_y, v2_x, v2_y);\
            if (area == 0.0f)\
            {\
                index += increment;\
                continue;\
            }\
            const f32 invArea = 1.0f / area;\
            const i32 xmin = ZUN_MAX(viewport[0],(i32)floor(ZUN_MIN3(v0_x, v1_x, v2_x)));\
            const i32 xmax = ZUN_MIN(viewport[0] + viewport[2] - 1,(i32)ceil(ZUN_MAX3(v0_x, v1_x, v2_x)));\
            const i32 ymin = ZUN_MAX(viewport[1],(i32)floor(ZUN_MIN3(v0_y, v1_y, v2_y)));\
            const i32 ymax = ZUN_MIN(viewport[1] + viewport[3] - 1,(i32)ceil(ZUN_MAX3(v0_y, v1_y, v2_y)));\
            const f32 vP_x = xmin+0.5f;\
            const f32 vP_y = ymin+0.5f;\

            //barycentrics
            const f32 w0_dx = invArea * (v1_y - v2_y);\
            const f32 w1_dx = invArea * (v2_y - v0_y);\
            const f32 w2_dx = invArea * (v0_y - v1_y);\
            const f32 w0_dy = invArea * (v2_x - v1_x);\
            const f32 w1_dy = invArea * (v0_x - v2_x);\
            const f32 w2_dy = invArea * (v1_x - v0_x);\
            f32 w0_row = invArea * EdgeFunctionXY(v1_x, v1_y, v2_x, v2_y, vP_x, vP_y);\
            f32 w1_row = invArea * EdgeFunctionXY(v2_x, v2_y, v0_x, v0_y, vP_x, vP_y);\
            f32 w2_row = invArea * EdgeFunctionXY(v0_x, v0_y, v1_x, v1_y, vP_x, vP_y);\

            const f32 fu_row =\
                tc0x*w0_row +\
                tc1x*w1_row +\
                tc2x*w2_row;\
            const f32 fv_row =\
                tc0y*w0_row +\
                tc1y*w1_row +\
                tc2y*w2_row;\
            const f32 fu_dx =\
                tc0x*w0_dx +\
                tc1x*w1_dx +\
                tc2x*w2_dx;\
            const f32 fu_dy =\
                tc0x*w0_dy +\
                tc1x*w1_dy +\
                tc2x*w2_dy;\
            const f32 fv_dx =\
                tc0y*w0_dx +\
                tc1y*w1_dx +\
                tc2y*w2_dx;\
            const f32 fv_dy =\
                tc0y*w0_dy +\
                tc1y*w1_dy +\
                tc2y*w2_dy;

            fixed32 u_row = (fixed32)(fu_row * FIXED_ONE);
            fixed32 v_row = (fixed32)(fv_row * FIXED_ONE);
            const fixed32 u_dx = (fixed32)(fu_dx * FIXED_ONE);
            const fixed32 v_dx = (fixed32)(fv_dx * FIXED_ONE);
            const fixed32 u_dy = (fixed32)(fu_dy * FIXED_ONE);
            const fixed32 v_dy = (fixed32)(fv_dy * FIXED_ONE);

            fogdeclare1
            //add fb_row instead of Y multiplication
            ZunColor *fb_row = framebuffer + ymin * GAME_WINDOW_WIDTH_REAL;
            for (i32 y = ymin; y <= ymax; ++y)
            {
                fogdeclare3
                fixed32 u = u_row;
                fixed32 v = v_row;
                ZunColor *fb = fb_row + xmin;
                f32 w0 = w0_row; f32 w1 = w1_row; f32 w2 = w2_row;
                for (i32 x = xmin; x <= xmax; ++x)
                {
                    // barycentric inside test (fast reject first)
                    if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
                    {
                        //directly inside
                        u32 tu = (u >> 16) & texMaskX;
                        u32 tv = (v >> 16) & texMaskY;
                        const ZunColor frag = texels[(tv << texShift) + tu];

                        u8 frag_a=tableA[frag >> 24];
                        if (frag_a >= alphaThreshold){
                            u8 frag_r=tableR[(frag >> 16) & 0xFF];
                            u8 frag_g=tableG[(frag >> 8) & 0xFF];
                            u8 frag_b=tableB[frag & 0xFF];

                            fogdeclare5
                            // const ZunColor dst = *fb;
                            // const u8 da = tableDA[frag_a];
                            // frag_r = AlphaBlendU8(frag_r, (dst >> 16) & 0xFF, frag_a, da);
                            // frag_g = AlphaBlendU8(frag_g, (dst >> 8) & 0xFF, frag_a, da);
                            // frag_b = AlphaBlendU8(frag_b, dst & 0xFF, frag_a, da);

                            *fb = (ZunColor)(
                                (frag_a << 24) |
                                (frag_r << 16) |
                                (frag_g << 8)  |
                                frag_b);
                        }
                    }
                    ++fb;
                    u += u_dx;
                    v += v_dx;
                    fogdeclare4
                    w0 += w0_dx, w1 += w1_dx, w2 += w2_dx;
                }

                u_row += u_dy;
                v_row += v_dy;
                fb_row += GAME_WINDOW_WIDTH_REAL;
                fogdeclare2
                w0_row += w0_dy, w1_row += w1_dy, w2_row += w2_dy;
            }
            index += increment;
        }
    }
}