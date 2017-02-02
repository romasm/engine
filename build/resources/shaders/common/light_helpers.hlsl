
#define NOH_EPCILON 0.00001f

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

// VOXEL GI
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

	float2 reprojCoords = reproj[0] * lightViewProjPos.xy + reproj[1];
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

// LIGHTING
LightPrepared PrepareSpotLight(in SpotLightBuffer lightData, in GBufferData gbuffer)
{		
	LightPrepared result = 0;
	result.unnormL = lightData.PosRange.xyz - gbuffer.wpos;
	result.L = normalize(unnormL);
	result.DoUL = dot(lightData.DirConeY.xyz, -unnormL);
}

bool CalculateSpotLight(in SpotLightBuffer lightData, in LightPrepared preparedData, in GBufferData gbuffer, in DataForLightCompute mData, 
						in MaterialParams materialParams, float3 ViewVector, out LightComponents results)
{	
	results = 0;

	[branch]
	if(preparedData.DoUL <= 0)
		return false;
		
	float illuminance = getDistanceAtt( preparedData.unnormL, lightData.PosRange.w );
	[branch]
	if(illuminance == 0)
		return false;

	illuminance *= getAngleAtt(preparedData.L, -lightData.DirConeY.xyz, lightData.ColorConeX.w, lightData.DirConeY.w);
	[branch]
	if(illuminance == 0)
		return false;

	const float3 colorIlluminance = illuminance * lightData.ColorConeX.rgb;
	
	results.scattering = colorIlluminance * directSubScattering(gbuffer.subsurf, materialParams, preparedData.L, gbuffer.normal, ViewVector);

	const float NoL = saturate( dot(gbuffer.normal, preparedData.L) );
	[branch]
	if( NoL == 0.0f )
		return true;
	
	const float3 H = normalize( ViewVector + preparedData.L );
	const float VoH = saturate( dot(ViewVector, H) );
	const float NoH = saturate( dot(gbuffer.normal, H) + NOH_EPCILON );
	
	results.diffuse = results.specular = colorIlluminance * NoL;
	results.diffuse *= directDiffuseBRDF(gbuffer.albedo, mData.avgR, mData.NoV, NoL, VoH);
	results.specular *= directSpecularBRDF(gbuffer.reflectivity, mData.R, NoH, mData.NoV, NoL, VoH, H, gbuffer.tangent, gbuffer.binormal, mData.avgR);	
	return true;
}

LightPrepared PrepareDiskLight(in DiskLightBuffer lightData, in GBufferData gbuffer)
{		
	result.unnormL = lightData.PosRange.xyz - gbuffer.wpos;
	result.L = normalize(unnormL);
	result.virtUnnormL = lightData.VirtposEmpty.xyz - gbuffer.wpos;
	result.virtL = normalize(virtUnnormL);
	result.DoUL = dot(lightData.DirConeY.xyz, -unnormL);
}

bool CalculateDiskLight(in DiskLightBuffer lightData, in LightPrepared preparedData, in GBufferData gbuffer, in DataForLightCompute mData, 
						in MaterialParams materialParams, float3 ViewVector, out LightComponents results)
{
	results = 0;
	
	[branch]
	if(preparedData.DoUL <= 0)
		return false;
			
	const float sqrDist = dot( preparedData.unnormL, preparedData.unnormL );
			
	const float smoothFalloff = smoothDistanceAtt(sqrDist, lightData.PosRange.w);
	[branch]
	if(smoothFalloff == 0)
		return false;
	float coneFalloff = getAngleAtt(preparedData.virtL, -lightData.DirConeY.xyz, lightData.ColorConeX.w, lightData.DirConeY.w);
	[branch]
	if(coneFalloff == 0)
		return false;
	coneFalloff *= smoothFalloff;
	
	const float NoL = dot(gbuffer.normal, preparedData.L);
				
	// specular
	const float e = clamp(dot(lightData.DirConeY.xyz, mData.reflect), -1, -0.0001f);
	const float3 planeRay = gbuffer.wpos - mData.reflect * preparedData.DoUL / e;
	const float3 newL = planeRay - lightData.PosRange.xyz;
		
	const float SphereAngle = clamp( -e * lightData.AreaInfoEmpty.x / max(sqrt( sqrDist ), lightData.AreaInfoEmpty.x), 0, 0.5 );
	const float specEnergy = Square( mData.aGGX / saturate( mData.aGGX + SphereAngle ) );
		
	const float3 specL = normalize(preparedData.unnormL + normalize(newL) * clamp(length(newL), 0, lightData.AreaInfoEmpty.x));
				
	//diffuse
	const float3 diffL = normalize(preparedData.unnormL + gbuffer.normal * lightData.AreaInfoEmpty.x * (1 - saturate(NoL)));	
			
	// Disk evaluation
	const float sinSigmaSqr = lightData.AreaInfoEmpty.y / (lightData.AreaInfoEmpty.y + max(sqrDist, lightData.AreaInfoEmpty.y));
	float noDirIlluminance;
	float illuminance = illuminanceSphereOrDisk( NoL, sinSigmaSqr, noDirIlluminance );
		
	coneFalloff *= saturate(dot(lightData.DirConeY.xyz, -preparedData.L));
		
	illuminance *= coneFalloff;
	noDirIlluminance *= coneFalloff;
			
	Light.diffuse += noDirIlluminance * lightData.ColorConeX.rgb * directSubScattering(gbuffer.subsurf, materialParams, preparedData.L, gbuffer.normal, ViewVector);

	[branch]
	if(illuminance == 0)
		return true;
		
	const float diffNoL = saturate(dot(gbuffer.normal, diffL));	
	const float3 diffH = normalize(ViewVector + diffL);
	const float diffVoH = saturate(dot(ViewVector, diffH));

	const float specNoL = saturate( dot(gbuffer.normal, specL) );
	const float3 specH = normalize(ViewVector + specL);
	const float specNoH = saturate( dot(gbuffer.normal, specH) + NOH_EPCILON );
	const float specVoH = saturate( dot(ViewVector, specH) );
			
	results.diffuse = results.specular = illuminance * lightData.ColorConeX.rgb;
	results.specular *= specEnergy * directSpecularBRDF(gbuffer.reflectivity, gbuffer.R, specNoH, gbuffer.NoV, specNoL, specVoH, specH, gbuffer.tangent, gbuffer.binormal, mData.avgR);	
	results.diffuse *= directDiffuseBRDF(gbuffer.albedo, mData.avgR, gbuffer.NoV, diffNoL, diffVoH);
	return true;
}

LightPrepared PrepareRectLight(in RectLightBuffer lightData, in GBufferData gbuffer)
{		
	result.unnormL = lightData.PosRange.xyz - gbuffer.wpos;
	result.L = normalize(unnormL);
	result.virtUnnormL = lightData.VirtposAreaZ.xyz - gbuffer.wpos;
	result.virtL = normalize(virtUnnormL);
	result.DoUL = dot(lightData.DirConeY.xyz, -unnormL);
}

bool CalculateRectLight(in RectLightBuffer lightData, in LightPrepared preparedData, in GBufferData gbuffer, in DataForLightCompute mData, 
						in MaterialParams materialParams, float3 ViewVector, out LightComponents results)
{
	results = 0;
	
	[branch]
	if(preparedData.DoUL <= 0)
		return false;
			
	const float sqrDist = dot( preparedData.unnormL, preparedData.unnormL );
			
	const float smoothFalloff = smoothDistanceAtt(sqrDist, lightData.PosRange.w);
	[branch]
	if(smoothFalloff == 0)
		return false;
	float coneFalloff = getAngleAtt(preparedData.virtL, -lightData.DirConeY.xyz, lightData.ColorConeX.w, lightData.DirConeY.w);
	[branch]
	if(coneFalloff == 0)
		return false;
	coneFalloff *= smoothFalloff;
					
	// specular				
	const float RLengthL = rcp( max(sqrt( sqrDist ), lightData.DirUpAreaX.x) );
		
	const float e = clamp(dot(lightData.DirConeY.xyz, mData.reflect), -1, -0.0001f);
	const float3 planeRay = gbuffer.wpos - mData.reflect * preparedData.DoUL / e;
	const float3 newL = planeRay - lightData.PosRange.xyz;
		
	const float LineXAngle = clamp( -e * lightData.VirtposAreaZ.w * RLengthL, 0, 0.5 );
	const float LineYAngle = clamp( -e * lightData.DirSideAreaY.w * RLengthL, 0, 0.5 );
	const float specEnergy = mData.sqr_aGGX / ( saturate( mData.aGGX + LineXAngle ) * saturate( mData.aGGX + LineYAngle ) );
			
	const float3 specL = normalize(
		preparedData.unnormL + clamp(dot(newL, lightData.DirSideAreaY.xyz), -lightData.DirSideAreaY.w, lightData.DirSideAreaY.w) * 
		lightData.DirSideAreaY.xyz + clamp(dot(newL, lightData.DirUpAreaX.xyz), -lightData.VirtposAreaZ.w, lightData.VirtposAreaZ.w) * lightData.DirUpAreaX.xyz);
			
	//diffuse
	const float3 diffL = normalize( preparedData.unnormL + gbuffer.normal * lightData.DirUpAreaX.w * (1 - saturate(dot(gbuffer.normal, preparedData.L))) );	
				
	// Rect evaluation
	float noDirIlluminance;
	float illuminance = illuminanceRect(gbuffer.wpos, lightData.PosRange.xyz, preparedData.L, gbuffer.normal, lightData.DirConeY.xyz, 
		lightData.DirSideAreaY.xyz * lightData.DirSideAreaY.w, lightData.DirUpAreaX.xyz * lightData.VirtposAreaZ.w, noDirIlluminance);
	illuminance = max(0, illuminance);
	noDirIlluminance = max(0, noDirIlluminance);

	illuminance *= coneFalloff;
	noDirIlluminance *= coneFalloff;
			
	Light.diffuse += noDirIlluminance * lightData.ColorConeX.rgb * directSubScattering(gbuffer.subsurf, materialParams, preparedData.L, gbuffer.normal, ViewVector);

	[branch]
	if(illuminance == 0)
		return true;
		
	const float diffNoL = saturate(dot(gbuffer.normal, diffL));	
	const float3 diffH = normalize(ViewVector + diffL);
	const float diffVoH = saturate(dot(ViewVector, diffH));

	const float specNoL = saturate( dot(gbuffer.normal, specL) );
	const float3 specH = normalize(ViewVector + specL);
	const float specNoH = saturate( dot(gbuffer.normal, specH) + NOH_EPCILON );
	const float specVoH = saturate( dot(ViewVector, specH) );
			
	results.diffuse = results.specular = illuminance * lightData.ColorConeX.rgb;
	results.specular *= specEnergy * directSpecularBRDF(gbuffer.reflectivity, gbuffer.R, specNoH, gbuffer.NoV, specNoL, specVoH, specH, gbuffer.tangent, gbuffer.binormal, mData.avgR);	
	results.diffuse *= directDiffuseBRDF(gbuffer.albedo, mData.avgR, gbuffer.NoV, diffNoL, diffVoH);
	return true;
}