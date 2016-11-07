#include "../common/math.hlsl"
#include "../common/structs.hlsl"

#include "../common/voxel_helpers.hlsl"

cbuffer volumeBuffer : register(b0)
{
	matrix volumeVP[3];
	
	float3 cornerOffset;
	float worldSize;
		
	float scaleHelper;
	uint volumeRes;
	uint volumeDoubleRes;
	float voxelSize;
};

cbuffer downsampleBuffer : register(b1)
{
	uint currentLevel;
	uint currentRes;
	uint _padding0;
	uint _padding1;
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
	uint face = treadID.y / currentRes;
	uint negative = face % 2;

	uint3 voxelID = treadID;
	voxelID.y = treadID.y % currentRes;
	voxelID *= 2;
	voxelID.y += volumeRes * face;
	voxelID.x += volumeRes * (currentLevel - 1);

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
		float4 sampleEO = emittanceVolume.Load(int4(voxelID + voxelOffset[negative][i], 0));
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
	
	downsampleVolumeRW[treadID] = float4(finalEmittance, finalOpacity);
}

RWTexture3D <float4> emittanceVolumeRW : register(u0);  
Texture3D <float4> downsampleVolume : register(t0);  

[numthreads( 1, 1, 1 )]
void DownsampleMove(uint3 treadID : SV_DispatchThreadID)
{
	uint3 emitID = treadID;

	uint face = emitID.y / currentRes;
	emitID.y = face * volumeRes + (emitID.y % currentRes);

	emitID.x += volumeRes * currentLevel;

	emittanceVolumeRW[emitID] = downsampleVolume.Load(int4(treadID, 0));
}