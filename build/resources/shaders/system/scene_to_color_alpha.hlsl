TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "PS";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"

Texture2D shaderTexture : register(t0); 
SamplerState samplerBilinearClamp : register(s0); 

float4 PS(PI_PosTex input) : SV_TARGET
{
	float4 color = shaderTexture.Sample(samplerBilinearClamp, input.tex);
	return color;
}