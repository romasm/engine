#include "../common/math.hlsl"
#include "../common/structs.hlsl"

#include "../common/light_structs.hlsl"

#include "../system/direct_brdf.hlsl" 
#include "../common/voxel_helpers.hlsl"
#include "../common/shadow_helpers.hlsl"
#include "../common/light_helpers.hlsl" 
    
#define GROUP_THREAD_COUNT 4

#define ONE_DIV_THREE 1.0 / 3.0
#define COS_45 0.7071068

RWTexture3D <float4> targetLightVolume : register(u0);  

SamplerState samplerPointClamp : register(s0);

Texture3D <float4> emittanceVolume : register(t0);
Texture3D <float4> sourceLightVolume : register(t1);

cbuffer volumeBuffer0 : register(b0)
{
	VolumeData sourceData[VCT_CLIPMAP_COUNT_MAX];
};

cbuffer volumeBuffer1 : register(b1)
{
	VolumeData targetData[VCT_CLIPMAP_COUNT_MAX];
};

static const int4 cornerOffset[8] = 
{
	int4(1, 1, 1, 0),
	int4(1, 1, -1, 0),
	int4(1, -1, 1, 0),
	int4(1, -1, -1, 0),
	int4(-1, 1, 1, 0),
	int4(-1, 1, -1, 0),
	int4(-1, -1, 1, 0),
	int4(-1, -1, -1, 0)
};

static const int cornerFaceIDs[8][3] = 
{
	{1,3,5},
	{1,3,4},
	{1,2,5},
	{1,2,4},
	{0,3,5},
	{0,3,4},
	{0,2,5},
	{0,2,4}
};

static const int4 sideOffset[12] = 
{
	int4(0, 1, 1, 0),
	int4(0, 1, -1, 0),
	int4(0, -1, 1, 0),
	int4(0, -1, -1, 0),
	int4(-1, 0, 1, 0),
	int4(-1, 0, -1, 0),
	int4(1, 0, 1, 0),
	int4(1, 0, -1, 0),
	int4(-1, 1, 0, 0),
	int4(-1, -1, 0, 0),
	int4(1, 1, 0, 0),
	int4(1, -1, 0, 0)
};

static const int sideFaceIDs[12][2] = 
{
	{1,3},// TODO
};

static const int4 dirOffset[6] = 
{
	int4(1, 0, 0, 0),
	int4(-1, 0, 0, 0),
	int4(0, 1, 0, 0),
	int4(0, -1, 0, 0),
	int4(0, 0, 1, 0),
	int4(0, 0, -1, 0)
};

#define CACHE_DIM (GROUP_THREAD_COUNT + 2)
//groupshared float4 voxelCache[CACHE_DIM * CACHE_DIM * CACHE_DIM * VOXEL_FACES_COUNT];

[numthreads( GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, GROUP_THREAD_COUNT )]
void PropagateLight(uint3 voxelID : SV_DispatchThreadID)
{
	uint level = voxelID.x / volumeData[0].volumeRes;

	uint3 voxelIDinLevel = voxelID;
	voxelIDinLevel.x = voxelIDinLevel.x % volumeData[0].volumeRes;

	float3 wpos = (float3(voxelIDinLevel) + 0.5f) * volumeData[level].voxelSize;
	wpos += volumeData[level].cornerOffset;
		
	int4 coords = int4(voxelID, 0);

	float4 lightSelf[VOXEL_FACES_COUNT] = {0,0,0,0,0,0};

	// accumulate light from neighbors

	// corners
	[unroll]
	for(int k0 = 0; k0 < 8; k0++)
	{
		const int4 cornerCoords = coords + cornerOffset[k0];
		float4 corner = 0;
		[unroll]
		for(int f0 = 0; f0 < 3; f0++)
		{
			int4 cornerCoordsFace = cornerCoords;
			cornerCoordsFace.y += volumeData[0].volumeRes *	cornerFaceIDs[k0][f0];

			// TODO: prev frame & out of bounds correction 
			corner += sourceLightVolume.Load(cornerCoordsFace);
		}

		corner = (corner * ONE_DIV_THREE) * (volumeData[level].voxelDiagRcp * volumeData[level].voxelDiagRcp) * COS_45;

		[unroll]
		for(int l0 = 0; l0 < 3; l0++)
			lightSelf[cornerFaceIDs[k0][l0]] += corner;
	}

	// sides
	[unroll]
	for(int k1 = 0; k1 < 12; k1++)
	{
		const int4 sideCoords = coords + sideOffset[k1];
		float4 side = 0;
		[unroll]
		for(int f1 = 0; f1 < 2; f1++)
		{
			int4 sideCoordsFace = sideCoords;
			sideCoordsFace.y += volumeData[0].volumeRes * sideFaceIDs[k1][f1];

			// TODO: prev frame & out of bounds correction 
			side += sourceLightVolume.Load(sideCoordsFace);
		}

		side = (side * 0.5) * (volumeData[level].voxelSizeRcp * volumeData[level].voxelSizeRcp * 0.5) * COS_45;

		[unroll]
		for(int l1 = 0; l1 < 2; l1++)
			lightSelf[sideFaceIDs[k1][l1]] += side;
	}

	// dirs
	[unroll]
	for(int k2 = 0; k2 < 6; k2++)
	{
		const int4 dirCoords = coords + dirOffset[k2];
		dirCoords.y += volumeData[0].volumeRes * k2;

		// TODO: prev frame & out of bounds correction 
		float4 dir = sourceLightVolume.Load(dirCoords);
		dir = dir * (volumeData[level].voxelSizeRcp * volumeData[level].voxelSizeRcp);

		lightSelf[k2] += dir;
	}
	
	// overwrite light with fresh emittance
	int4 selfCoords = coords;
	[unroll]
	for(int i = 0; i < VOXEL_FACES_COUNT; i++)
	{
		selfCoords.y += volumeData[0].volumeRes;
		const float4 emittance = emittanceVolume.Load(selfCoords);
		[branch]
		if(any(emittance))
			lightSelf[i] = emittance;
	}

	// write light
	int3 targetCoords = voxelID;
	[unroll]
	for(int t = 0; t < VOXEL_FACES_COUNT; t++)
	{
		targetCoords.y += volumeData[0].volumeRes;
		targetLightVolume[targetCoords] = lightSelf[t];
	}
}