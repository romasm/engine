TECHNIQUE_DEFAULT
{
	Queue = GUI_3D;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;

	VertexShader = "ToolMeshVS";
	PixelShader = "ToolMeshPS";
}

//~ code
#include "../../common/math.hlsl"
#include "../../common/shared.hlsl"
#include "../../common/structs.hlsl"

Texture2D colorTex : register(t0);
SamplerState samplerAnisotropicWrap : register(s0);

cbuffer materialBuffer : register(b1)
{
	float4 colorMul;

	float useTexture;
	float _padding0;
	float _padding1;
	float _padding2;
};

cbuffer matrixBuffer : register(b2)
{
	matrix worldMatrix;
	matrix normalMatrix;
};

// pixel
float4 ToolMeshPS(PI_ToolMesh input) : SV_TARGET
{
	float4 color_opacity = 0;
	
	[branch]
	if( useTexture > 0 )
	{
		color_opacity = colorTex.Sample(samplerAnisotropicWrap, input.tex);
		if( color_opacity.a < colorMul.a )
			discard;

		color_opacity.rgb = color_opacity.rgb * colorMul.rgb;
	}
	else
	{
		color_opacity.rgb = colorMul.rgb;
	}
	
	float4 res = 1;
	
	float3 camdir = g_CamPos - input.worldPos.rgb;
	camdir = normalize(camdir);
	float angle = dot(camdir, input.normal);
	res.rgb = color_opacity.rgb * saturate((angle + 0.5) / 1.5);  
	
	return res;
}

// vertex
PI_ToolMesh ToolMeshVS(VI_Mesh input)
{
    PI_ToolMesh output;

    output.position = mul(float4(input.position, 1), worldMatrix);
	output.worldPos = output.position;
    output.position = mul(output.position, g_viewProj);
	
    output.normal = mul(input.normal, (float3x3)normalMatrix);
    output.normal = normalize(output.normal);
	
	output.tex = input.tex;

    return output;
}