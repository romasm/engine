TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "PS";
}

#include "../common/math.hlsl"
#include "../common/structs.hlsl"

cbuffer materialBuffer : register(b0)
{
    float4 clip;
    float4 _remove;
    float4 pickerColor;
};

float4 PS(PI_PosTex input) : SV_TARGET
{
	if( input.pos.x < clip.r ||
		input.pos.y < clip.g ||
		input.pos.x > clip.b ||
		input.pos.y > clip.a )
		discard;
	
	float3 color = lerp( float3(1,1,1), pickerColor.rgb, input.tex.x );
	color = lerp( color, float3(0,0,0), input.tex.y );
	
	return float4(color, 1);
}