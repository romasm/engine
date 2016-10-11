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

Texture2D depthTex : register(t0);

SamplerState samplerPointClamp : register(s0);

float CalcHiZ(PI_PosTex input) : SV_TARGET
{
	float4 depth = depthTex.GatherRed(samplerPointClamp, input.tex);
	return min( min(depth.x, depth.y), min(depth.z, depth.w) );
}
