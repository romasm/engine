TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "PS";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"

Texture2D shaderTexture : register(t0); 
Texture2D maskTexture : register(t1); 
SamplerState samplerBilinearClamp : register(s0); 

float4 PS(PI_PosTex input) : SV_TARGET
{
	float4 color = shaderTexture.Sample(samplerBilinearClamp, input.tex);
	float mask = maskTexture.Sample(samplerBilinearClamp, input.tex).a;

	color.a = mask;
	return color;
}