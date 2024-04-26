
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

#include "../common/light_structs.hlsl"

#define GROUP_THREAD_COUNT 8

RWTexture2D <float4> sceneCombinedRW : register(u0);
RWTexture2D <float4> sceneForNextFrameRW : register(u1);

Texture2D diffuseRT : register(t0);
Texture2D specularRT : register(t1);
Texture2D specularMoreRT : register(t2);
//Texture2D gb_matID_objID : register(t3); 
Texture2D depthGB : register(t3);
Texture2D forwardRT : register(t4);

SamplerState samplerPointClamp : register(s0);

[numthreads(GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, 1)]
void Combine(uint3 threadID : SV_DispatchThreadID)
{
	const float2 coords = PixelCoordsFromThreadID(threadID.xy);

	[branch]
	if (coords.x > 1.0f || coords.y > 1.0f)
		return;
		
	float4 diffuseSample = diffuseRT.SampleLevel(samplerPointClamp, coords, 0);
	float4 specularSample = specularRT.SampleLevel(samplerPointClamp, coords, 0);
	float2 specularSecondSample = specularMoreRT.SampleLevel(samplerPointClamp, coords, 0).rg;

	float4 forwardSample = forwardRT.SampleLevel(samplerPointClamp, coords, 0);

	float depth = depthGB.SampleLevel(samplerPointClamp, UVforSamplePow2(coords), 0).r;
	
	// ss blur: TODO

	float4 sceneForNextFrame, sceneCombined;

	sceneForNextFrame.rgb = diffuseSample.rgb + specularSample.rgb;
	sceneForNextFrame.a = depth;

	sceneCombined.rgb = diffuseSample.rgb + specularSample.rgb * specularSecondSample.g + float3(diffuseSample.a, specularSample.a, specularSecondSample.r);
	sceneCombined.a = depth;
	
	// forwardSample.rgb must be premultiplied
	sceneCombined.rgb = sceneCombined.rgb * (1.0f - min(forwardSample.a, 1.0f)) + forwardSample.rgb;

	sceneCombinedRW[threadID.xy] = sceneCombined;//float4(coords,0,1);//
	sceneForNextFrameRW[threadID.xy] = sceneForNextFrame;
}
