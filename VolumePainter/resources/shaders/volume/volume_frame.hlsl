TECHNIQUE_DEFAULT
{
	Queue = SC_FORWARD;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;

	BlendEnable = false;

	CullMode = BACK;

	VertexShader = "VolumeFrameVS";
	PixelShader = "VolumeFramePS";
}

#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"
#include "../common/common_helpers.hlsl"

cbuffer matrixBuffer : register(b1)
{
	matrix worldMatrix;
	matrix normalMatrix;
};

struct PI_Frame
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 localPos: POSITION;
};

#define FRAME_THICKNESS 0.01
#define FRAME_OPACITY 0.1

float4 VolumeFramePS(PI_Frame input) : SV_TARGET
{
	float2 edgeTest = abs(input.tex - 0.5f) * 2.0f;
	if (edgeTest.x < (1 - FRAME_THICKNESS) && edgeTest.y < (1 - FRAME_THICKNESS))
		discard;

	float3 edgeColor = 0;
	if (abs(input.normal.y) < 0.01 && (2 * abs(input.localPos.y)) < (1 - FRAME_THICKNESS))
		edgeColor = float3(0.02, 0.6, 0.02);
	else if(abs(input.normal.x) < 0.01 && (2 * abs(input.localPos.x)) < (1 - FRAME_THICKNESS))
		edgeColor = float3(0.6, 0.02, 0.02);
	else if (abs(input.normal.z) < 0.01 && (2 * abs(input.localPos.z)) < (1 - FRAME_THICKNESS))
		edgeColor = float3(0.02, 0.02, 0.6);
	
	return float4(edgeColor, FRAME_OPACITY);
}

PI_Frame VolumeFrameVS(VI_Mesh input)
{
	PI_Frame output;

	output.localPos = input.position;
	output.position = mul(float4(input.position, 1), worldMatrix);
	output.position = mul(output.position, g_viewProj);

	output.normal = mul(input.normal, (float3x3)normalMatrix);
	output.normal = normalize(output.normal);

	output.tex = input.tex;

	return output;
}