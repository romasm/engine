TECHNIQUE_DEFAULT
{
	Queue = GUI_2D;

	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "Texture";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"

Texture2D shaderTexture : register(t0);
SamplerState samplerTrilinearClamp : register(s0);

cbuffer materialBuffer : register(b0)
{
    float4 clip;
};

float4 Texture(PI_PosTex input) : SV_TARGET
{
	if( input.pos.x < clip.r ||
		input.pos.y < clip.g ||
		input.pos.x > clip.b ||
		input.pos.y > clip.a )
		discard;
	
	return shaderTexture.Sample(samplerTrilinearClamp, input.tex);
}
