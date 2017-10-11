#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"
#include "../common/voxel_helpers.hlsl"

cbuffer matrixBuffer : register(b1)
{
	matrix worldMatrix;
	matrix normalMatrix;
};

StructuredBuffer<matrix> skinnedMatrixBuffer : register(t0);

PI_Mesh OpaqueVS(VI_Mesh input)
{
    PI_Mesh output = (PI_Mesh)0;

	output.worldPos = mul(float4(input.position, 1), worldMatrix);
    output.position = mul(output.worldPos, g_viewProj);
	
	output.tex = input.tex;
	
	const float3x3 nM = (float3x3)normalMatrix;
    output.normal = normalize(mul(input.normal, nM));
    output.tangent = normalize(mul(input.tangent, nM));
    output.binormal = normalize(mul(input.binormal, nM));

    return output;
}

PI_Mesh OpaqueSkinnedVS(VI_Skinned_Mesh input)
{
    PI_Mesh output = (PI_Mesh)0;

	int bone = 0;
	const float4 localPos = float4(input.position, 1);
	[loop]
	while(input.boneID[bone] >= 0)
	{
		const float weight = input.boneWeight[bone];
		const int indexM = input.boneID[bone] * 2;

		output.worldPos += mul(localPos, skinnedMatrixBuffer[indexM]) * weight;

		const float3x3 nM = (float3x3)skinnedMatrixBuffer[indexM + 1];
		output.normal += mul(input.normal, nM) * weight;
		output.tangent += mul(input.tangent, nM) * weight;
		output.binormal += mul(input.binormal, nM) * weight;

		bone++;
	}

    output.position = mul(output.worldPos, g_viewProj);
	output.tex = input.tex;
	
    output.normal = normalize(output.normal);
    output.tangent = normalize(output.tangent);
    output.binormal = normalize(output.binormal);	

    return output;
}

PI_Mesh OpaqueDepthNormalVS(VI_Mesh input)
{
	PI_Mesh output = (PI_Mesh)0;

    output.position = mul(float4(input.position, 1), worldMatrix);
    output.position = mul(output.position, g_viewProj);
	
	output.tex = input.tex;

	const float3x3 nM = (float3x3)normalMatrix;
    output.normal = normalize(mul(input.normal, nM));

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

float4 OpaqueSkinnedShadowVS(VI_Skinned_Mesh input) : SV_POSITION
{
    float4 output = 0;

	int bone = 0;
	const float4 localPos = float4(input.position, 1);
	[loop]
	while(input.boneID[bone] >= 0)
	{
		output += mul(localPos, skinnedMatrixBuffer[input.boneID[bone] * 2]) * input.boneWeight[bone];
		bone++;
	}
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