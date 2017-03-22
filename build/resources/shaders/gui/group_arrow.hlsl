TECHNIQUE_DEFAULT
{
	Queue = GUI_2D_FONT;

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
	float4 arrowColor;
	float4 angleRot;
};

float4 PS(PI_PosTex input) : SV_TARGET
{
	if( input.pos.x < clip.r ||
		input.pos.y < clip.g ||
		input.pos.x > clip.b ||
		input.pos.y > clip.a )
		discard;

	float2 coords = input.tex - float2(0.5f, 0.5f);
	
	float sinRot = sin(angleRot.r);
	float cosRot = cos(angleRot.r);

	float2 rotCoords;
	rotCoords.x = coords.x * cosRot - coords.y * sinRot;
	rotCoords.y = coords.y * cosRot + coords.x * sinRot;
	rotCoords += float2(0.5f, 0.5f);

	float mask = maskTex.Sample(samplerPointClamp, rotCoords).r;
	float4 color = arrowColor * mask;
	if( color.a == 0 )
		discard;
	
	return color;
}