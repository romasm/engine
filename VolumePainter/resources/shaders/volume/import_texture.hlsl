
#include "common_volume.hlsl"

RWTexture3D <unorm float4> volumeRW : register(u0);
RWTexture3D <unorm float4> volumeDiff : register(u1);
Texture3D volumeTexture : register(t0);
SamplerState samplerBilinearVolumeClamp : register(s0);

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
void Copy(uint3 threadID : SV_DispatchThreadID)
{
	const float3 coords = (float3(threadID) + 0.5f) * volumeSizeInv;
	if (coords.x > 1.0f || coords.y > 1.0f || coords.z > 1.0f)
		return;
		
	float4 newData = volumeTexture.SampleLevel(samplerBilinearVolumeClamp, coords, 0);

	uint3 volumeCoords = threadID;
	float4 data = volumeRW.Load(volumeCoords);

	float4 difference = volumeDiff.Load(volumeCoords);
	volumeDiff[volumeCoords] = difference + (newData - data);

	volumeRW[volumeCoords] = newData;
}
