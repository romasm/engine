#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/sh_helpers.hlsl"

#define GROUP_THREAD_COUNT_X 128

RWTexture3D <uint> bricksTempAtlas : register(u0);

struct ProbInterpolation
{
	float4 lerpAdresses[4]; // lerpAdresses[0].w == count
	float4 targetAdresses[48]; // targetAdresses[0].w == count
};

StructuredBuffer<ProbInterpolation> probes : register(t0);

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

[numthreads( GROUP_THREAD_COUNT_X, 1, 1 )]
void Interpolate(uint3 threadID : SV_DispatchThreadID)
{
	uint count, stride;
	probes.GetDimensions(count, stride);
	if (threadID.x >= count)
		return;
	
	ProbInterpolation data = probes[threadID.x];

	SHcoef3 sh = (SHcoef3)0;
	[loop]
	for (int h = 0; h < (int)data.lerpAdresses[0].w; h++)
	{
		uint3 coords = (uint3)data.lerpAdresses[h].xyz;
		SHcoef3 lerpSH = ReadSH(coords);
		[unroll]
		for (int i = 0; i < 9; i++)
			sh.L[i] += lerpSH.L[i];
	}

	[unroll]
	for (int i = 0; i < 9; i++)
		sh.L[i] /= data.lerpAdresses[0].w;

	[loop]
	for (int k = 0; k < (int)data.targetAdresses[0].w; k++)
	{
		uint3 coords = (uint3)data.targetAdresses[k].xyz;
		WriteSH(sh, coords);
	}
}