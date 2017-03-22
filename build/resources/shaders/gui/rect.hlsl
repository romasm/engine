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

#include "../common/math.hlsl"
#include "../common/structs.hlsl"

cbuffer materialBuffer : register(b0)
{
    float4 clip;
    float4 border;
    float4 bgColor;
    float4 borderColor;
};

float4 PS(PI_PosTex input) : SV_TARGET
{
	if( input.pos.x < clip.r ||
		input.pos.y < clip.g ||
		input.pos.x > clip.b ||
		input.pos.y > clip.a )
		discard;

	float4 color = bgColor;
	if( border.x == 0 )
		return color;

	if( input.tex.x <= border.x || 
		input.tex.y <= border.y ||
		input.tex.x >= border.z || 
		input.tex.y >= border.w )
		color = borderColor;
		
	if( color.a == 0 )
		discard;
	
	return color;
}