
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
bool PrepareSpotLight(in SpotLightBuffer lightData, in GBufferData gbuffer, in DataForLightCompute mData, in MaterialParams materialParams, 
						float3 ViewVector, out LightComponents results)
{		
	results = 0;
	shadowHelpers = 0;

	const float3 unnormL = lightData.PosRange.xyz - gbuffer.wpos;
	const float3 L = normalize(unnormL);
		
	const float DoUL = dot(lightData.DirConeY.xyz, -unnormL);
	[branch]
	if(DoUL <= 0)
		return false;
		
	float illuminance = getDistanceAtt( unnormL, lightData.PosRange.w );
	[branch]
	if(illuminance == 0)
		return false;

	illuminance *= getAngleAtt(L, -lightData.DirConeY.xyz, lightData.ColorConeX.w, lightData.DirConeY.w);
	[branch]
	if(illuminance == 0)
		return false;

	return true;
}

bool CalculateSpotLight(in SpotLightBuffer lightData, in GBufferData gbuffer, in DataForLightCompute mData, in MaterialParams materialParams, 
						float3 ViewVector, out LightComponents results)
{	
	const float3 colorIlluminance = illuminance * lightData.ColorConeX.rgb;
		
	shadowHelpers.DoUL = DoUL;
	shadowHelpers.L = L;

	results.scattering = colorIlluminance * directSubScattering(gbuffer.subsurf, materialParams, L, normal, VtoWP);

	const float NoL = saturate( dot(gbuffer.normal, L) );
	[branch]
	if( NoL == 0.0f )
		return true;
	
	const float3 H = normalize( ViewVector + L );
	const float VoH = saturate( dot(ViewVector, H) );
	const float NoH = saturate( dot(gbuffer.normal, H) + NOH_EPCILON );
	
	results.diffuse = results.specular = colorIlluminance * NoL;
	results.diffuse *= directDiffuseBRDF(gbuffer.albedo, mData.avgR, mData.NoV, NoL, VoH);
	results.specular *= directSpecularBRDF(gbuffer.reflectivity, mData.R, NoH, mData.NoV, NoL, VoH, H, gbuffer.tangent, gbuffer.binormal, mData.avgR);	
	return true;
}

bool CalculateDiskLight(in DiskLightBuffer lightData, in GBufferData gbuffer, in DataForLightCompute mData, in MaterialParams materialParams, 
						float3 ViewVector, out LightComponents results, out ShadowHelpers shadowHelpers)
{		
	results = 0;
	shadowHelpers = 0;

	const float3 unnormL = lightData.PosRange.xyz - gbuffer.wpos;
	const float3 L = normalize(unnormL);
		
	const float DoUL = dot(lightData.DirConeY.xyz, -unnormL);
	[branch]
	if(DoUL <= 0)
		return false;

	const float sqrDist = dot( unnormL, unnormL );
		
	const float smoothFalloff = smoothDistanceAtt(sqrDist, lightData.PosRange.w);
	[branch]
	if(smoothFalloff == 0)
		continue;
	float coneFalloff = getAngleAtt(normalize(lightData.VirtposEmpty.rgb - gbuffer.wpos), -lightData.DirConeY.xyz, 
		lightData.ColorConeX.w, lightData.DirConeY.w);
	[branch]
	if(coneFalloff == 0)
		continue;
	coneFalloff *= smoothFalloff;
		
	const float NoL = dot(gbuffer.normal, L);
				
	// specular
	const float e = clamp(dot(dir_coney.xyz, Refl), -1, -0.0001f);
	const float3 planeRay = wpos - Refl * DoUL / e;
	const float3 newL = planeRay - pos_range.xyz;
		
	const float SphereAngle = clamp( -e * rad_sqrrad.x / max(sqrt( sqrDist ), rad_sqrrad.x), 0, 0.5 );
	const float specEnergy = Square( aGGX / saturate( aGGX + SphereAngle ) );
		
	const float3 specL = normalize(unnormL + normalize(newL) * clamp(length(newL), 0, rad_sqrrad.x));
				
	//diffuse
	const float3 diffL = normalize(unnormL + normal * rad_sqrrad.x * (1 - saturate(NoL)));	

	// Disk evaluation
	const float sinSigmaSqr = rad_sqrrad.y / (rad_sqrrad.y + max(sqrDist, rad_sqrrad.y));
	float noDirIlluminance;
	float illuminance = illuminanceSphereOrDisk( NoL, sinSigmaSqr, noDirIlluminance );
		
	coneFalloff *= saturate(dot(dir_coney.xyz, -L));
		
	illuminance *= coneFalloff;
	noDirIlluminance *= coneFalloff;

		
	shadowHelpers.DoUL = DoUL;
	shadowHelpers.L = L;

	results.scattering = colorIlluminance * directSubScattering(gbuffer.subsurf, materialParams, L, normal, VtoWP);;

	const float NoL = saturate( dot(gbuffer.normal, L) );
	if( NoL == 0.0f )
		return true;
	
	const float3 H = normalize( ViewVector + L );
	const float VoH = saturate( dot(ViewVector, H) );
	const float NoH = saturate( dot(gbuffer.normal, H) + NOH_EPCILON );
	
	results.diffuse = results.specular = colorIlluminance * NoL;
	results.diffuse *= directDiffuseBRDF(gbuffer.albedo, mData.avgR, mData.NoV, NoL, VoH);
	results.specular *= directSpecularBRDF(gbuffer.reflectivity, mData.R, NoH, mData.NoV, NoL, VoH, H, gbuffer.tangent, gbuffer.binormal, mData.avgR);	
	return true;
}