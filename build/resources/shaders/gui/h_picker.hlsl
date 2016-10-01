TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "PS";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"

cbuffer materialBuffer : register(b0)
{
    float4 clip;
};

float4 PS(PI_PosTex input) : SV_TARGET
{
	if( input.pos.x < clip.r ||
		input.pos.y < clip.g ||
		input.pos.x > clip.b ||
		input.pos.y > clip.a )
		discard;

	float inv_y = 1 - input.tex.y;

	float h = inv_y * 360;
	uint hi = uint(trunc(inv_y * 6.0));
	float a = frac(inv_y * 6.0);
	float inv_a = 1 - a;
	float3 color;
	if(hi == 0)
		color = float3(1, a, 0);
	else if(hi == 1)
		color = float3(inv_a, 1, 0);
	else if(hi == 2)
		color = float3(0, 1, a);
	else if(hi == 3)
		color = float3(0, inv_a, 1);
	else if(hi == 4)
		color = float3(a, 0, 1);
	else if(hi == 5)
		color = float3(1, 0, inv_a);
	
	return float4(color, 1);
}