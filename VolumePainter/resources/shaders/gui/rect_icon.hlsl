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

Texture2D maskTex : register(t0); 
SamplerState samplerPointClamp : register(s0);

cbuffer materialBuffer : register(b0)
{
    float4 clip;
    float4 iconColor;
};

float4 PS(PI_PosTex input) : SV_TARGET
{
	if( input.pos.x < clip.r ||
		input.pos.y < clip.g ||
		input.pos.x > clip.b ||
		input.pos.y > clip.a )
		discard;

	float mask = maskTex.Sample(samplerPointClamp, input.tex).r;
	float4 color = iconColor;
	color.a *= mask;
	if( color.a == 0 )
		discard;
	
	return color;
}