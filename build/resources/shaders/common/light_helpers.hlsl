
#define VOXEL_SHADOW_BIAS 0.00025

#define VOXEL_SHADOW_AA 8
#define VOXEL_SHADOW_AA_RCP 1.0f / VOXEL_SHADOW_AA
static const float3 shadowVoxelOffsets[VOXEL_SHADOW_AA] = 
{
	float3(1, 1, 1),
	float3(-1, 1, 1),
	float3(1, -1, 1),
	float3(-1, -1, 1),
	float3(1, 1, -1),
	float3(-1, 1, -1),
	float3(1, -1, -1),
	float3(-1, -1, -1)
};

static const float2 reproj[2] =
{
	float2(0.5f,-0.5f),
	float2(0.5f,0.5f)
};

float GetVoxelSpotShadow(sampler samp, Texture2DArray <float> shadowmap, float4 wpos, SpotVoxelBuffer lightData)
{
	float4 lightViewProjPos = mul(wpos, lightData.matViewProj);
	lightViewProjPos.xyz /= lightViewProjPos.w;

	float2 reprojCoords = reproj[0] * lightViewProjPos.xy + reproj[1];
	if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
		return 0;

	float3 shadowmapCoords;
	shadowmapCoords.xy = lightData.ShadowmapAdress.xy + reprojCoords.xy * lightData.ShadowmapAdress.z;
	shadowmapCoords.z = lightData.ShadowmapAdress.w;
	
	float shmDepth = shadowmap.SampleLevel(samp, shadowmapCoords, 0);
	return float(shmDepth > lightViewProjPos.z);
}


float GetVoxelDirShadow(sampler samp, Texture2DArray <float> shadowmap, float4 wpos, DirVoxelBuffer lightData)
{
	float4 adress = 0;

	float4 lightViewProjPos = mul(wpos, lightData.ViewProj0);	
	if(abs(lightViewProjPos.x) < 1 && abs(lightViewProjPos.y) < 1 && lightViewProjPos.z < 1) // 0 cascede
	{
		adress = lightData.ShadowmapAdress0;
	}
	else
	{
		lightViewProjPos = mul(wpos, lightData.ViewProj1);
		if(abs(lightViewProjPos.x) < 1 && abs(lightViewProjPos.y) < 1 && lightViewProjPos.z < 1) // 1 cascede
		{
			adress = lightData.ShadowmapAdress1;
		}
		else
		{
			lightViewProjPos = mul(wpos, lightData.ViewProj2);
			if(abs(lightViewProjPos.x) < 1 && abs(lightViewProjPos.y) < 1 && lightViewProjPos.z < 1) // 2 cascede
			{
				adress = lightData.ShadowmapAdress2;
			}
			else // 3 cascade
			{
				lightViewProjPos = mul(wpos, lightData.ViewProj3);
				adress = lightData.ShadowmapAdress3;
			}
		}
	}

	lightViewProjPos.xyz /= lightViewProjPos.w;
		
	float2 reprojCoords = reproj[0] * lightViewProjPos.xy + reproj[1];
			
	float3 shadowmapCoords;
	shadowmapCoords.xy = adress.xy + reprojCoords.xy * adress.z;
	shadowmapCoords.z = adress.w;

	float shmDepth = shadowmap.SampleLevel(samp, shadowmapCoords, 0);
	return float(shmDepth > lightViewProjPos.z);
}

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