#include "Software2DHeader.hpp"

#include <xtl.h>
#include <xgraphics.h>
#include <string.h>     // memset, memcpy
#include <algorithm>

struct COLORVERTEXS
{
    float       Position[3];
    DWORD       Color;
};

const char* g_strVertexShaderProgramS = 
" struct VS_IN                                 "  
" {                                            " 
"     float4 ObjPos   : POSITION;              "  // Object space position 
"     float2 Tex0   : TEXCOORD0;               "  // Vertex color          <= original float4 Color    : COLOR       
" };                                           " 
"                                              " 
" struct VS_OUT                                " 
" {                                            " 
"     float4 ProjPos  : POSITION;              "  // Projected space position 
"     float2 Tex0   : TEXCOORD0;               "  // <= original float4 Color    : COLOR
" };                                           "  
"                                              "  
" VS_OUT main( VS_IN In )                      "  
" {                                            "  
"     VS_OUT Out;                              "  
"     Out.ProjPos = float4(In.ObjPos.xy, 0.5, 1);"  // Transform vertex into
"     Out.Tex0 = In.Tex0;                      "  // Projected space and <= original Out.Color = In.Color
"     return Out;                              "  // Transfer color
" }                                            ";

//-------------------------------------------------------------------------------------
// Pixel shader
//-------------------------------------------------------------------------------------
const char* g_strPixelShaderProgramS = 
" sampler2D Frame : register(s0);			   "
" struct PS_IN                                 "
" {                                            "
"     float2 Tex0 : TEXCOORD0;                 "  // Interpolated color from <= original float4 Color : COLOR                    
" };                                           "  // the vertex shader
"                                              "  
" float4 main( PS_IN In ) : COLOR              "  
" {                                            "  
"     return tex2D(Frame, In.Tex0);            "  // Output color <= original return In.Color
" }                                            ";	

D3DVertexShader*       g_pVertexShaderS; // Vertex Shader
D3DPixelShader*        g_pPixelShaderS;  // Pixel Shader

Direct3D*  g_D3D    = NULL;
D3DDevice* g_Device = NULL;
D3DTexture* framebufferTexture = NULL;
D3DVertexDeclaration* screenDecl = NULL;
D3DVertexBuffer*      screenVB   = NULL;

struct ScreenVertex
{
    float x, y, z, w;
    float u, v;
};

static const D3DVERTEXELEMENT9 decl[] =
{
    { 0,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT,
      D3DDECLUSAGE_POSITION, 0 },

    { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT,
      D3DDECLUSAGE_TEXCOORD, 0 },

    D3DDECL_END()
};

GfxInterface *Software::Init()
{
    Software* gfx = new Software;

    XVIDEO_MODE videoMode;
    XGetVideoMode(&videoMode);

    g_GameWindow.ConfigureInit();
//    GAME_WINDOW_WIDTH_REAL  = min((int)videoMode.dwDisplayWidth, GAME_WINDOW_WIDTH);
//    GAME_WINDOW_HEIGHT_REAL = min((int)videoMode.dwDisplayHeight, GAME_WINDOW_HEIGHT);
	GAME_WINDOW_WIDTH_REAL  = 1280;
	GAME_WINDOW_HEIGHT_REAL = 1024;
    g_GameWindow.ConfigureView();

    const int width  = GAME_WINDOW_WIDTH_REAL;
    const int height = GAME_WINDOW_HEIGHT_REAL;

    g_D3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_D3D)
    {
        delete gfx;
        return NULL;
    }

    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));

    pp.BackBufferWidth            = width;
    pp.BackBufferHeight           = height;
    pp.BackBufferFormat           = D3DFMT_X8R8G8B8;
    pp.BackBufferCount            = 1;
    pp.EnableAutoDepthStencil     = FALSE;
    pp.SwapEffect                 = D3DSWAPEFFECT_DISCARD;
    pp.PresentationInterval       = D3DPRESENT_INTERVAL_ONE;

    if (FAILED(g_D3D->CreateDevice(
        0,
        D3DDEVTYPE_HAL,
        NULL,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &pp,
        &g_Device)))
    {
        delete gfx;
        return NULL;
    }

    if (FAILED(g_Device->CreateTexture(
        width,
        height,
        1,
        0,
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        &framebufferTexture,
        NULL)))
    {
        delete gfx;
        return NULL;
    }

    InitColorOpTable();

    gfx->boundTexture = NULL;
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

    gfx->noVertexBuffer =
        (g_Supervisor.cfg.opts &
        (1 << GCOS_DONT_USE_VERTEX_BUF)) != 0;

    gfx->noFog =
        (g_Supervisor.cfg.opts &
        (1 << GCOS_DONT_USE_FOG)) != 0;

    gfx->InitFlattenedMatrix();
    // Compile vertex shader.
    ID3DXBuffer* pVertexShaderCode;
    ID3DXBuffer* pVertexErrorMsg;
    HRESULT hr = D3DXCompileShader( g_strVertexShaderProgramS, 
                                    (UINT)strlen( g_strVertexShaderProgramS ),
                                    NULL, 
                                    NULL, 
                                    "main", 
                                    "vs_2_0", 
                                    0, 
                                    &pVertexShaderCode, 
                                    &pVertexErrorMsg, 
                                    NULL );
    if( FAILED(hr) )
    {
        if( pVertexErrorMsg )
            OutputDebugString( (char*)pVertexErrorMsg->GetBufferPointer() );
        return NULL;
    }    

    // Create vertex shader.
    g_Device->CreateVertexShader( (DWORD*)pVertexShaderCode->GetBufferPointer(), 
                                      &g_pVertexShaderS );
	pVertexShaderCode->Release();
	if (pVertexErrorMsg)
		pVertexErrorMsg->Release();


    // Compile pixel shader.
    ID3DXBuffer* pPixelShaderCode;
    ID3DXBuffer* pPixelErrorMsg;
    hr = D3DXCompileShader( g_strPixelShaderProgramS, 
                            (UINT)strlen( g_strPixelShaderProgramS ),
                            NULL, 
                            NULL, 
                            "main", 
                            "ps_2_0", 
                            0, 
                            &pPixelShaderCode, 
                            &pPixelErrorMsg,
                            NULL );
    if( FAILED(hr) )
    {
        if( pPixelErrorMsg )
            OutputDebugString( (char*)pPixelErrorMsg->GetBufferPointer() );
        return NULL;
    }

    // Create pixel shader.
    g_Device->CreatePixelShader( (DWORD*)pPixelShaderCode->GetBufferPointer(), 
                                     &g_pPixelShaderS );
    pPixelShaderCode->Release();
	if (pPixelErrorMsg)
		pPixelErrorMsg->Release();

    // Define the vertex elements and
    // Create a vertex declaration from the element descriptions.
	D3DVERTEXELEMENT9 VertexElements[] =
	{
		{ 0,  0, D3DDECLTYPE_FLOAT4,
		  D3DDECLMETHOD_DEFAULT,
		  D3DDECLUSAGE_POSITION, 0 },

		{ 0, 16, D3DDECLTYPE_FLOAT2,
		  D3DDECLMETHOD_DEFAULT,
		  D3DDECLUSAGE_TEXCOORD, 0 },

		D3DDECL_END()
	};
    g_Device->CreateVertexDeclaration( VertexElements, &screenDecl );

    // Create the vertex buffer. Here we are allocating enough memory
    // (from the default pool) to hold all our 3 custom vertices. 

    if( FAILED( g_Device->CreateVertexBuffer(
			4 * sizeof(ScreenVertex),
			D3DUSAGE_WRITEONLY,
			NULL,
			D3DPOOL_MANAGED,
			&screenVB,
			NULL) ) )
        return NULL;

	ScreenVertex* v;
	if (FAILED(screenVB->Lock(0, 0, (void**)&v, 0)))
	{
		delete gfx;
		return NULL;
	}

	// Fullscreen quad (triangle strip)
	v[0].x = -1.0f;
	v[0].y =  1.0f;
	v[0].z =  0.0f;
	v[0].w =  1.0f;
	v[0].u =  0.0f;
	v[0].v =  0.0f;

	v[1].x =  1.0f;
	v[1].y =  1.0f;
	v[1].z =  0.0f;
	v[1].w =  1.0f;
	v[1].u =  1.0f;
	v[1].v =  0.0f;

	v[2].x = -1.0f;
	v[2].y = -1.0f;
	v[2].z =  0.0f;
	v[2].w =  1.0f;
	v[2].u =  0.0f;
	v[2].v =  1.0f;

	v[3].x =  1.0f;
	v[3].y = -1.0f;
	v[3].z =  0.0f;
	v[3].w =  1.0f;
	v[3].u =  1.0f;
	v[3].v =  1.0f;

	screenVB->Unlock();
	g_Device->SetRenderState(D3DRS_ZENABLE, FALSE);
	g_Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	g_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	g_Device->SetVertexShader(g_pVertexShaderS);
	g_Device->SetPixelShader(g_pPixelShaderS);
	g_Device->SetVertexDeclaration(screenDecl);
	g_Device->SetStreamSource(0, screenVB, 0, sizeof(ScreenVertex));
    return gfx;
}


bool Software::GameLoop()
{
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(state));

    if (XInputGetState(0, &state) == ERROR_SUCCESS)
    {
        // BACK button quits
        if (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
            return false;
    }

    return true;
}

void Software::Exit()
{
    if (framebufferTexture)
    {
        framebufferTexture->Release();
        framebufferTexture = NULL;
    }

    delete[] framebuffer;
    framebuffer = NULL;

    if (g_Device)
    {
        g_Device->Release();
        g_Device = NULL;
    }

    if (g_D3D)
    {
        g_D3D->Release();
        g_D3D = NULL;
    }
}
/*
u16 current=0;
void Software::SwapBuffers()
{
    D3DLOCKED_RECT rect;

    if (FAILED(framebufferTexture->LockRect(0, &rect, NULL, 0)))
        return;

    const int width  = GAME_WINDOW_WIDTH_REAL;
    const int height = GAME_WINDOW_HEIGHT_REAL;
	memcpy(
		rect.pBits,
		framebuffer,
		width * height * sizeof(u32));

    framebufferTexture->UnlockRect(0);
    g_Device->Clear(
        0, NULL,
        D3DCLEAR_TARGET,
        D3DCOLOR_XRGB(0,0,0),
        1.0f, 0);

	g_Device->BeginScene();

	g_Device->SetTexture(0, framebufferTexture);

	g_Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

	g_Device->EndScene();

    g_Device->Present(NULL,NULL,NULL,NULL);
}
*/
void Software::SwapBuffers()
{
    D3DLOCKED_RECT rect;

    if (FAILED(framebufferTexture->LockRect(0, &rect, NULL, 0)))
        return;

    const i32 width  = GAME_WINDOW_WIDTH_REAL;
    const i32 height = GAME_WINDOW_HEIGHT_REAL;
	u32* dst = static_cast<u32*>(rect.pBits);

	i32 i=0;
	for (i32 y = 0; y < height/2; y++){
		for (i32 x = 0; x < width; ++x,++i)
		{
			dst[i] = 0XFFFFFFFF;
		}
		for (i32 x = 0; x < width; ++x,++i)
		{
			dst[i] = 0XFFFF00FF;
		}
	}

    framebufferTexture->UnlockRect(0);

    g_Device->Clear(
        0,
        NULL,
        D3DCLEAR_TARGET,
        0,
        1.0f,
        0);

    g_Device->BeginScene();

    g_Device->SetTexture(0, framebufferTexture);

    g_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    g_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    g_Device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    g_Device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    g_Device->SetVertexDeclaration(screenDecl);
    g_Device->SetStreamSource(0, screenVB, 0, sizeof(ScreenVertex));

    g_Device->SetVertexShader(g_pVertexShaderS);
    g_Device->SetPixelShader(g_pPixelShaderS);

    g_Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    g_Device->EndScene();

    g_Device->Present(NULL, NULL, NULL, NULL);
}

#include "Software2DCore.hpp"