#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "light_constants.hlsl"
#include "voxel_helpers.hlsl"

#define GROUP_THREAD_COUNT 8

RWTexture3D <float4> emittanceVolume : register(u0);  

SamplerState samplerBilinearClamp : register(s0);
SamplerState samplerPointClamp : register(s1);

Texture2D <float> shadowsAtlas : register(t0);  

Texture3D <uint> opacityVolume : register(t1);
Texture3D <uint> colorVolume0 : register(t2);
Texture3D <uint> colorVolume1 : register(t3);
Texture3D <uint> normalVolume : register(t4);

StructuredBuffer<SpotVoxelBuffer> spotLightInjectBuffer : register(t5); 
StructuredBuffer<PointVoxelBuffer> pointLightInjectBuffer : register(t6); 
StructuredBuffer<DirVoxelBuffer> dirLightInjectBuffer : register(t7); 

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

cbuffer volumeBuffer : register(b1)
{
	uint spotCount;
	uint pointCount;
	uint dirCount;
	uint _padding;
};

// temp
float smoothDistanceAtt( float squaredDistance, float invSqrAttRadius )
{
	float factor = squaredDistance * invSqrAttRadius;
	if(factor >= 1.0)return 0;
	float smoothFactor = saturate(1.0f - factor * factor );
	return smoothFactor * smoothFactor;
}

float getDistanceAtt( float3 unormalizedLightVector, float invSqrAttRadius )
{
	float sqrDist = dot( unormalizedLightVector, unormalizedLightVector );
	float smoothFalloff = smoothDistanceAtt( sqrDist , invSqrAttRadius );
	if(smoothFalloff == 0)return 0;
	float attenuation = 1.0f / (max( sqrDist , 0.0001f) );
	attenuation *= smoothFalloff;
	return attenuation ;
}

float getAngleAtt( float3 normalizedLightVector, float3 lightDir, float lightAngleScale, float lightAngleOffset )
{
	float cd = dot( lightDir, normalizedLightVector );
	float attenuation = saturate( cd * lightAngleScale + lightAngleOffset );
	attenuation *= attenuation; // smoother???
	return attenuation;
}

[numthreads( GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, GROUP_THREAD_COUNT )]
void InjectLightToVolume(uint3 voxelID : SV_DispatchThreadID)
{
	float3 wpos = (float3(voxelID) + 0.5f) * voxelSize;
	wpos += cornerOffset;

	uint3 faceAdress[6];
	float4 faceColor[6];
	float3 faceNormal[6];
	float4 faceEmittance[6];
	[unroll]
	for(int i = 0; i < 6; i++)
	{
		faceAdress[i] = uint3(voxelID.x, voxelID.y + volumeRes * i, voxelID.z);
		uint4 coorsd = uint4(faceAdress[i], 0);

		uint opacitySample = opacityVolume.Load(coorsd);
		faceColor[i] = DecodeVoxelColor( colorVolume0.Load(coorsd), colorVolume1.Load(coorsd), DecodeVoxelOpacity(opacitySample) );

		faceNormal[i] = DecodeVoxelNormal(normalVolume.Load(coorsd), opacitySample);

		faceEmittance[i] = 0;
	}	

	[loop]
	for(int spotID = 0; spotID < (int)spotCount; spotID++)
	{
		SpotVoxelBuffer lightData = spotLightInjectBuffer[spotID];
	
		const float3 unnormL = lightData.PosRange.xyz - wpos;
		
		const float DoUL = dot(lightData.DirConeY.xyz, -unnormL);
		if(DoUL <= 0)
			continue;
		
		float illuminance = getDistanceAtt( unnormL, lightData.PosRange.w );
		if(illuminance == 0) 
			continue;
		
		const float3 L = normalize(unnormL);
			 
		illuminance *= getAngleAtt(L, -lightData.DirConeY.xyz, lightData.ColorConeX.w, lightData.DirConeY.w);
		if(illuminance == 0)
			continue; 

		float3 colorIlluminance = illuminance * lightData.ColorConeX.rgb;
		
		[unroll]
		for(int k = 0; k < 6; k++)
			faceEmittance[k] += float4(faceColor[k].rgb * colorIlluminance * saturate(dot(faceNormal[k], L)), 0);
	}

	[unroll]
	for(int j = 0; j < 6; j++)
	{
		[branch]
		if( faceColor[j].w > 0.0f )
			emittanceVolume[faceAdress[j]] = float4(faceColor[j].rgb * faceColor[j].w, 0);
		else
			emittanceVolume[faceAdress[j]] = faceEmittance[j];
	}
}