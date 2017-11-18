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
	{3,5},
	{3,4},
	{2,5},
	{2,4},
	{0,5},
	{0,4},
	{1,5},
	{1,4},
	{0,3},
	{0,2},
	{1,3},
	{1,2}
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

static const int faceInv[6] = {1,0,3,2,5,4};

#define CACHE_DIM (GROUP_THREAD_COUNT + 2)
//groupshared float4 voxelCache[CACHE_DIM * CACHE_DIM * CACHE_DIM * VOXEL_FACES_COUNT];

bool validateCoords(inout int4 coords, inout uint sampleLevel)
{
	coords.xyz -= int3(volumeData[sampleLevel].prevFrameOffset);

	bool res = true;
	[branch]
	if( coords.x < 0 || coords.y < 0 || coords.z < 0 ||
		coords.x >= (int)volumeData[0].volumeRes || coords.y >= (int)volumeData[0].volumeRes || coords.z >= (int)volumeData[0].volumeRes )
	{
		float3 upLevelCoords = coords.xyz * 0.5 + volumeData[sampleLevel].volumeOffset;

		sampleLevel++;
		[branch]
		if( sampleLevel > volumeTraceData.maxLevel )
		{
			res = false;
		}
		else
		{ 
			upLevelCoords.x = upLevelCoords.x > 0 ? ceil(upLevelCoords.x) : floor(upLevelCoords.x);  
			upLevelCoords.y = upLevelCoords.y > 0 ? ceil(upLevelCoords.y) : floor(upLevelCoords.y);  
			upLevelCoords.z = upLevelCoords.z > 0 ? ceil(upLevelCoords.z) : floor(upLevelCoords.z);  

			coords.xyz = int3(upLevelCoords);
		}
	}
	return res;
}

float4 sampleSource(int4 coords, uint level)
{
	coords.xyz -= int3(volumeData[level].prevFrameOffset);
	return sourceLightVolume.Load(coords);
}

[numthreads( GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, GROUP_THREAD_COUNT )]
void PropagateLight(uint3 voxelID : SV_DispatchThreadID)
{
	const uint level = voxelID.x / volumeData[0].volumeRes;

	int4 coords = int4(voxelID, 0);
	coords.x -= volumeData[0].volumeRes * level;

	float4 lightSelf[VOXEL_FACES_COUNT] = 
	{
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0)
	};
	 
	// overwrite light with fresh emittance
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

	/*
	// corners
	[unroll]
	for(int k0 = 0; k0 < 8; k0++)
	{
		uint sampleLevel = level;
		int4 cornerCoords = coords + cornerOffset[k0];
		
		[branch]
		if(!validateCoords(cornerCoords, sampleLevel, Xoffset))
			continue;

		float4 corner = 0;
		[unroll]
		for(int f0 = 0; f0 < 3; f0++)
		{
			int4 cornerCoordsFace = cornerCoords;
			cornerCoordsFace.y += volumeData[0].volumeRes *	cornerFaceIDs[k0][f0];
			corner += sampleSource(cornerCoordsFace, sampleLevel);
		}
		corner = (corner * ONE_DIV_THREE) * (volumeData[level].voxelDiagRcp * volumeData[level].voxelDiagRcp) * COS_45;

		[unroll]
		for(int l0 = 0; l0 < 3; l0++)
			lightIn[cornerFaceIDs[k0][l0]] += corner;
	}

	// sides
	[unroll]
	for(int k1 = 0; k1 < 12; k1++)
	{
		uint sampleLevel = level;
		int4 sideCoords = coords + sideOffset[k1];
				
		[branch]
		if(!validateCoords(sideCoords, sampleLevel, Xoffset))
			continue;

		float4 side = 0;
		[unroll]
		for(int f1 = 0; f1 < 2; f1++)
		{
			int4 sideCoordsFace = sideCoords;
			sideCoordsFace.y += volumeData[0].volumeRes * sideFaceIDs[k1][f1];
			side += sampleSource(sideCoordsFace, sampleLevel);
		}
		side = (side * 0.5) * (volumeData[level].voxelSizeRcp * volumeData[level].voxelSizeRcp * 0.5) * COS_45;

		[unroll]
		for(int l1 = 0; l1 < 2; l1++)
			lightIn[sideFaceIDs[k1][l1]] += side;
	}
	*/
	// dirs
	[unroll]
	for(int k2 = 0; k2 < 6; k2++)
	{
		uint sampleLevel = level;
		int4 dirCoords = coords + dirOffset[k2];
		[branch]
		if(!validateCoords(dirCoords, sampleLevel))
			continue;
		dirCoords.x += volumeData[0].volumeRes * sampleLevel;
		dirCoords.y += volumeData[0].volumeRes * k2;

		// on CPU
		float extingtion = 10.0;
		float invSqLaw = 1.0 / (volumeData[level].voxelSize * volumeData[level].voxelSize * extingtion * extingtion);
		//float invSqLaw = (volumeData[level].voxelSizeRcp * volumeData[level].voxelSizeRcp);
		const float4 light = sourceLightVolume.Load(dirCoords) * invSqLaw;
		const float occluding = 1 - lightSelf[faceInv[k2]].a;
		lightIn[k2] += light * occluding;
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