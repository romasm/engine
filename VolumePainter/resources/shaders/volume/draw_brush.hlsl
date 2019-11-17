
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
};

[numthreads(GROUP_TREADS_X, GROUP_TREADS_Y, GROUP_TREADS_Z)]
void Draw(uint3 threadID : SV_DispatchThreadID)
{
	uint3 volumeSizeUint = uint3(volumeSize);
	if(threadID.x > volumeSizeUint.x || threadID.y > volumeSizeUint.y || threadID.z > volumeSizeUint.z)
		return;
	
	uint3 volumeCoords = threadID + uint3(volumeMinCorner);

	float4 data = volumeRW.Load(volumeCoords);

	float brushValue = length(brushPosition - volumeCoords) / brushRadius;
	brushValue = 1.0f - clamp(brushValue, 0.0f, 1.0f);

	data += brushColorOpacity * brushValue;
	data = clamp(data, 0.0f, 1.0f);

	volumeRW[volumeCoords] = data;
}
