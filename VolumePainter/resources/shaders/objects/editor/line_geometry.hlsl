TECHNIQUE_DEFAULT
{
	Queue = GUI_3D;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;

	FillMode = WIREFRAME;
	CullMode = NONE;

	VertexShader = "VSline";
	PixelShader = "PS";
}

#include "../../common/math.hlsl"
#include "../../common/structs.hlsl"
#include "../../common/shared.hlsl"

cbuffer materialBuffer : register(b1)
{
	float4 dcolor;

	float radius;
	float _padding0;
	float _padding1;
	float _padding2;
};

cbuffer matrixBuffer : register(b2)
{
	matrix world;
};

float4 PS(PI_Pos input) : SV_TARGET
{
	return dcolor;
}

PI_Pos VSline(VI_Pos input)
{
	PI_Pos output;
	
	output.position = mul(float4(input.position, 1), world);
	output.position = mul(output.position, g_viewProj);
	
	return output;
}

PI_Pos VSsphere(VI_Pos input)
{
	PI_Pos output;
	
	input.position *= radius;
	output.position = mul(float4(input.position, 1), world);
	output.position = mul(output.position, g_viewProj);
	
	return output;
}