#include "../common/math.hlsl"
#include "../common/structs.hlsl"

#define GROUP_THREAD_COUNT 8

RWTexture3D <uint> colorVolume0 : register(u0);  
RWTexture3D <uint> colorVolume1 : register(u1);  

SamplerState samplerBilinearClamp : register(s0);

Texture2D <float> shadowsAtlas : register(t0);  

cbuffer inputConfig : register(b0)
{
	float4 volumeOffsetSize;
	float4 volumeScaleResDir;
};

[numthreads( GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, GROUP_THREAD_COUNT )]
void InjectLightToVolume(uint3 voxelID : SV_DispatchThreadID)
{
    colorVolume1[voxelID] |= 0x0000ffff;
}