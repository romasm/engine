#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/sh_helpers.hlsl"

#define GROUP_THREAD_COUNT_X 128
#define EPCILON 0.00001

RWTexture3D <uint> bricksTempAtlas : register(u0);

struct ProbInterpolation
{
	float4 pos;
	float4 offset;
	float4 targetAdresses[48]; // targetAdresses[0].w == count
};

StructuredBuffer<ProbInterpolation> probes : register(t0);

Texture3D<uint4> g_giChunks : register(t1);
Texture3D<uint> g_giLookups : register(t2);

cbuffer giData : register(b0)
{
	GISampleData g_giSampleData;
};

#define GI_HELPERS_ONLY
#include "../common/gi_helpers.hlsl"   

float3 ReadColor(uint3 coords)
{
	coords.x *= 4;

	float r = asfloat(bricksTempAtlas[coords]);
	coords.x += 1;
	float g = asfloat(bricksTempAtlas[coords]);
	coords.x += 1;
	float b = asfloat(bricksTempAtlas[coords]);
	return float3(r, g, b);
}

void WriteColor(float3 color, uint3 coords)
{
	coords.x *= 4;

	bricksTempAtlas[coords] = asuint(color.r);
	coords.x += 1;
	bricksTempAtlas[coords] = asuint(color.g);
	coords.x += 1;
	bricksTempAtlas[coords] = asuint(color.b);
}

SHcoef3 ReadSH(uint3 coords)
{
	SHcoef3 sh;
	[unroll]
	for (int i = 0; i < 9; i++)
	{
		sh.L[i] = ReadColor(coords);
		coords.z += 3;
	}
	return sh;
}

void WriteSH(SHcoef3 sh, uint3 coords)
{
	[unroll]
	for (int i = 0; i < 9; i++)
	{
		WriteColor(sh.L[i], coords);
		coords.z += 3;
	}
}

#define BRICK_ADRESS_BITS 24
#define BRICK_XY_BITS 12
#define BRICK_X_MASK 0x00fff000
#define BRICK_Y_MASK 0x00000fff
#define BRICK_DEPTH_MASK 0xff000000

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

	// interpolate pos
	float3 samplePos = (data.pos.xyz - g_giSampleData.minCorner) * g_giSampleData.chunkSizeRcp;
	float3 brickInOffset = frac( (samplePos - floor(samplePos)) * brickDepth );
	brickInOffset *= g_giSampleData.brickSampleSize;

	float3 brickSampleAdress = brickInOffset;
	brickSampleAdress.xy += brickOffset * g_giSampleData.brickAtlasOffset.xy;

	brickSampleAdress += g_giSampleData.halfBrickVoxelSize;
		
	const uint3 atlasRes = (uint3)(floor(1.0 / (g_giSampleData.halfBrickVoxelSize * 2.0) + 0.5));
	brickSampleAdress *= atlasRes;
	
	// sampling
	uint3 lerpCoords[8];
	lerpCoords[0] = uint3(floor(brickSampleAdress.x), floor(brickSampleAdress.y), floor(brickSampleAdress.z));
	lerpCoords[1] = uint3(ceil(brickSampleAdress.x), floor(brickSampleAdress.y), floor(brickSampleAdress.z));
	lerpCoords[2] = uint3(floor(brickSampleAdress.x), ceil(brickSampleAdress.y), floor(brickSampleAdress.z));
	lerpCoords[3] = uint3(ceil(brickSampleAdress.x), ceil(brickSampleAdress.y), floor(brickSampleAdress.z));
	lerpCoords[4] = uint3(floor(brickSampleAdress.x), floor(brickSampleAdress.y), ceil(brickSampleAdress.z));
	lerpCoords[5] = uint3(ceil(brickSampleAdress.x), floor(brickSampleAdress.y), ceil(brickSampleAdress.z));
	lerpCoords[6] = uint3(floor(brickSampleAdress.x), ceil(brickSampleAdress.y), ceil(brickSampleAdress.z));
	lerpCoords[7] = uint3(ceil(brickSampleAdress.x), ceil(brickSampleAdress.y), ceil(brickSampleAdress.z));

	float3 lerpFactors = brickSampleAdress - lerpCoords[0];
	float3 invLerpFactors = 1.0 - lerpFactors;

	float weights[8];
	weights[0] = invLerpFactors.x * invLerpFactors.y * invLerpFactors.z;
	weights[1] = lerpFactors.x * invLerpFactors.y * invLerpFactors.z;
	weights[2] = invLerpFactors.x * lerpFactors.y * invLerpFactors.z;
	weights[3] = lerpFactors.x * lerpFactors.y * invLerpFactors.z;
	weights[4] = invLerpFactors.x * invLerpFactors.y * lerpFactors.z;
	weights[5] = lerpFactors.x * invLerpFactors.y * lerpFactors.z;
	weights[6] = invLerpFactors.x * lerpFactors.y * lerpFactors.z;
	weights[7] = lerpFactors.x * lerpFactors.y * lerpFactors.z;

	SHcoef3 sh = (SHcoef3)0;
	[unroll]
	for (int h = 0; h < 8; h++)
	{
		SHcoef3 lerpSH = ReadSH(lerpCoords[h]);
		[unroll]
		for (int i = 0; i < 9; i++)
			sh.L[i] += lerpSH.L[i] * weights[h];
	}

	// write
	[loop]
	for (int k = 0; k < (int)data.targetAdresses[0].w; k++)
	{
		uint3 coords = (uint3)data.targetAdresses[k].xyz;
		WriteSH(sh, coords);
	}
}