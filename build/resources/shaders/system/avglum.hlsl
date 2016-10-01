TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "CalcAvgLum";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

RWTexture2D<unsigned int> currentLumTex : register(u1);  

Texture2D scene : register(t0); 

SamplerState samplerBilinearClamp : register(s0);

cbuffer materialBuffer : register(b1)
{
	float mip;
	float speed;
	float minLum;
	float maxLum;
};

float CalcAvgLum(PI_PosTex input) : SV_TARGET
{
	const float2 uav_coords = 0;
	float curLum = asfloat(currentLumTex[uav_coords]);

	const float2 tex_coords = float2(0.5f, 0.5f);
	
	float3 color = scene.SampleLevel(samplerBilinearClamp, tex_coords, uint(mip)).rgb;
	float targetLum = luminance(color);
	
	curLum += (targetLum - curLum) * min(g_dt * speed, 1.0f);
	curLum = clamp(curLum, minLum, maxLum);
	currentLumTex[uav_coords] = asuint(curLum);

	return curLum;
}
