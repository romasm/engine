TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "PS";
}

#include "../common/math.hlsl"
#include "../common/structs.hlsl"

Texture2D shaderTexture : register(t0); 
SamplerState samplerBilinearClamp : register(s0); 

cbuffer paramsBuffer : register(b0)
{
    float4 samplesPerPixel;
    float4 pixelSize;
};

float4 PS(PI_PosTex input) : SV_TARGET
{
	float4 color = 0;
	[loop]
	for(int i = 0; i < (int)samplesPerPixel.x; i++)
		[loop]
		for(int j = 0; j < (int)samplesPerPixel.y; j++)
			color += shaderTexture.SampleLevel(samplerBilinearClamp, 
				input.tex + pixelSize.xy * float2(i, j), 0);

	return color * samplesPerPixel.z;
}