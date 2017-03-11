#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"
#include "../common/voxel_helpers.hlsl"

cbuffer matrixBuffer : register(b1)
{
	matrix worldMatrix;
	matrix normalMatrix;
};

PI_Mesh OpaqueVS(VI_Mesh input)
{
    PI_Mesh output;

	output.worldPos = mul(float4(input.position, 1), worldMatrix);
    output.position = mul(output.worldPos, g_viewProj);
	
	output.tex = input.tex;
	
	const float3x3 nM = (float3x3)normalMatrix;
    output.normal = normalize(mul(input.normal, nM));
    output.tangent = normalize(mul(input.tangent, nM));
    output.binormal = normalize(mul(input.binormal, nM));

    return output;
}

float4 OpaqueDepthVS(VI_Mesh input) : SV_POSITION
{
    float4 output;
    output = mul(float4(input.position, 1), worldMatrix);
    output = mul(output, g_viewProj);

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

    output.worldPos = mul(float4(input.position, 1), worldMatrix);
    output.position = mul(output.worldPos, shadowVP);

	output.tex = input.tex;

	output.normal = 0;
	output.tangent = 0;
	output.binormal = 0;

    return output;
}

// voxelization
cbuffer matrixBuffer : register(b1)
{
	StmInstanceMatrix matrixPerInstance[VCT_MESH_MAX_INSTANCE];
};

GI_Mesh VoxelizationOpaqueVS(VI_Mesh input, uint instID : SV_InstanceID)
{
    GI_Mesh output;

    output.position = mul(float4(input.position, 1), matrixPerInstance[instID].worldMatrix).rgb;

	output.tex = input.tex;
	
	const float3x3 nM = (float3x3)matrixPerInstance[instID].normalMatrix;
    output.normal = normalize(mul(input.normal, nM));
    output.tangent = normalize(mul(input.tangent, nM));
    output.binormal = normalize(mul(input.binormal, nM));

    return output;
}