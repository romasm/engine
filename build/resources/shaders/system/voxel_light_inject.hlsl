#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "light_constants.hlsl"

#define GROUP_THREAD_COUNT 8

RWTexture3D <uint> colorVolume0 : register(u0);  
RWTexture3D <uint> colorVolume1 : register(u1);  

SamplerState samplerBilinearClamp : register(s0);

Texture2D <float> shadowsAtlas : register(t0);  

StructuredBuffer<SpotVoxelBuffer> spotLightInjectBuffer : register(t1); 
StructuredBuffer<PointVoxelBuffer> pointLightInjectBuffer : register(t2); 
StructuredBuffer<DirVoxelBuffer> dirLightInjectBuffer : register(t3); 

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

	uint3 faceAdress[6] = {
		uint3(voxelID.x, voxelID.y, voxelID.z),
		uint3(voxelID.x, voxelID.y + volumeRes, voxelID.z),
		uint3(voxelID.x, voxelID.y + volumeRes * 2, voxelID.z),
		uint3(voxelID.x, voxelID.y + volumeRes * 3, voxelID.z),
		uint3(voxelID.x, voxelID.y + volumeRes * 4, voxelID.z),
		uint3(voxelID.x, voxelID.y + volumeRes * 5, voxelID.z),
	};

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
		
		if( L.x < 0 )
			colorVolume1[faceAdress[0]] |= 0x0000ffff;
		else
			colorVolume1[faceAdress[1]] |= 0x0000ffff;

		if( L.y < 0 )
			colorVolume1[faceAdress[2]] |= 0x0000ffff;
		else
			colorVolume1[faceAdress[3]] |= 0x0000ffff;

		if( L.z < 0 )
			colorVolume1[faceAdress[4]] |= 0x0000ffff;
		else
			colorVolume1[faceAdress[5]] |= 0x0000ffff;
	}
}