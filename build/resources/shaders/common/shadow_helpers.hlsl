#define PCF_DEPTH_TEST_SENCE 1000000.0f
#define PCF_PIXEL 1.0f / SHADOWS_BUFFER_RES

// TODO!!! configurate

#define normalShadowOffsetSpot 2
#define shadowBiasSpotArea 0.0008 
#define shadowBiasSpot shadowBiasSpotArea * SHADOW_NEARCLIP

#define normalShadowOffsetPoint 2
#define shadowBiasPoint shadowBiasSpotArea * SHADOW_NEARCLIP

#define normalShadowOffsetDir0 0.1 * 0.05
#define normalShadowOffsetDir1 1 * 0.05
#define normalShadowOffsetDir2 3 * 0.05
#define normalShadowOffsetDir3 2 * 0.05
#define shadowBiasDir0 0.000003

#undef ORTHO_SHADOW
#include "../common/PCF.hlsl"

float2 SpotlightShadow(sampler samp, Texture2DArray <float> shadowmap, in LightPrepared prepared, 
					  in SpotCasterBuffer lightData, in GBufferData gbuffer, float3 depthFix, bool scatter)
{
	const float VNoL = dot(gbuffer.vertex_normal, prepared.L);
	const float4 wpos = float4(gbuffer.wpos, 1.0);
	float4 lightViewProjPos = mul(wpos, lightData.matViewProj);
	float normalOffsetScale = clamp(1 - VNoL, depthFix.y, 1);
	float shadowMapTexelSize = lightData.ShadowmapParams.x * prepared.DoUL * lightData.ShadowmapParams.y;
	float4 shadowOffset = float4(gbuffer.vertex_normal * normalShadowOffsetSpot * normalOffsetScale * shadowMapTexelSize * depthFix.x, 0);
	
	float4 correctedWpos = wpos + shadowOffset;
	float4 uvOffset = mul(correctedWpos, lightData.matViewProj);
	lightViewProjPos.xy = uvOffset.xy;
		
	float lvp_rcp = rcp(lightViewProjPos.w);
	float2 reprojCoords = ClipToScreenConsts[0] * lightViewProjPos.xy * lvp_rcp + ClipToScreenConsts[1];
		
	float2 result;
	[branch]
	if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
	{
		result = 0;
	}
	else
	{
		float3 shadowmapCoords;
		shadowmapCoords.xy = lightData.ShadowmapAdress.xy + reprojCoords.xy * lightData.ShadowmapAdress.z;
		shadowmapCoords.z = lightData.ShadowmapAdress.w;
	
		lightViewProjPos.z -= shadowBiasSpotArea * lightData.ShadowmapParams.z * depthFix.z;
		float depthView = lightViewProjPos.z * lvp_rcp;

		/*const float resBiasScale = max(2, (lightData.ShadowmapParams.x * SHADOWS_RES_BIAS_SCALE) * 0.2); 
		lightViewProjPos.z -= shadowBiasSpot * min(10, depthFix.z * resBiasScale);
		float depthView = lightViewProjPos.z * lvp_rcp;*/
	
		result = PCF_Filter(samp, shadowmap, shadowmapCoords, depthView, gbuffer, lightData.farNear, scatter);
	}
	return result;
}

/*static const float3 pl_dirs[6] =
{
	float3(1,0,0),
	float3(-1,0,0),
	float3(0,0,1),
	float3(0,0,-1),
	float3(0,1,0),
	float3(0,-1,0)
};*/
float2 PointlightShadow(sampler samp, Texture2DArray <float> shadowmap, in LightPrepared prepared, 
					   in PointCasterBuffer lightData, in GBufferData gbuffer, float3 depthFix, bool scatter)
{
	const float3 posInLight = mul(float4(gbuffer.wpos, 1.0f), lightData.matView).xyz;
	const float zInLSq[3] = {posInLight.x, posInLight.z, posInLight.y};
	float zInLSqAbs[3];
	[unroll]for(uint q=0; q<3; q++) zInLSqAbs[q] = abs(zInLSq[q]);
	
	const float VNoL = saturate(dot(gbuffer.vertex_normal, prepared.L));
	const float normalOffsetScale = clamp(1 - VNoL, depthFix.y, 1);
	
	float4 pil;
	float4 corWpos;
	float4 adress;
	[branch]
	if(zInLSqAbs[0]>=zInLSqAbs[1] && zInLSqAbs[0]>=zInLSqAbs[2]) // TODO: shadow bias wrong
	{
		[branch]
		if(zInLSq[0]>0)
		{
			pil = float4(-posInLight.z,posInLight.y,posInLight.x,1);
			const float shadowMapTexelSize = lightData.ColorShParams.w * lightData.ShadowmapParams0.r * pil.z;
			float3 shadowOffset = gbuffer.vertex_normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			corWpos = float4((gbuffer.wpos + shadowOffset) - lightData.PosRange.xyz, 1);
			corWpos = float4(-corWpos.z,corWpos.y,corWpos.x, 1);
			adress = lightData.ShadowmapAdress0;
		}
		else
		{
			pil = float4(posInLight.z,posInLight.y,-posInLight.x,1);
			const float shadowMapTexelSize = lightData.ColorShParams.w * lightData.ShadowmapParams0.g * pil.z;		
			float3 shadowOffset = gbuffer.vertex_normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			corWpos = float4((gbuffer.wpos + shadowOffset) - lightData.PosRange.xyz, 1);
			corWpos = float4(corWpos.z,corWpos.y,-corWpos.x, 1);
			adress = lightData.ShadowmapAdress1;
		}
	}
	else if(zInLSqAbs[1]>=zInLSqAbs[2])
	{
		[branch]
		if(zInLSq[1]>0)
		{
			pil = float4(posInLight,1);
			const float shadowMapTexelSize = lightData.ColorShParams.w * lightData.ShadowmapParams0.b * pil.z;
			float3 shadowOffset = gbuffer.vertex_normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			corWpos = float4((gbuffer.wpos + shadowOffset) - lightData.PosRange.xyz, 1);
			adress = lightData.ShadowmapAdress2;
		}
		else
		{
			pil = float4(-posInLight.x,posInLight.y,-posInLight.z,1);
			const float shadowMapTexelSize = lightData.ColorShParams.w * lightData.ShadowmapParams0.a * pil.z;
			float3 shadowOffset = gbuffer.vertex_normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			corWpos = float4((gbuffer.wpos + shadowOffset) - lightData.PosRange.xyz, 1);
			corWpos = float4(-corWpos.x,corWpos.y,-corWpos.z, 1);
			adress = lightData.ShadowmapAdress3;
		}
	}
	else
	{
		[branch]
		if(zInLSq[2]>0)
		{
			pil = float4(posInLight.z,posInLight.x,posInLight.y,1);
			const float shadowMapTexelSize = lightData.ColorShParams.w * lightData.ShadowmapParams1.r * pil.z;
			float3 shadowOffset = gbuffer.vertex_normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			corWpos = float4((gbuffer.wpos + shadowOffset) - lightData.PosRange.xyz, 1);
			corWpos = float4(corWpos.z,corWpos.x,corWpos.y, 1);
			adress = lightData.ShadowmapAdress4;
		}
		else
		{
			pil = float4(posInLight.z,-posInLight.x,-posInLight.y,1);
			const float shadowMapTexelSize = lightData.ColorShParams.w * lightData.ShadowmapParams1.g * pil.z;
			float3 shadowOffset = gbuffer.vertex_normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			corWpos = float4((gbuffer.wpos + shadowOffset) - lightData.PosRange.xyz, 1);
			corWpos = float4(corWpos.z,-corWpos.x,-corWpos.y, 1);
			adress = lightData.ShadowmapAdress5;
		}
	}
			
	float4 uvOffset = mul(corWpos, lightData.matProj);
	float4 lightViewProjPos = mul(pil, lightData.matProj);
	lightViewProjPos.xy = uvOffset.xy;
				
	const float lvp_rcp = rcp(lightViewProjPos.w);
	float2 reprojCoords = ClipToScreenConsts[0] * lightViewProjPos.xy * lvp_rcp + ClipToScreenConsts[1];
			
	// REMOVE
	if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
		return false;
					
	float3 shadowmapCoords;
	shadowmapCoords.xy = adress.xy + reprojCoords.xy * adress.z;
	shadowmapCoords.z = adress.w;
	lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
	float depthView = lightViewProjPos.z * lvp_rcp;

	return PCF_Filter(samp, shadowmap, shadowmapCoords, depthView, gbuffer, lightData.farNear, scatter);
}

#define ORTHO_SHADOW
#include "../common/PCF.hlsl"

#if DEBUG_CASCADE_LIGHTS != 0 // TODO: hard triangles
float3
#else
float2
#endif
DirlightShadow(sampler samp, Texture2DArray <float> shadowmap, in DirLightBuffer lightData, in float3 L, in GBufferData gbuffer, float3 depthFix, bool scatter)
{
#if DEBUG_CASCADE_LIGHTS != 0
	float3 res = 0;
#endif

	const float4 wpos = float4(gbuffer.wpos, 1.0);
	const float VNoL = saturate(dot(gbuffer.vertex_normal, L));
	
	matrix viewproj;
	float4 adress;
	float normalShadowOffsetDir;
	float farClip;
	
	float4 lightViewProjPos = mul(wpos, lightData.matViewProj0);
	[branch]
	if(abs(lightViewProjPos.x) < 1 && abs(lightViewProjPos.y) < 1 && lightViewProjPos.z < 1) // 0 cascede
	{
		viewproj = lightData.matViewProj0;
		adress = lightData.ShadowmapAdress0;
		normalShadowOffsetDir = normalShadowOffsetDir0;
		farClip = DIRLIGHT_Z_CASCADE_0;
#if DEBUG_CASCADE_LIGHTS != 0
		res = float3(1.0, 0.0, 0.0);
#endif
	}
	else
	{
		lightViewProjPos = mul(wpos, lightData.matViewProj1);
		[branch]
		if(abs(lightViewProjPos.x) < 1 && abs(lightViewProjPos.y) < 1 && lightViewProjPos.z < 1) // 1 cascede
		{
			viewproj = lightData.matViewProj1;
			adress = lightData.ShadowmapAdress1;
			normalShadowOffsetDir = normalShadowOffsetDir1;
			farClip = DIRLIGHT_Z_CASCADE_1;
#if DEBUG_CASCADE_LIGHTS != 0
			res = float3(0.0, 1.0, 0.0);
#endif
		}
		else
		{
			lightViewProjPos = mul(wpos, lightData.matViewProj2);
			[branch]
			if(abs(lightViewProjPos.x) < 1 && abs(lightViewProjPos.y) < 1 && lightViewProjPos.z < 1) // 2 cascede
			{
				viewproj = lightData.matViewProj2;
				adress = lightData.ShadowmapAdress2;
				normalShadowOffsetDir = normalShadowOffsetDir2;
				farClip = DIRLIGHT_Z_CASCADE_2;
#if DEBUG_CASCADE_LIGHTS != 0
				res = float3(0.0, 1.0, 1.0);
#endif
			}
			else // 3 cascade
			{
				lightViewProjPos = mul(wpos, lightData.matViewProj3);
				viewproj = lightData.matViewProj3;
				adress = lightData.ShadowmapAdress3;
				normalShadowOffsetDir = normalShadowOffsetDir3;
				farClip = DIRLIGHT_Z_CASCADE_3;
#if DEBUG_CASCADE_LIGHTS != 0
				res = float3(0.0, 0.0, 1.0);
#endif
			}
		}
	}

	float normalOffsetScale = clamp(1 - VNoL, depthFix.y, 1);
		
	float4 shadowOffset = float4(gbuffer.vertex_normal * normalShadowOffsetDir * normalOffsetScale * depthFix.x, 0);
		
	float4 correctedWpos = wpos + shadowOffset;
	float4 uvOffset = mul(correctedWpos, viewproj);
	lightViewProjPos.xy = uvOffset.xy;
		
	float lvp_rcp = rcp(lightViewProjPos.w);
	float2 reprojCoords = ClipToScreenConsts[0] * lightViewProjPos.xy * lvp_rcp + ClipToScreenConsts[1];
			
	float3 shadowmapCoords;
	shadowmapCoords.xy = adress.xy + reprojCoords.xy * adress.z;
	shadowmapCoords.z = adress.w;
	
	lightViewProjPos.z -= shadowBiasDir0 * depthFix.z;
	float depthView = lightViewProjPos.z * lvp_rcp;

#if DEBUG_CASCADE_LIGHTS != 0
	return res * PCF_Filter_Ortho(samp, shadowmap, shadowmapCoords, depthView, gbuffer, farClip, scatter);
#else
	return PCF_Filter_Ortho(samp, shadowmap, shadowmapCoords, depthView, gbuffer, farClip, scatter);
#endif
}

// VOXEL CONE TRACING GI

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

float GetVoxelSpotShadow(sampler samp, Texture2DArray <float> shadowmap, float4 wpos, SpotVoxelBuffer lightData)
{
	float4 lightViewProjPos = mul(wpos, lightData.matViewProj);
	lightViewProjPos.xyz /= lightViewProjPos.w;

	float2 reprojCoords = ClipToScreenConsts[0] * lightViewProjPos.xy + ClipToScreenConsts[1];
	if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
		return 0;

	float3 shadowmapCoords;
	shadowmapCoords.xy = lightData.ShadowmapAdress.xy + reprojCoords.xy * lightData.ShadowmapAdress.z;
	shadowmapCoords.z = lightData.ShadowmapAdress.w;
	
	float shmDepth = shadowmap.SampleLevel(samp, shadowmapCoords, 0);
	return float(shmDepth > lightViewProjPos.z);
}

float GetVoxelPointShadow(sampler samp, Texture2DArray <float> shadowmap, float4 wpos, PointVoxelBuffer lightData)
{
	const float4 posInLight = mul(wpos, lightData.matView);

	const float zInLSq[3] = {posInLight.x, posInLight.z, posInLight.y};
	float zInLSqAbs[3];
	[unroll]
	for(uint q=0; q<3; q++) 
		zInLSqAbs[q] = abs(zInLSq[q]);

	float4 adress = 0;
	float4 pil = 0;
	[branch]
	if(zInLSqAbs[0]>=zInLSqAbs[1] && zInLSqAbs[0]>=zInLSqAbs[2])
	{
		[branch]
		if(zInLSq[0]>0)
		{
			pil = float4(-posInLight.z,posInLight.y,posInLight.x,1);
			adress = lightData.ShadowmapAdress0;
		}
		else
		{
			pil = float4(posInLight.z,posInLight.y,-posInLight.x,1);
			adress = lightData.ShadowmapAdress1;
		}
	}
	else if(zInLSqAbs[1]>=zInLSqAbs[2])
	{
		[branch]
		if(zInLSq[1]>0)
		{
			pil = posInLight;
			adress = lightData.ShadowmapAdress2;
		}
		else
		{
			pil = float4(-posInLight.x,posInLight.y,-posInLight.z,1);
			adress = lightData.ShadowmapAdress3;
		}
	}
	else
	{
		[branch]
		if(zInLSq[2]>0)
		{
			pil = float4(posInLight.z,posInLight.x,posInLight.y,1);
			adress = lightData.ShadowmapAdress4;
		}
		else
		{
			pil = float4(posInLight.z,-posInLight.x,-posInLight.y,1);
			adress = lightData.ShadowmapAdress5;
		}
	}

	float4 lightViewProjPos = mul(pil, lightData.matProj);
	lightViewProjPos.xyz /= lightViewProjPos.w;

	float2 reprojCoords = ClipToScreenConsts[0] * lightViewProjPos.xy + ClipToScreenConsts[1];
	if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
		return 0;

	float3 shadowmapCoords;
	shadowmapCoords.xy = adress.xy + reprojCoords.xy * adress.z;
	shadowmapCoords.z = adress.w;
	
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
		
	float2 reprojCoords = ClipToScreenConsts[0] * lightViewProjPos.xy + ClipToScreenConsts[1];
			
	float3 shadowmapCoords;
	shadowmapCoords.xy = adress.xy + reprojCoords.xy * adress.z;
	shadowmapCoords.z = adress.w;

	float shmDepth = shadowmap.SampleLevel(samp, shadowmapCoords, 0);
	return float(shmDepth > lightViewProjPos.z);
}