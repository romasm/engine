TECHNIQUE_DEFAULT
{
	Queue = GUI_3D_OVERLAY;

	BlendEnable = true;
	BlendOp = ADD;
	BlendOpAlpha = ADD;
	SrcBlend = ONE;//SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
	SrcBlendAlpha = ONE;
	DestBlendAlpha = ONE;

	CullMode = BACK;

	VertexShader = "HudVS";
	PixelShader = "HudPS";
}

//~ code
#include "../../common/math.hlsl"
#include "../../common/shared.hlsl"
#include "../../common/structs.hlsl"

Texture2D colorTex : register(t0);
SamplerState samplerAnisotropicClamp : register(s0);

cbuffer materialBuffer : register(b1)
{
	float4 dcolor;

	float useTexture;
	float _padding0;
	float _padding1;
	float _padding2;
};

float4 HudPS(PI_PosTex input) : SV_TARGET
{
	float4 color_opacity = 1;
	if(useTexture > 0)
		color_opacity = colorTex.Sample(samplerAnisotropicClamp, input.tex);

	return color_opacity * dcolor;
}

// vetrex
cbuffer matrixBuffer : register(b2)
{
	matrix worldMatrix;
	matrix normalMatrix;
};

PI_PosTex HudVS(VI_Mesh input)
{
	PI_PosTex output;
	
	output.pos = mul(float4(input.position, 1), worldMatrix);
    output.pos = mul(output.pos, g_viewProj);
	
	output.tex = input.tex;

	return output;
}