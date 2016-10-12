TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "CalcHiZ";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

#include "light_constants.hlsl"

Texture2D <float2> depthTex : register(t0);

SamplerState samplerPointClamp : register(s0);

cbuffer materialBuffer : register(b1)
{
	float is_first;
	float _padding0;
	float _padding1;
	float _padding2;
};

float2 CalcHiZ(PI_PosTex input) : SV_TARGET
{
	float4 depthMin = depthTex.GatherRed(samplerPointClamp, input.tex);
	float4 depthMax = 0;
	if(is_first > 0)
		depthMax = depthMin;
	else
		depthMax = depthTex.GatherGreen(samplerPointClamp, input.tex);

	float2 res = 0;
	res.x = min( min(depthMin.x, depthMin.y), min(depthMin.z, depthMin.w) );
	res.y = max( max(depthMax.x, depthMax.y), max(depthMax.z, depthMax.w) );

	return res;
}
