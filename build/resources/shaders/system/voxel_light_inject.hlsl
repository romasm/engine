#include "../common/math.hlsl"
#include "../common/structs.hlsl"

#include "light_constants.hlsl"

#include "../common/voxel_helpers.hlsl"
#include "../common/light_helpers.hlsl" 

#include "direct_brdf.hlsl" 
 
    
#define GROUP_THREAD_COUNT 2

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
	VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX];
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
	uint level = voxelID.x / volumeData[0].volumeRes;
	uint face = voxelID.y / volumeData[0].volumeRes;

	uint3 voxelIDinLevel = voxelID;
	voxelIDinLevel.x = voxelIDinLevel.x % volumeData[0].volumeRes;
	voxelIDinLevel.y = voxelIDinLevel.y % volumeData[0].volumeRes;

	float3 wpos = (float3(voxelIDinLevel) + 0.5f) * volumeData[level].voxelSize;
	wpos += volumeData[level].cornerOffset;

	const float3 voxelVector[4] = 
	{
		float3(volumeData[level].voxelSize, volumeData[level].voxelSize, volumeData[level].voxelSize),
		float3(-volumeData[level].voxelSize, volumeData[level].voxelSize, volumeData[level].voxelSize),
		float3(volumeData[level].voxelSize, -volumeData[level].voxelSize, volumeData[level].voxelSize),
		float3(-volumeData[level].voxelSize, -volumeData[level].voxelSize, volumeData[level].voxelSize)
	};

	const float voxelSizeThird = 0.33333f * volumeData[level].voxelSize;

	uint4 coorsd = uint4(voxelID, 0);

	uint opacitySample = opacityVolume.Load(coorsd);

	float faceOpacity = DecodeVoxelOpacity(opacitySample);
	[branch] 
	if( faceOpacity == 0 )
		return;

	float4 faceColor = DecodeVoxelColor( colorVolume0.Load(coorsd), colorVolume1.Load(coorsd), faceOpacity );
	faceOpacity = saturate(faceOpacity * VOXEL_SUBSAMPLES_COUNT_RCP);
	[branch]
	if( faceColor.w > 0 )
	{
		emittanceVolume[voxelID] = float4(faceColor.rgb * faceColor.w, faceOpacity);
		return;
	} 
	 
	float3 faceNormal = DecodeVoxelNormal(normalVolume.Load(coorsd), opacitySample);

	float3 faceEmittance = 0;
	
	float3 diffuseColor = Diffuse_Lambert(faceColor.rgb);

	[loop]
	for(int spotID = 0; spotID < (int)spotCount; spotID++)
	{
		SpotVoxelBuffer spotLightData = spotLightInjectBuffer[spotID];
	
		const float3 unnormL = spotLightData.PosRange.xyz - wpos;
		
		const float DoUL = dot(spotLightData.DirConeY.xyz, -unnormL);
		if(DoUL <= 0)
			continue;
		 
		float illuminance = getDistanceAtt( unnormL, spotLightData.PosRange.w );
		if(illuminance <= 0) 
			continue;
		
		const float3 L = normalize(unnormL);
			 
		const float3 virtUnnormL = spotLightData.Virtpos.xyz - wpos;
		const float3 virtL = normalize(virtUnnormL);

		illuminance *= getAngleAtt(virtL, -spotLightData.DirConeY.xyz, spotLightData.ColorConeX.w, spotLightData.DirConeY.w);
		if(illuminance <= 0)
			continue; 

		float4 samplePoint = float4(wpos, 1.0f);

		float offset = 0;
		[unroll] 
		for(int h = 0; h < 4; h++) 
			offset = max(offset, abs(dot( voxelVector[h], L )));
		samplePoint.xyz += L * offset;

		float light_blocked = 0; 
		[loop]
		for(int shadowAA = 0; shadowAA < VOXEL_SHADOW_AA; shadowAA++) // optimize to piramid 4 points
		{
			float4 aaPoint = samplePoint;  
			aaPoint.xyz += shadowVoxelOffsets[shadowAA] * voxelSizeThird;
			light_blocked += GetVoxelSpotShadow(samplerPointClamp, shadowsAtlas, aaPoint, spotLightData);
		}
		light_blocked *= VOXEL_SHADOW_AA_RCP;
		
		float3 colorIlluminance = light_blocked * illuminance * spotLightData.ColorConeX.rgb;
		
		faceEmittance += diffuseColor * colorIlluminance * saturate(dot(faceNormal, L));
	}

	[loop]
	for(int dirID = 0; dirID < (int)dirCount; dirID++)
	{
		DirVoxelBuffer dirLightData = dirLightInjectBuffer[dirID];

		float4 samplePoint = float4(wpos, 1.0f);

		const float3 L = -dirLightData.Dir.xyz;

		float offset = 0;
		[unroll] 
		for(int h = 0; h < 4; h++) 
			offset = max(offset, abs(dot( voxelVector[h], L )));
		samplePoint.xyz += L * offset;
		
		float light_blocked = 0; 
		[loop]
		for(int shadowAA = 0; shadowAA < VOXEL_SHADOW_AA; shadowAA++) // optimize to piramid 4 points
		{ 
			float4 aaPoint = samplePoint;  
			aaPoint.xyz += shadowVoxelOffsets[shadowAA] * voxelSizeThird;
			light_blocked += GetVoxelDirShadow(samplerPointClamp, shadowsAtlas, aaPoint, dirLightData);
		}
		light_blocked *= VOXEL_SHADOW_AA_RCP;

		float3 colorIlluminance = light_blocked * dirLightData.Color.rgb;
		
		faceEmittance += diffuseColor * colorIlluminance * saturate(dot(faceNormal, L));
	}

	emittanceVolume[voxelID] = float4(faceEmittance, faceOpacity);
}