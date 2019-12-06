#include "../common/math.hlsl"
#include "../common/structs.hlsl"

#include "../common/voxel_helpers.hlsl"

cbuffer volumeBuffer : register(b0)
{
	VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX];
};

cbuffer downsampleBuffer : register(b1)
{
	float3 writeOffset;
	uint currentLevel;

	uint currentRes;
	float3 isShifted;
};

static const uint3 voxelOffset[2][8] = 
{
	{
	uint3(0,0,0),
	uint3(1,0,0),
	uint3(0,1,0),
	uint3(1,1,0),
	uint3(0,0,1),
	uint3(1,0,1),
	uint3(0,1,1),
	uint3(1,1,1)
	},{
	uint3(1,1,1),
	uint3(0,1,1),
	uint3(1,0,1),
	uint3(0,0,1),
	uint3(1,1,0),
	uint3(0,1,0),
	uint3(1,0,0),
	uint3(0,0,0)
	}
};

static const uint perFaceOpacity[6][8] = 
{
	{0,0,1,1,2,2,3,3},
	{0,0,1,1,2,2,3,3},

	{0,1,0,1,2,3,2,3},
	{0,1,0,1,2,3,2,3},

	{0,1,2,3,0,1,2,3},
	{0,1,2,3,0,1,2,3}
};

// DOWNSAMPLE
Texture3D <float4> emittanceVolume : register(t0);  
RWTexture3D <float4> downsampleVolumeRW : register(u0);  

inline void DownsampleEmittance(uint3 currentID)
{
	uint face = currentID.y / currentRes;
	uint negative = face % 2;

	uint3 voxelID = currentID;
	voxelID.y = currentID.y % currentRes;
	voxelID *= 2;
	voxelID.y += volumeData[0].volumeRes * face;
	voxelID.xy += volumeData[currentLevel - 1].levelOffset;

	voxelID += (uint3)isShifted;

	float4 emittanceWeight[4] = {
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0),
		float4(0,0,0,0)
	};
	float opacity[4] = {0,0,0,0};
	[unroll]
	for(int i = 0; i < 8; i++)
	{
		float4 sampleEO = emittanceVolume.Load(int4(voxelID + voxelOffset[negative][i], 0));
		uint opacityID = perFaceOpacity[face][i];

		emittanceWeight[ opacityID ] = lerp( sampleEO, emittanceWeight[ opacityID ], opacity[ opacityID ]);
		opacity[ opacityID ] += saturate(sampleEO.a * VOXEL_SUBSAMPLES_COUNT_RCP);
	}

	float finalOpacity = 0;
	float4 finalEmittance = 0;
	[unroll]
	for(int j = 0; j < 4; j++)
	{
		finalOpacity += opacity[j];
		finalEmittance += emittanceWeight[j] * opacity[j];
	}

	[branch]
	if(finalOpacity == 0.0)
		return;

	finalEmittance /= finalOpacity;
	
	downsampleVolumeRW[currentID] = finalEmittance;
}

#define DOWNSAMPLE(x, y, z) [numthreads( x, y, z )] \
void DownsampleEmittance##x(uint3 treadID : SV_DispatchThreadID) {DownsampleEmittance(treadID);}

DOWNSAMPLE(8,8,4)
DOWNSAMPLE(4,4,4)
DOWNSAMPLE(2,2,2)
DOWNSAMPLE(1,1,1)

// MOVE DATA
RWTexture3D <float4> emittanceVolumeRW : register(u0);  
Texture3D <float4> downsampleVolume : register(t0);  

inline void DownsampleMove(uint3 currentID)
{
	uint3 emitID = currentID;
	[branch]
	if(any( isShifted * ( (currentRes - 1) == currentID % currentRes ) )) 
		return; 
	
	uint face = currentID.y / currentRes;
	emitID.y = face * volumeData[0].volumeRes + (currentID.y % currentRes);
	emitID.xy += volumeData[currentLevel].levelOffset;

	emitID += (uint3)writeOffset;
	emittanceVolumeRW[emitID] = downsampleVolume.Load(int4(currentID, 0));
}

#define DOWNSAMPLE_MOVE(x, y, z) [numthreads( x, y, z )] \
void DownsampleMove##x(uint3 treadID : SV_DispatchThreadID)	{DownsampleMove(treadID);}

DOWNSAMPLE_MOVE(8,8,4)
DOWNSAMPLE_MOVE(4,4,4)
DOWNSAMPLE_MOVE(2,2,2)
DOWNSAMPLE_MOVE(1,1,1)