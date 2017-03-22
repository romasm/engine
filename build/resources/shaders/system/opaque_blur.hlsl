TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "Blur";
}

#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

#include "../common/light_structs.hlsl"

Texture2D colortex : register(t0); 

SamplerState samplerPointClamp : register(s0);

float4 Blur(PI_PosTex input) : SV_TARGET
{
	const float Gweights[4] = { 0.474, 0.233, 0.028, 0.002 };
	float4 color = 0;
	
	[unroll]
	for(int i=-3; i<=3; i++)
	{
		[unroll]
		for(int j=-3; j<=3; j++)
		{
			color += colortex.SampleLevel(samplerPointClamp, input.tex, 0, int2(i, j)) * Gweights[abs(i)] * Gweights[abs(j)];
		}
	}
	
	return color;
}
