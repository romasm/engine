TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "CalcHiZ";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

#include "../common/light_structs.hlsl" 

Texture2D depthTex : register(t0); 

SamplerState samplerPointClamp : register(s0);

float2 CalcHiZ(PI_PosTex input) : SV_TARGET
{
	float4 depthMin = depthTex.GatherRed(samplerPointClamp, input.tex);
	float4 depthMax = depthTex.GatherGreen(samplerPointClamp, input.tex);
	
	float2 hiz;
	hiz.x = min( min(depthMin.x, depthMin.y), min(depthMin.z, depthMin.w) );
	hiz.y = max( max(depthMax.x, depthMax.y), max(depthMax.z, depthMax.w) );

	return hiz;
}
