TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "BloomFind";
}

#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

Texture2D sceneTex : register(t0); 
Texture2D lumTex : register(t1); 

SamplerState samplerPointClamp : register(s0);
SamplerState samplerBilinearClamp : register(s1);

cbuffer materialBuffer : register(b1)
{
	float lumBias;
	float lumMax;
	float _padding0;
	float _padding1;
};

float4 BloomFind(PI_PosTex input) : SV_TARGET
{
	float3 color = sceneTex.SampleLevel(samplerBilinearClamp, input.tex, 0).rgb;
	
	const float2 lum_coords = float2(0.5f, 0.5f);
	float averageFrameLum = lumTex.Sample(samplerPointClamp, lum_coords).r;
	
	color -= averageFrameLum + lumBias;
	color = max(0, color);
	
	float maxChannel = max(max(color.r, color.g), color.b);
	float d = saturate(lumMax / maxChannel);
	color = color * d;
	
	return float4(color, 0);
}
