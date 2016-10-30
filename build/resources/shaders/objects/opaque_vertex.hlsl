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
GI_Mesh VoxelizationOpaqueVS(VI_Mesh input)
{
    GI_Mesh output;

    output.position = mul(float4(input.position, 1), worldMatrix).rgb;

	output.tex = input.tex;
	
	const float3x3 nM = (float3x3)normalMatrix;
    output.normal = normalize(mul(input.normal, nM));
    output.tangent = normalize(mul(input.tangent, nM));
    output.binormal = normalize(mul(input.binormal, nM));

    return output;
}