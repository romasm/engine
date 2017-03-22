#include "../common/math.hlsl"
#include "../common/structs.hlsl"

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

TECHNIQUE_DEFAULT
{
	Queue = GUI_2D;

	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "PS";

	BlendEnable = true;
	BlendOp = ADD;
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
}