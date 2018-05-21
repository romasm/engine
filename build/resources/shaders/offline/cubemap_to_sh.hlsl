#include "../common/math.hlsl"
#include "../common/structs.hlsl"

#define GROUP_THREAD_COUNT 8

RWTexture3D <uint> bricksAtlas : register(u0);

TextureCube <float4> cubemap : register(t0);
SamplerState samplerBilinearWrap : register(s0);

cbuffer configBuffer : register(b0)
{
	float4 adressesCount;
	float4 adresses[48];
};

#define CUBE_RES 64
#define SH_MUL 1.0 / (CUBE_RES * CUBE_RES * 6)

struct SHcoef
{
	float3 L[9];
};

SHcoef CalcSH(float3 color, float3 dir)
{
	float Y00 = 0.282095;
	float Y11 = 0.488603 * dir.x;
	float Y10 = 0.488603 * dir.z;
	float Y1_1 = 0.488603 * dir.y;
	float Y21 = 1.092548 * dir.x * dir.z;
	float Y2_1 = 1.092548 * dir.y * dir.z;
	float Y2_2 = 1.092548 * dir.y * dir.x;
	float Y20 = 0.946176 * dir.z * dir.z - 0.315392;
	float Y22 = 0.546274 * (dir.x * dir.x - dir.y * dir.y);

	float3 colorWeighed = color * SH_MUL;

	SHcoef res;
	res.L[0] = colorWeighed * Y00;
	res.L[1] = colorWeighed * Y11;
	res.L[2] = colorWeighed * Y10;
	res.L[3] = colorWeighed * Y1_1;
	res.L[4] = colorWeighed * Y21;
	res.L[5] = colorWeighed * Y2_1;
	res.L[6] = colorWeighed * Y2_2;
	res.L[7] = colorWeighed * Y20;
	res.L[8] = colorWeighed * Y22;
	return res;
}

#define MAX_WAITING_CYCLES 100
void InterlockedFloatAdd(uint3 coords, float value)
{
	uint comp;
	uint orig = bricksAtlas[coords];
	int iter = 0;
	[allow_uav_condition]
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
	}
};

float3 GetCubeDir(float x, float y, uint face)
{
	float3 xVect0 = lerp(cubeVectors[face][0], cubeVectors[face][1], x);
	float3 xVect1 = lerp(cubeVectors[face][2], cubeVectors[face][3], x);
	return normalize(lerp(xVect0, xVect1, y));
}

[numthreads( GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, 1 )]
void ComputeSH(uint3 threadID : SV_DispatchThreadID)
{
	float width, height;
	cubemap.GetDimensions(width, height);
	const float cubeResRpc = 1.0f / width;

	[unroll]
	for (int face = 0; face < 6; face++) 
	{		
		float3 dir = GetCubeDir(threadID.x * cubeResRpc, threadID.y * cubeResRpc, face);
		float3 color = cubemap.SampleLevel(samplerBilinearWrap, dir, 0).xyz;

		SHcoef sh = CalcSH(color, dir);
		
		[loop]
		for(int k = 0; k < (int)adressesCount.x; k++)
		{
			uint3 coords = (uint3)adresses[k].xyz;
			coords.z *= 9;

			[unroll]
			for (int i = 0; i < 9; i++) 
			{
				AppendSHvalue(sh.L[i], coords);
				coords.z++;
			}
		}
	}
}