
#include "common_volume.hlsl"

RWTexture3D <unorm float4> volumeRW : register(u0);
RWTexture3D <unorm float4> volumeDiff : register(u1);

cbuffer volumeInfo : register(b0)
{
	float3 volumeMinCorner;
	float _padding0;

	float3 volumeSizeInv;
	float _padding1;

	float3 volumeSize;
	float _padding2;
};

[numthreads(GROUP_TREADS_X, GROUP_TREADS_Y, GROUP_TREADS_Z)]
void StepBack(uint3 threadID : SV_DispatchThreadID)
{
	uint3 volumeCoords = threadID;
	if (volumeCoords.x > uint(volumeSize.x) || volumeCoords.y > uint(volumeSize.y) || volumeCoords.z > uint(volumeSize.z))
		return;

	volumeCoords += uint3(volumeMinCorner.xyz);
		
	float4 data = volumeRW.Load(volumeCoords);
	float4 difference = volumeDiff.Load(volumeCoords);
	//float4 difference = volumeDiff.Load(volumeCoords + uint3(volumeMinCorner.xyz));

	volumeRW[volumeCoords] = data - difference;// +0.1; 
}

[numthreads(GROUP_TREADS_X, GROUP_TREADS_Y, GROUP_TREADS_Z)]
void StepForward(uint3 threadID : SV_DispatchThreadID)
{
	uint3 volumeCoords = threadID;
	if (volumeCoords.x > uint(volumeSize.x) || volumeCoords.y > uint(volumeSize.y) || volumeCoords.z > uint(volumeSize.z))
		return;

	volumeCoords += uint3(volumeMinCorner.xyz);

	float4 data = volumeRW.Load(volumeCoords);
	float4 difference = volumeDiff.Load(volumeCoords);

	volumeRW[volumeCoords] = data + difference; 
}
