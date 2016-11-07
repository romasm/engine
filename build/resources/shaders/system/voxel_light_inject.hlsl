#include "../common/math.hlsl"
#include "../common/structs.hlsl"

#include "light_constants.hlsl"

#include "../common/voxel_helpers.hlsl"
#include "../common/light_helpers.hlsl" 
 
    
#define GROUP_THREAD_COUNT 8

RWTexture3D <float4> emittanceVolume : register(u0);  

SamplerState samplerPointClamp : register(s0);

Texture2DArray <float> shadowsAtlas : register(t0);   

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
	
	float voxelDiag;
	float _padding0;
	float _padding1;
	float _padding2;
};

cbuffer lightCountBuffer : register(b1)
{
	uint spotCount;
	uint pointCount;
	uint dirCount;
	uint _padding;
};

[numthreads( GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, GROUP_THREAD_COUNT )]
void InjectLightToVolume(uint3 voxelID : SV_DispatchThreadID)
{
	float3 wpos = (float3(voxelID) + 0.5f) * voxelSize;
	wpos += cornerOffset;

	const float3 voxelVector[4] = 
	{
		float3(voxelSize, voxelSize, voxelSize),
		float3(-voxelSize, voxelSize, voxelSize),
		float3(voxelSize, -voxelSize, voxelSize),
		float3(-voxelSize, -voxelSize, voxelSize)
	};

	const float voxelSizeThird = 0.33333f * voxelSize;

	uint3 faceAdress[6];
	float4 faceColor[6];
	float faceOpacity[6];
	float3 faceNormal[6];
	float3 faceEmittance[6];

	float is_emissive = 1.0f;
	[unroll]
	for(int i = 0; i < 6; i++)
	{
		faceAdress[i] = uint3(voxelID.x, voxelID.y + volumeRes * i, voxelID.z);
		uint4 coorsd = uint4(faceAdress[i], 0);

		uint opacitySample = opacityVolume.Load(coorsd);

		faceOpacity[i] = DecodeVoxelOpacity(opacitySample);
		faceColor[i] = DecodeVoxelColor( colorVolume0.Load(coorsd), colorVolume1.Load(coorsd), faceOpacity[i] );
		is_emissive = min(is_emissive, faceColor[i].w);

		faceNormal[i] = DecodeVoxelNormal(normalVolume.Load(coorsd), opacitySample);

		faceEmittance[i] = 0; 
		faceOpacity[i] = saturate(faceOpacity[i] * VOXEL_SUBSAMPLES_COUNT_RCP);
	}	

	[branch]
	if(is_emissive > 0.0f)
	{
		[unroll]
		for(int j = 0; j < 6; j++)
			emittanceVolume[faceAdress[j]] = float4(faceColor[j].rgb * faceColor[j].w, faceOpacity[j]);
		return;
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
		if(illuminance <= 0) 
			continue;
		
		const float3 L = normalize(unnormL);
			 
		const float3 virtUnnormL = lightData.Virtpos.xyz - wpos;
		const float3 virtL = normalize(virtUnnormL);

		illuminance *= getAngleAtt(virtL, -lightData.DirConeY.xyz, lightData.ColorConeX.w, lightData.DirConeY.w);
		if(illuminance <= 0)
			continue; 

		float4 samplePoint = float4(wpos, 1.0f);

		float offset = 0;
		[unroll]
		for(int h = 0; h < 4; h++)
			offset = max(offset, abs(dot( voxelVector[h], L )));
		samplePoint.xyz += L * offset;

		float light_blocked = 0;
		[unroll]
		for(int shadowAA = 0; shadowAA < VOXEL_SHADOW_AA; shadowAA++) // optimize
		{
			float4 aaPoint = samplePoint;
			aaPoint.xyz += shadowVoxelOffsets[shadowAA] * voxelSizeThird;
			light_blocked += GetVoxelSpotShadow(samplerPointClamp, shadowsAtlas, aaPoint, lightData);
		}
		light_blocked *= VOXEL_SHADOW_AA_RCP;

		if(light_blocked == 0)
			continue;		

		float3 colorIlluminance = light_blocked * illuminance * lightData.ColorConeX.rgb;
		
		[unroll]
		for(int k = 0; k < 6; k++)
			faceEmittance[k] += faceColor[k].rgb * colorIlluminance * saturate(dot(faceNormal[k], L));
	}

	[unroll]
	for(int j = 0; j < 6; j++)
	{
		float4 final = faceColor[j].w > 0.0f ? 
			float4(faceColor[j].rgb * faceColor[j].w, faceOpacity[j]) :
			float4(faceEmittance[j], faceOpacity[j]);
		
		emittanceVolume[faceAdress[j]] = final;
	}
}