TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "MixSceneHud";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

Texture2D sceneTex : register(t0);  
Texture2D hudTex : register(t1);  
SamplerState samplerPointClamp : register(s0);

float4 MixSceneHud(PI_PosTex input) : SV_TARGET
{
	float4 color = sceneTex.Sample(samplerPointClamp, input.tex);
	float4 hud = hudTex.Sample(samplerPointClamp, input.tex);
	
	color = lerp(color, hud, hud.a);
	color.a = 1.0f;
	//color = saturate(color);
	return color;
}