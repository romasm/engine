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
	VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX];
};

cbuffer volumeBuffer1 : register(b4)
{ 
	VolumeTraceData volumeTraceData;
};

/*
Faces:
0 - X+
1 - X-
2 - Y+
3 - Y-
4 - Z+
5 - Z-
*/

static const int4 dirOffset[6] = 
{
	int4(1, 0, 0, 0),
	int4(-1, 0, 0, 0),
	int4(0, 1, 0, 0),
	int4(0, -1, 0, 0),
	int4(0, 0, 1, 0),
	int4(0, 0, -1, 0)
};
static const int dirFaceIDs[6][4] = 
{
	{2,3,4,5},
	{2,3,4,5},
	{0,1,4,5},
	{0,1,4,5},
	{0,1,2,3},
	{0,1,2,3}
};
static const int faceInv[6] = {1,0,3,2,5,4};

static const float sideWeightMax = 0.175;
static const float lightFalloff = 1.0;
 
#define CACHE_DIM (GROUP_THREAD_COUNT + 2)
//groupshared float4 voxelCache[CACHE_DIM][CACHE_DIM][CACHE_DIM][VOXEL_FACES_COUNT];

bool validateCoords(inout int4 coords, inout uint sampleLevel)
{
	coords.xyz -= int3(volumeData[sampleLevel].prevFrameOffset);

	bool res = true;
	[flatten]
	if( coords.x < 0 || coords.y < 0 || coords.z < 0 ||
		coords.x >= (int)volumeData[0].volumeRes || coords.y >= (int)volumeData[0].volumeRes || coords.z >= (int)volumeData[0].volumeRes )
	{
		sampleLevel++;
		[flatten]
		if( sampleLevel > volumeTraceData.maxLevel )
		{
			res = false;
		}
		else
		{ 
			float3 upLevelCoords = coords.xyz * 0.5 + volumeData[sampleLevel].volumeOffset;
			upLevelCoords.x = upLevelCoords.x > 0 ? ceil(upLevelCoords.x) : floor(upLevelCoords.x);  
			upLevelCoords.y = upLevelCoords.y > 0 ? ceil(upLevelCoords.y) : floor(upLevelCoords.y);  
			upLevelCoords.z = upLevelCoords.z > 0 ? ceil(upLevelCoords.z) : floor(upLevelCoords.z);  

			coords.xyz = int3(upLevelCoords);
		}
	}
	return res;
}

[numthreads( GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, GROUP_THREAD_COUNT )]
void PropagateLight(uint3 voxelID : SV_DispatchThreadID)
{
	const uint level = voxelID.x / volumeData[0].volumeRes;
		
	// overwrite light with fresh emittance
	float4 lightSelf[VOXEL_FACES_COUNT] = 
	{
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0)
	};
	 
	int4 selfCoords = int4(voxelID, 0);
	[unroll] 
	for(int i = 0; i < VOXEL_FACES_COUNT; i++)
	{
		lightSelf[i] = DecodeVoxelData(emittanceVolume.Load(selfCoords));
		selfCoords.y += volumeData[0].volumeRes;
	}

	// accumulate light from neighbors
	float4 lightIn[VOXEL_FACES_COUNT] = 
	{
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0)
	};

	int4 coords = int4(voxelID, 0);
	coords.x -= volumeData[0].volumeRes * level;

	const float lightFalloffLevel = pow(lightFalloff, pow(2.0, level));

	[unroll]
	for(int k = 0; k < 6; k++)
	{
		uint sampleLevel = level;
		int4 dirCoords = coords + dirOffset[k];
		[flatten]
		if(!validateCoords(dirCoords, sampleLevel))
			continue;
		dirCoords.x += volumeData[0].volumeRes * sampleLevel;

		// light sample
		float sideWeight = 0;
		float4 light = 0;
		[unroll]
		for(int f = 0; f < 4; f++)
		{
			const uint faceID = dirFaceIDs[k][f];
			int4 faceCoords = dirCoords;
			faceCoords.y += volumeData[0].volumeRes * faceID;

			const float occluding = 1 - lightSelf[faceInv[faceID]].a;

			const float4 sample = sourceLightVolume.Load(faceCoords);
			const float weight = sideWeightMax * sample.a;

			sideWeight += weight;
			light += sample * weight * occluding;
		}
		
		const uint frontFaceID = k;
		dirCoords.y += volumeData[0].volumeRes * frontFaceID;
		const float frontOccluding = 1 - lightSelf[faceInv[frontFaceID]].a;
		const float frontWeight = 1 - sideWeight;
		light += sourceLightVolume.Load(dirCoords) * frontWeight * frontOccluding;

		lightIn[k] = light * lightFalloffLevel;
	}

	// write light
	int3 targetCoords = voxelID;
	[unroll]
	for(int t = 0; t < VOXEL_FACES_COUNT; t++)
	{
		if(any(lightSelf[t]))
			targetLightVolume[targetCoords] = lightSelf[t];
		else
			targetLightVolume[targetCoords] = lightIn[t];
		targetCoords.y += volumeData[0].volumeRes;
	}
}