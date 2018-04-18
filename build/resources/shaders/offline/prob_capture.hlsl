#include "../common/math.hlsl"
#include "../common/structs.hlsl"

#define GROUP_THREAD_COUNT 8

RWTexture2D <float4> probFace : register(u0);  

Texture2D linColorAndDepth : register(t0); 
SamplerState samplerPointClamp : register(s0); 

[numthreads( GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, 1 )]
void Capture(uint3 threadID : SV_DispatchThreadID)
{
	const uint2 pixelID = threadID.xy;

	float width, height;
	linColorAndDepth.GetDimensions(width, height);

	const float2 uv = (float2(pixelID) + 0.5) / float2(width, height);

	float4 color = linColorAndDepth.SampleLevel(samplerPointClamp, uv, 0);
	color.a = color.a == 1.0 ? 0.0 : 1.0;

	probFace[pixelID] = color;
}