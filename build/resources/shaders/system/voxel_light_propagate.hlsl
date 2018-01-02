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

#define INV_SQRT3 0.577735
#define INV_SQRT2 0.707107

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
	int4(-1, 0, 0, 0),
	int4(1, 0, 0, 0),
	int4(0, -1, 0, 0),
	int4(0, 1, 0, 0),
	int4(0, 0, -1, 0),
	int4(0, 0, 1, 0)
};

static const int faceInv[6] = {1,0,3,2,5,4};

static const float cornerWeight = (1.0 / 4.0) * INV_SQRT3 / (1 + INV_SQRT2 + INV_SQRT3);// * COS_45;
static const float sideWeight = (1.0 / 4.0) * INV_SQRT2 / (1 + INV_SQRT2 + INV_SQRT3);// * COS_45;
static const float dirWeight = 1.0 / (1 + INV_SQRT2 + INV_SQRT3);

static const float lightFalloff = 0.9; 
static const float emissiveFalloff = 6.0; 
 
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
	
	// corners
	[unroll]
	for(int k0 = 0; k0 < 8; k0++)
	{
		uint sampleLevel = level;
		int4 cornerCoords = coords - cornerOffset[k0];
		
		[flatten]
		if(!validateCoords(cornerCoords, sampleLevel))
			continue;
		cornerCoords.x += volumeData[0].volumeRes * sampleLevel;

		float4 cornerLight[3];
		[unroll]
		for(int f0 = 0; f0 < 3; f0++)
		{
			int4 cornerCoordsFace = cornerCoords;
			cornerCoordsFace.y += volumeData[0].volumeRes *	cornerFaceIDs[k0][f0];
			cornerLight[f0] = sourceLightVolume.Load(cornerCoordsFace);
		}

		float totalGeomWeightRcp = (cornerLight[0].a + cornerLight[1].a + cornerLight[2].a);
		totalGeomWeightRcp = totalGeomWeightRcp > 0 ? (1.0 / totalGeomWeightRcp) : 1.0;

		[unroll]
		for(int l0 = 0; l0 < 3; l0++)
		{
			float4 cornerLightPerFace = cornerLight[0] * (cornerLight[0].a * totalGeomWeightRcp) + 
				cornerLight[1] * (cornerLight[1].a * totalGeomWeightRcp) + 
				cornerLight[2] * (cornerLight[2].a * totalGeomWeightRcp);
			//cornerLightPerFace.a = max(max(cornerLight[0].a, cornerLight[1].a), cornerLight[2].a); // todo
			lightIn[cornerFaceIDs[k0][l0]].rgb += (cornerLightPerFace.rgb * cornerWeight);
			lightIn[cornerFaceIDs[k0][l0]].a = max(lightIn[cornerFaceIDs[k0][l0]].a, cornerLightPerFace.a);
		}
	}
	
	// sides
	[unroll]
	for(int k1 = 0; k1 < 12; k1++)
	{
		uint sampleLevel = level;
		int4 sideCoords = coords - sideOffset[k1];
				
		[flatten]
		if(!validateCoords(sideCoords, sampleLevel))
			continue;
		sideCoords.x += volumeData[0].volumeRes * sampleLevel;

		float4 sideLight[2];
		[unroll]
		for(int f1 = 0; f1 < 2; f1++)
		{
			int4 sideCoordsFace = sideCoords;
			sideCoordsFace.y += volumeData[0].volumeRes * sideFaceIDs[k1][f1];
			sideLight[f1] = sourceLightVolume.Load(sideCoordsFace);
		}
		
		float totalGeomWeightRcp = (sideLight[0].a + sideLight[1].a);
		totalGeomWeightRcp = totalGeomWeightRcp > 0 ? (1.0 / totalGeomWeightRcp) : 1.0;

		[unroll]
		for(int l1 = 0; l1 < 2; l1++)
		{
			float4 sideLightPerFace = sideLight[0] * (sideLight[0].a * totalGeomWeightRcp) + 
				sideLight[1] * (sideLight[1].a * totalGeomWeightRcp);

			lightIn[sideFaceIDs[k1][l1]].rgb += (sideLightPerFace.rgb * sideWeight);
			lightIn[sideFaceIDs[k1][l1]].a = max(lightIn[sideFaceIDs[k1][l1]].a, sideLightPerFace.a);
		}
	}
	
	// dirs
	[unroll]
	for(int k2 = 0; k2 < 6; k2++)
	{
		uint sampleLevel = level;
		int4 dirCoords = coords - dirOffset[k2];
				
		[flatten]
		if(!validateCoords(dirCoords, sampleLevel))
			continue;
		dirCoords.x += volumeData[0].volumeRes * sampleLevel;

		dirCoords.y += volumeData[0].volumeRes * k2;
		float4 dir = sourceLightVolume.Load(dirCoords);
		
		lightIn[k2].rgb += (dir.rgb * dirWeight);
		lightIn[k2].a = max(lightIn[k2].a, dir.a);
	}
	
	// TODO
	const float lightFalloffLevel = pow(lightFalloff, pow(3.0, level));
	const float lightFalloffLevelA = 0.9;//pow(lightFalloff, pow(1.0, level));
	
	// write light
	float occluding = 0;
	[unroll]
	for(int v = 0; v < VOXEL_FACES_COUNT; v++)
	{
		occluding = max(occluding, lightSelf[faceInv[v]].a);
	}
	//occluding = 1 - occluding;
	
	int3 targetCoords = voxelID;
	[unroll]
	for(int t = 0; t < VOXEL_FACES_COUNT; t++)
	{
		if(any(lightSelf[t]))
		{
			lightSelf[t].rgb *= emissiveFalloff;
			lightSelf[t].a = occluding;
			targetLightVolume[targetCoords] = lightSelf[t];
		}
		else
		{
			lightIn[t].rgb *= (1 - occluding) * lightFalloffLevel;
			lightIn[t].a = max(lightIn[t].a * lightFalloffLevelA, occluding); 
			//lightIn[t].a = lightIn[t].a * lightFalloffLevelA;
			targetLightVolume[targetCoords] = lightIn[t];// * occluding * lightFalloffLevel;
		}
		targetCoords.y += volumeData[0].volumeRes;
	}
}