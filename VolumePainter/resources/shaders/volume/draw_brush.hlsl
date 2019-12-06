
#include "common_volume.hlsl"

RWTexture3D <unorm float4> volumeRW : register(u0);

cbuffer volumeInfo : register(b0)
{
	float3 volumeMinCorner;
	float _padding0;

	float3 volumeSizeInv;
	float _padding1;

	float3 volumeSize;
	float _padding2;
};

cbuffer brushInfo : register(b1)
{
	float3 brushPosition;
	float brushRadius;

	float4 brushColorOpacity;

	float brushHardness;
	float _pad1;
	float _pad2;
	float _pad3;
};

[numthreads(GROUP_TREADS_X, GROUP_TREADS_Y, GROUP_TREADS_Z)]
void Draw(uint3 threadID : SV_DispatchThreadID)
{
	uint3 volumeSizeUint = uint3(volumeSize);
	if(threadID.x > volumeSizeUint.x || threadID.y > volumeSizeUint.y || threadID.z > volumeSizeUint.z)
		return;
	
	uint3 volumeCoords = threadID + uint3(volumeMinCorner);

	float4 data = volumeRW.Load(volumeCoords);

	float brushValue = clamp(length(brushPosition - volumeCoords) / brushRadius, 0.0f, 1.0f);
	brushValue = (brushValue - brushHardness) / (1.0f - brushHardness);

	brushValue = (sin((0.5f - brushValue) * PI) + 1.0f) * 0.5f;

	float4 topClapm = lerp(data, brushColorOpacity, brushValue);
	topClapm.a = 1.0f;

	data += brushColorOpacity * brushValue;	
	data = clamp(data, 0.0f, topClapm);

	volumeRW[volumeCoords] = data;
}
