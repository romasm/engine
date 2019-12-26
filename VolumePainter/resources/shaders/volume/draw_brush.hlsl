
#include "common_volume.hlsl"

RWTexture3D <unorm float4> volumeRW : register(u0);
RWTexture3D <unorm float4> volumeDiff : register(u1);

cbuffer volumeInfo : register(b0)
{
	float3 volumeMinCorner;
	float _padding0;

	float3 volumeSizeInv;
	float _padding1;

	float3 volumeSize;
	float _padding2;
};

cbuffer brushInfo : register(b1)
{
	float3 brushPosition;
	float brushRadius;

	float4 brushColorOpacity;

	float brushHardness;
	float3 brushPrevPosition;
};

float CalcBrushValue(float value)
{
	float brushValue = value / brushRadius;
	brushValue = clamp((brushValue - brushHardness) / (1.0f - brushHardness), 0.0f, 1.0f);
	brushValue = (sin((0.5f - brushValue) * PI) + 1.0f) * 0.5f;
	return brushValue;
}

[numthreads(GROUP_TREADS_X, GROUP_TREADS_Y, GROUP_TREADS_Z)]
void Draw(uint3 threadID : SV_DispatchThreadID)
{
	uint3 volumeSizeUint = uint3(volumeSize);
	if(threadID.x > volumeSizeUint.x || threadID.y > volumeSizeUint.y || threadID.z > volumeSizeUint.z)
		return;
	
	uint3 volumeCoords = threadID + uint3(volumeMinCorner);

	float4 data = volumeRW.Load(volumeCoords);

	float3 vToPrev = volumeCoords - brushPrevPosition;
	float3 prevToCurr = brushPosition - brushPrevPosition;
	float prevToCurrLen = length(prevToCurr);

	float vDist;
	float valueReductor = 0;
	if (prevToCurrLen == 0)
	{
		vDist = length(vToPrev);
	}
	else
	{
		float3 prevToCurrNorm = prevToCurr / prevToCurrLen;

		float3 vProj = prevToCurrNorm * clamp(dot(vToPrev, prevToCurrNorm), 0, prevToCurrLen);
		vDist = length(vToPrev - vProj);

		valueReductor = CalcBrushValue(length(vToPrev));
	}

	float brushValue = CalcBrushValue(vDist);

	float4 topClapm = 1;
	topClapm.rgb = lerp(data.rgb, brushColorOpacity.rgb, brushValue);

	brushValue = max(0, brushValue - valueReductor);

	float4 newData = data + brushColorOpacity * brushValue;
	newData = clamp(newData, 0.0f, topClapm);

	float4 difference = volumeDiff.Load(volumeCoords);
	volumeDiff[volumeCoords] = difference + (newData - data);

	volumeRW[volumeCoords] = newData;
} 
