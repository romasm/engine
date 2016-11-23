#include "../common/math.hlsl"
#include "../common/structs.hlsl"

#include "../common/voxel_helpers.hlsl"

cbuffer volumeBuffer : register(b0)
{
	VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX];
};

cbuffer downsampleBuffer : register(b1)
{
	float3 volumeOffset;
	uint _padding0;

	uint currentLevel;
	uint currentRes;
	uint currentResMore;
	uint _padding1;

	uint isShifted[3];
	uint _padding2;
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

Texture3D <float4> emittanceVolume : register(t0);  
RWTexture3D <float4> downsampleVolumeRW : register(u0);  

[numthreads( 1, 1, 1 )]
void DownsampleEmittance(uint3 treadID : SV_DispatchThreadID)
{
	const int vRes = volumeData[0].volumeRes;
	uint3 dataRes = uint3(currentRes + isShifted[0],
						currentRes + isShifted[1],
						currentRes + isShifted[2]);

	uint face = treadID.y / dataRes.y;
	uint negative = face % 2;

	uint3 destID = treadID;
	uint inFaceOffset = treadID.y % dataRes.y;
	destID.y = face * currentResMore + inFaceOffset;

	uint3 voxelID = treadID;
	voxelID.y = inFaceOffset;
	voxelID *= 2;
	//voxelID.y += vRes * face;
	//voxelID.x += vRes * (currentLevel - 1);

	float3 emittanceWeight[4] = {
		float3(0,0,0),
		float3(0,0,0),
		float3(0,0,0),
		float3(0,0,0)
	};
	float opacity[4] = {0,0,0,0};
	[unroll]
	for(int i = 0; i < 8; i++)
	{
		int4 coords = int4(voxelID + voxelOffset[negative][i], 0);
		coords.xyz -= int3(isShifted[0], isShifted[1], isShifted[2]);

		if( coords.x < 0 || coords.y < 0 || coords.z < 0 ||
			coords.x >= vRes || coords.y >= vRes || coords.z >= vRes )
		{
			coords.xyz = int3(treadID.x, inFaceOffset, treadID.z) + int3(volumeOffset);
		}

		coords.y += vRes * face;
		coords.x += vRes * (currentLevel - 1);

		float4 sampleEO = emittanceVolume.Load(coords);
		uint opacityID = perFaceOpacity[face][i];

		emittanceWeight[ opacityID ] = lerp( sampleEO.rgb, emittanceWeight[ opacityID ], opacity[ opacityID ] );
		opacity[ opacityID ] += sampleEO.a;
	}

	float finalOpacity = 0;
	float3 finalEmittance = 0;
	[unroll]
	for(int j = 0; j < 4; j++)
	{
		finalOpacity += opacity[j];
		finalEmittance += emittanceWeight[j] * opacity[j];
	}
	finalEmittance /= (finalOpacity > 0 ? finalOpacity : 1.0f);
	finalOpacity *= 0.25f;
	
	downsampleVolumeRW[destID] = float4(finalEmittance, finalOpacity);
}

RWTexture3D <float4> emittanceVolumeRW : register(u0);  
Texture3D <float4> downsampleVolume : register(t0);  

[numthreads( 1, 1, 1 )]
void DownsampleMove(uint3 treadID : SV_DispatchThreadID)
{
	uint3 emitID = treadID;

	uint3 dataRes = uint3(currentRes + isShifted[0],
						currentRes + isShifted[1],
						currentRes + isShifted[2]);

	uint face = emitID.y / dataRes.y;
	emitID.y = face * volumeData[0].volumeRes + (emitID.y % dataRes.y);
	emitID.x += volumeData[0].volumeRes * currentLevel;

	emitID += uint3(volumeOffset);	

	uint3 sourceID = treadID;
	sourceID.y = face * currentResMore + (sourceID.y % dataRes.y);

	emittanceVolumeRW[emitID] = downsampleVolume.Load(int4(sourceID, 0));
}