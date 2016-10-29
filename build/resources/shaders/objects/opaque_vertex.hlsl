#include "../common/shared.hlsl"
#include "../common/structs.hlsl"

cbuffer matrixBuffer : register(b1)
{
	matrix worldMatrix;
	matrix normalMatrix;
};

PI_Mesh OpaqueVS(VI_Mesh input)
{
    PI_Mesh output;

    output.position = mul(float4(input.position, 1), worldMatrix);
	output.worldPos = output.position;
    output.position = mul(output.position, g_viewProj);
	
	output.tex = input.tex;
	
	const float3x3 nM = (float3x3)normalMatrix;
    output.normal = normalize(mul(input.normal, nM));
    output.tangent = normalize(mul(input.tangent, nM));
    output.binormal = normalize(mul(input.binormal, nM));

    return output;
}

// shadow
cbuffer viewportBuffer : register(b0)
{
	matrix shadowVP;
};

float4 OpaqueShadowVS(VI_Mesh input) : SV_POSITION
{
    float4 output;
    output = mul(float4(input.position, 1), worldMatrix);
    output = mul(output, shadowVP);

    return output;
}

PI_Mesh AlphatestShadowVS(VI_Mesh input)
{
    PI_Mesh output;
    output.position = mul(float4(input.position, 1), worldMatrix);
    output.position = mul(output.position, shadowVP);

	output.tex = input.tex;
	output.normal = 0;
	output.tangent = 0;
	output.binormal = 0;

    return output;
}

// voxelization
cbuffer volumeBuffer : register(b2)
{
	matrix volumeVP;

	float4 volumeOffset;
	float4 volumeScale;
};

PI_Mesh_Subsample VoxelizationOpaqueVS(VI_Mesh input)
{
    PI_Mesh_Subsample output;

    output.position = mul(float4(input.position, 1), worldMatrix);
	output.worldPos.xyz = (output.position.xyz - volumeOffset.xyz) * volumeScale.xyz;
	output.worldPos.w = 1.0f;
    output.position = mul(output.position, volumeVP);
	
	output.tex = input.tex;
	
	const float3x3 nM = (float3x3)normalMatrix;
    output.normal = normalize(mul(input.normal, nM));
    output.tangent = normalize(mul(input.tangent, nM));
    output.binormal = normalize(mul(input.binormal, nM));

    return output;
}