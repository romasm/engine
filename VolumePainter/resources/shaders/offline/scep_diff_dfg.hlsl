TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "DFGGen";
}

#include "../common/math.hlsl"
#include "../common/structs.hlsl"

Texture2D hammersleyTexture : register(t0); 
SamplerState samplerPointClamp : register(s0); 

#include "offline_ibl.hlsl"

float4 DFGGen(PI_PosTex input) : SV_TARGET
{
	float3 a_bias_dd = saturate(IntegrateBRDF(input.tex.y, input.tex.x));

	return float4(a_bias_dd.r, a_bias_dd.g, a_bias_dd.b, 1);
}
