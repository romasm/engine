#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/sh_helpers.hlsl"

#define GROUP_THREAD_COUNT_X 8
#define GROUP_THREAD_COUNT_Y 8

RWTexture3D <uint> bricksAtlas : register(u0);

TextureCube <float4> cubemap : register(t0);
SamplerState samplerBilinearWrap : register(s0);

cbuffer adressBuffer : register(b0)
{
	float pixelCountRcp; //1.0 / (CUBE_RES * CUBE_RES * 6)
	uint probCount;
	float _padding0;
	float _padding1;
	float4 adresses[48]; // adresses[0].w == count
};

#define MAX_WAITING_CYCLES 100000
void InterlockedFloatAdd(uint3 coords, float value)
{
	uint comp;
	uint orig = bricksAtlas[coords];
	int iter = 0;
	[allow_uav_condition]
	[loop]
	do
	{
		comp = orig;
		const float newValue = asfloat(orig) + value;
		InterlockedCompareExchange(bricksAtlas[coords], comp, asuint(newValue), orig);
		iter++;
	} while (orig != comp && iter < MAX_WAITING_CYCLES);
} 

void AppendSHvalue(float3 value, uint3 coords)
{
	coords.x *= 4;

	InterlockedFloatAdd(coords, value.r);
	coords.x += 1;
	InterlockedFloatAdd(coords, value.g); 
	coords.x += 1;
	InterlockedFloatAdd(coords, value.b);
}

static const float3 cubeVectors[6][4] = 
{
	{
		float3(1, 1, -1),
		float3(1, 1, 1),
		float3(1, -1, -1),
		float3(1, -1, 1)
	},
	{
		float3(-1, 1, 1),
		float3(-1, 1, -1),
		float3(-1, -1, 1),
		float3(-1, -1, -1)
	},
	{
		float3(1, 1, -1),
		float3(-1, 1, -1),
		float3(1, 1, 1),
		float3(-1, 1, 1)
	},
	{
		float3(-1, -1, -1),
		float3(1, -1, -1),
		float3(-1, -1, 1),
		float3(1, -1, 1)
	},
	{
		float3(1, 1, 1),
		float3(-1, 1, 1),
		float3(1, -1, 1),
		float3(-1, -1, 1)
	},
	{ 
		float3(-1, 1, -1),
		float3(1, 1, -1),
		float3(-1, -1, -1),
		float3(1, -1, -1)
	}, 
}; 

float3 GetCubeDir(float x, float y, uint face) 
{
	float3 xVect0 = lerp(cubeVectors[face][0], cubeVectors[face][1], x);
	float3 xVect1 = lerp(cubeVectors[face][2], cubeVectors[face][3], x);
	return normalize(lerp(xVect0, xVect1, y));
}

groupshared SHcoef3 groupSH[GROUP_THREAD_COUNT_X * GROUP_THREAD_COUNT_Y];

[numthreads( GROUP_THREAD_COUNT_X, GROUP_THREAD_COUNT_Y, 1 )]
void ComputeSH(uint3 threadID : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	float width, height;
	cubemap.GetDimensions(width, height);
	const float cubeResRpc = 1.0f / width;
	   
	const int face = threadID.z;

	//[unroll]
	//for (int face = 0; face < 6; face++)
	//{
		float3 dir = GetCubeDir(threadID.x * cubeResRpc, threadID.y * cubeResRpc, face);
		float3 color = cubemap.SampleLevel(samplerBilinearWrap, dir, 0).xyz;

		float3 colorWeighed = color * pixelCountRcp;
		groupSH[groupIndex] = CalculateSHCoefs(colorWeighed, dir);
	//}

	GroupMemoryBarrierWithGroupSync();

	if (groupIndex != 0)
		return;

	SHcoef3 finalSH = (SHcoef3)0;
	[loop]
	for (int p = 0; p < GROUP_THREAD_COUNT_X * GROUP_THREAD_COUNT_Y; p++)
	{
		SHcoef3 sh = groupSH[p];
		[unroll]
		for (int j = 0; j < 9; j++)
		{
			finalSH.L[j] += sh.L[j];
		}
	}
		
	[loop]
	for(int k = 0; k < (int)adresses[0].w; k++)
	{
		uint3 coords = (uint3)adresses[k].xyz;
			
		[unroll]
		for (int i = 0; i < 9; i++) 
		{
			AppendSHvalue(finalSH.L[i], coords);
			coords.z += 3;
		}
	}
}