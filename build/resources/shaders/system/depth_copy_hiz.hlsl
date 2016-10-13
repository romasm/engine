TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "DepthCopy";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

Texture2D depthTex: register(t0); 
SamplerState samplerPointClamp : register(s0);

float2 DepthCopy(PI_PosTex input) : SV_TARGET
{	
	float2 uv = input.tex / g_uvCorrectionForPow2;
	float d = depthTex.Sample(samplerPointClamp, uv).r;
	return float2(d,d);
}
