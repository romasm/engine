#include "../common/math.hlsl"
#include "../common/structs.hlsl"

#define SG_OFFLINE
#include "../common/sg_helpers.hlsl"

#define GROUP_THREAD_COUNT_X 128
#define EPCILON 0.001

RWTexture3D <float4> bricksAtlas : register(u0);

SamplerState samplerBilinearVolumeClamp : register(s0);

struct ProbInterpolation
{
	float4 pos;
	float4 offset;
	float4 targetAdresses[48]; // targetAdresses[0].w == count
};

StructuredBuffer<ProbInterpolation> probes : register(t0);

Texture3D<uint4> g_giChunks : register(t1);
Texture3D<uint> g_giLookups : register(t2);
Texture3D<float4> g_giBricks : register(t3);

cbuffer giData : register(b0)
{
	GISampleData g_giSampleData;
};

#define GI_HELPERS_ONLY
#include "../common/gi_helpers.hlsl"   

[numthreads( GROUP_THREAD_COUNT_X, 1, 1 )]
void Interpolate(uint3 threadID : SV_DispatchThreadID)
{
	uint count, stride;
	probes.GetDimensions(count, stride);
	if (threadID.x >= count)
		return;

	ProbInterpolation data = probes[threadID.x];

	// find near brick
	float3 chunkPos = (data.pos.xyz + data.offset.xyz - g_giSampleData.minCorner) * g_giSampleData.chunkSizeRcp;
	chunkPos = clamp(chunkPos, float3(0,0,0), g_giSampleData.chunksCount - EPCILON);
	
	float3 chunkPosFloor = floor(chunkPos);
	float3 chunkPosFrac = chunkPos - chunkPosFloor;

	uint2 brickOffset;
	uint brickDepth;
	GetBrickAddres(chunkPosFloor, chunkPosFrac, brickOffset, brickDepth);
	float3 brickSampleAdress = GetGIAddres(chunkPosFrac, brickOffset, brickDepth);	
	SGAmpl sg = ReadBrickSG(brickSampleAdress);
	
	// write 
	[loop]
	for (int k = 0; k < (int)data.targetAdresses[0].w; k++)
	{
		uint3 coords = (uint3)data.targetAdresses[k].xyz;
		
		[unroll]
		for (int i = 0; i < 9; i++)
		{
			bricksAtlas[coords] = float4(sg.A[i], 0);
			coords.z += 3;
		}
	}
}