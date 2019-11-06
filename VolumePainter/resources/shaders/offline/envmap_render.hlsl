TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "PS";
}

#include "../common/math.hlsl"
#include "../common/structs.hlsl"

Texture2D shaderTexture : register(t0); 
SamplerState samplerPointClamp : register(s0); 

cbuffer materialBuffer : register(b1)
{
	float farClip;
	float _padding0;
	float _padding1;
	float _padding2;
};

float4 PS(PI_PosTex input) : SV_TARGET
{
	float4 color = shaderTexture.Sample(samplerPointClamp, input.tex);
	if(color.a == 1.0)
	{
		color.a = 0;		// плавно гасить в зав-ти от подхождения к краю объема
	}
	else
	{
		color.a = 1;
	}
	return color;
} 
