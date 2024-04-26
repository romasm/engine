TECHNIQUE_DEFAULT
{
	Queue = GUI_2D;

	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "Viewport";

	BlendEnable = true;
	BlendOp = ADD;
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
}

#include "../common/math.hlsl"
#include "../common/structs.hlsl"

Texture2D shaderTexture : register(t0);
SamplerState samplerPointClamp : register(s0);

cbuffer materialBuffer : register(b0)
{
    float4 clip;

	float srgb;
	float mulColor;
	float isMulAlpha;
	float mulAlpha;

	float outAlpha;
	float powVal;
	float _padding0;
	float _padding1;
};

float4 Viewport(PI_PosTex input) : SV_TARGET
{
	if( input.pos.x < clip.r ||
		input.pos.y < clip.g ||
		input.pos.x > clip.b ||
		input.pos.y > clip.a )
		discard;
		
	float4 color = shaderTexture.Sample(samplerPointClamp, input.tex);
	
	if(outAlpha > 0)
		color.rgb = color.a;

	color.rgb *= mulColor;

	if(powVal != 1.0)
		color.rgb = PowAbs(color.rgb, powVal);

	if(isMulAlpha > 0)
		color.a *= mulAlpha;
	else
		color.a = mulAlpha;
		
	if(srgb > 0)
		color.rgb = LinearToSRGB(color.rgb);
		
	return saturate(color);
}
