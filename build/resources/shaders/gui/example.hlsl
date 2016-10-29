#include "../common/math.hlsl"

TECHNIQUE_DEFAULT
{
	Queue = GUI_2D;
	/*
		GUI_2D,
		GUI_2D_FONT,

		GUI_3D,
		GUI_3D_OVERLAY,

		SC_OPAQUE,
		SC_ALPHATEST,
		SC_ALPHA,
		SC_TRANSPARENT,
	*/

	DepthEnable = false;
	DepthWrite = false;
	DepthFunc = ALWAYS;
	/*
	  NEVER         = 1,
	  LESS          = 2,
	  EQUAL         = 3,
	  LESS_EQUAL     = 4,
	  GREATER       = 5,
	  NOT_EQUAL      = 6,
	  GREATER_EQUAL  = 7,
	  ALWAYS        = 8,
	*/

	StencilEnable = false;
	StencilReadMask = 0b00000000;
	StencilWriteMask = 0b11111111;

	FrontFace.StencilFunc = ALWAYS;
	FrontFace.StencilFailOp = KEEP;
	FrontFace.StencilDepthFailOp = INCR;
	FrontFace.StencilPassOp = KEEP;

	BackFace.StencilFunc = ALWAYS;
	BackFace.StencilFailOp = KEEP;
	BackFace.StencilDepthFailOp = INCR;
	BackFace.StencilPassOp = KEEP;
	/*
	KEEP	= 1,
    ZERO	= 2,
    REPLACE	= 3,
    INCR_SAT	= 4,
    DECR_SAT	= 5,
    INVERT	= 6,
    INCR	= 7,
    DECR	= 8
	*/

	BlendEnable = true;
	BlendOp = ADD;
	BlendOpAlpha = ADD;
	/* 
	  ADD          = 1,
	  SUBTRACT     = 2,
	  REV_SUBTRACT  = 3,
	  MIN          = 4,
	  MAX          = 5,
	*/
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
	SrcBlendAlpha = ONE;
	DestBlendAlpha = ZERO;
	/*
	ZERO             = 1,
	ONE              = 2,
	SRC_COLOR         = 3,
	INV_SRC_COLOR      = 4,
	SRC_ALPHA         = 5,
	INV_SRC_ALPHA      = 6,
	DEST_ALPHA        = 7,
	INV_DEST_ALPHA     = 8,
	DEST_COLOR        = 9,
	INV_DEST_COLOR     = 10,
	SRC_ALPHA_SAT      = 11,
	BLEND_FACTOR     = 12,
	INV_BLEND_FACTOR  = 13,
	SRC1_COLOR      = 14,
	INV_SRC1_COLOR   = 15,
	SRC1_ALPHA        = 16,
	INV_SRC1_ALPHA     = 17,
	*/

	RenderTargetWriteMask = ALL;
	/*
	RED
	GREEN
	BLUE
	ALPHA
	ALL
	*/

	FillMode = SOLID;
	/*
	WIREFRAME	= 2,
    SOLID	= 3
	*/

	CullMode = BACK;
	/*
	NONE	= 1,
    FRONT	= 2,
    BACK	= 3
	*/

	AntialiasedLineEnable = true;
	DepthBias = 0;
	DepthBiasClamp = 0.0;
	DepthClipEnable = true;
	FrontCounterClockwise = false;
	MultisampleEnable = false;
	ScissorEnable = false;
	SlopeScaledDepthBias = 0.0;

	VertexShader = "../resources/shaders/tech/screen_plane_hud VS";
	PixelShader = PS;

	HullShader = NULL; // default
	DomainShader = NULL; // default
	GeometryShader = NULL; // default
}

//~ CODE

SamplerState samplerPointClamp : register(s0);

/*
Texture2D sys_systemTex : register(t0);
Texture2D userTex : register(t1);
*/

cbuffer materialBuffer : register(b0)
{
    float4 clip_rect;
    float4 color;
};

float4 PS(PI_PosTex input) : SV_TARGET
{
	if( input.pos.x < clip_rect.r ||
		input.pos.y < clip_rect.g ||
		input.pos.x > clip_rect.b ||
		input.pos.y > clip_rect.a )
		discard;

	if( color.a == 0 )
		discard;
		
	return color;
}