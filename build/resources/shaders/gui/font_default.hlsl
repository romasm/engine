TECHNIQUE_DEFAULT
{
	Queue = GUI_2D_FONT;

	VertexShader = "../resources/shaders/gui/font_vs Main";
	PixelShader = "Font";

	BlendEnable = true;
	BlendOp = ADD;
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"

Texture2D sys_fontTexture : register(t0);
SamplerState samplerPointClamp : register(s0);

cbuffer materialBuffer : register(b0)
{
    float4 font_color;
    float4 clip;
};

float4 Font(PI_PosTex input) : SV_TARGET
{
	if(input.pos.x < clip.r)
		discard;
	if(input.pos.y < clip.g)
		discard;
	if(input.pos.x > clip.b)
		discard;
	if(input.pos.y > clip.a)
		discard;
		
	float4 color = font_color;
	color.a *= sys_fontTexture.Sample(samplerPointClamp, input.tex).a;
	
	if( color.a == 0 )
		discard;
	
	return color;
}