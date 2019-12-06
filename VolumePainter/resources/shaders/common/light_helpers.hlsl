#define DEBUG_CASCADE_LIGHTS 0

#define NOH_EPCILON 0.00001f

// rect
float rectangleSolidAngle( float3 worldPos ,float3 p0 , float3 p1 ,float3 p2 , float3 p3 )
{
	float3 v0 = p0 - worldPos ;
	float3 v1 = p1 - worldPos ;
	float3 v2 = p2 - worldPos ;
	float3 v3 = p3 - worldPos ;

	float3 n0 = normalize ( cross (v0 , v1 ));
	float3 n1 = normalize ( cross (v1 , v2 ));
	float3 n2 = normalize ( cross (v2 , v3 ));
	float3 n3 = normalize ( cross (v3 , v0 ));


	float g0 = acos ( dot (-n0 , n1 ));
	float g1 = acos ( dot (-n1 , n2 ));
	float g2 = acos ( dot (-n2 , n3 ));
	float g3 = acos ( dot (-n3 , n0 ));

	return g0 + g1 + g2 + g3 - PI_MUL2;
}

float illuminanceRect(float3 worldPos, float3 lightPos, float3 normL, float3 worldNormal, float3 planeNormal, float3 planeLeft, float3 planeUp, out float indirIllum)
{
	float3 p0 = lightPos - planeLeft + planeUp;
	float3 p1 = lightPos - planeLeft - planeUp;
	float3 p2 = lightPos + planeLeft - planeUp;
	float3 p3 = lightPos + planeLeft + planeUp;
	indirIllum = rectangleSolidAngle( worldPos, p0, p1, p2, p3 );

	float illuminance = indirIllum * 0.2 * (
		saturate( dot( normalize( p0 - worldPos ), worldNormal ) ) +
		saturate( dot( normalize( p1 - worldPos ), worldNormal ) ) +
		saturate( dot( normalize( p2 - worldPos ), worldNormal ) ) +
		saturate( dot( normalize( p3 - worldPos ), worldNormal ) ) +
		saturate( dot( normL, worldNormal )));
	
	return illuminance;
}

// sphere & disk
float illuminanceSphereOrDisk( float cosTheta, float sinSigmaSqr, out float indirIllum )
{
	float sqrCosTheta = cosTheta * cosTheta;
	float sinTheta = sqrt(1.0f - sqrCosTheta );
	float illuminance = 0.0f;
	
	indirIllum = PI * sinSigmaSqr;
	
	if( sqrCosTheta > sinSigmaSqr )
	{
		illuminance = indirIllum * saturate( cosTheta );
	}
	else
	{
		float x = sqrt(1.0f / sinSigmaSqr - 1.0f) ;
		float y = -x * ( cosTheta / sinTheta );
		float sinThetaSqrtY = sinTheta * sqrt(1.0f - y * y);
		illuminance = ( cosTheta * acos(y) - x * sinThetaSqrtY ) * sinSigmaSqr + atan(sinThetaSqrtY / x);
	}
	
	return max( illuminance, 0.0f);
}

// tube 
float illuminanceTube(float3 lightPos, float3 worldPos, float3 worldNormal, float lightRadius, float lightRadiusSqr, float3 nL, float3 L0, float3 Ld, float sqrLehgth, out float indirIllum )
{
	float t = dot(-L0, Ld) / sqrLehgth;
	
	// rect
	float3 forward = normalize( L0 + t * Ld );
	float3 left = Ld * 0.5;
	float3 up = normalize( cross( left, forward ) );

	up *= lightRadius;
	
	float nodirIlluminanceRect;
	float illuminance = illuminanceRect(worldPos, lightPos, nL, worldNormal, forward, left, up, nodirIlluminanceRect);

	// sphere
	float nodirIlluminanceSphere = 1;
	
	float3 sphereUnormL = L0 + saturate(t) * Ld;
	float3 sphereL = normalize( sphereUnormL );
	float sqrSphereDistance = dot( sphereUnormL, sphereUnormL );
	
	float cosTheta = clamp( dot(worldNormal, sphereL), -0.999, 0.999);
	float sinSigmaSqr = min( lightRadiusSqr / sqrSphereDistance, 0.9999f );
	float illuminanceSphere = illuminanceSphereOrDisk( cosTheta, sinSigmaSqr, nodirIlluminanceSphere );
	
	/*
	float3 sphereUnormL = L0 + saturate(t) * Ld;	
	float3 sphereL = normalize( sphereUnormL );
	float sqrSphereDistance = dot( sphereUnormL, sphereUnormL );

	float nodirIlluminanceSphere = PI * (( lightRadiusSqr ) / sqrSphereDistance );
	float illuminanceSphere = saturate( dot( sphereL, worldNormal )) * nodirIlluminanceSphere;
	*/
	
	illuminance += illuminanceSphere;
	indirIllum = nodirIlluminanceSphere + nodirIlluminanceRect;
	
	return illuminance;
}

// common
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
	LightPrepared result = (LightPrepared)0;
	result.unnormL = lightData.VirtposAreaZ.xyz - gbuffer.wpos;
	result.L = normalize(result.unnormL);
	result.DoUL = dot(lightData.DirConeY.xyz, -result.unnormL);
	return result;
}

bool CalculateSpotLight(in SpotLightBuffer lightData, in LightPrepared preparedData, in GBufferData gbuffer, in DataForLightCompute mData, 
						in MaterialParams materialParams, float3 ViewVector, float lightAmount, bool scatter, out LightComponents results)
{	
	results = (LightComponents)0;
	bool exec = false;

	[branch]
	if(preparedData.DoUL > 0)
	{
		float illuminance = getDistanceAtt( preparedData.unnormL, lightData.PosRange.w );
		[branch]
		if(illuminance > 0)
		{
			illuminance *= getAngleAtt(preparedData.L, -lightData.DirConeY.xyz, lightData.ColorConeX.w, lightData.DirConeY.w);
			[branch]
			if(illuminance > 0)
			{
				const float3 colorIlluminance = illuminance * lightData.ColorConeX.rgb;
	
				[branch] if(scatter)
				{
					results.scattering = colorIlluminance * directScattering(gbuffer, mData, materialParams, preparedData.L, ViewVector, lightAmount);
					exec = true;
				}

				const float NoL = saturate( dot(gbuffer.normal, preparedData.L) );
				[branch]
				if( NoL > 0.0f )
				{
					exec = true;

					const float3 H = normalize( ViewVector + preparedData.L );
					const float VoH = saturate( dot(ViewVector, H) );
					const float NoH = saturate( dot(gbuffer.normal, H) + NOH_EPCILON );
	
					results.diffuse = results.specular = colorIlluminance * NoL;
					results.diffuse *= directDiffuseBRDF(gbuffer.albedo, mData.avgR, mData.NoV, NoL, VoH);
					results.specular *= directSpecularBRDF(gbuffer.reflectivity, mData.R, NoH, mData.NoV, NoL, VoH, H, gbuffer.tangent, gbuffer.binormal, mData.avgRSq);
				}
			}
		}
	}
	return exec;
}

bool CalculateDiskLight(in SpotLightBuffer lightData, in LightPrepared preparedData, in GBufferData gbuffer, in DataForLightCompute mData,
						in MaterialParams materialParams, float3 ViewVector, float lightAmount, bool scatter, out LightComponents results)
{
	results = (LightComponents)0;
	bool exec = false;
	
	const float3 unnormL = lightData.PosRange.xyz - gbuffer.wpos;
	const float3 L = normalize(unnormL);
	const float DoUL = dot(lightData.DirConeY.xyz, -unnormL);

	[branch]
	if(DoUL > 0)
	{			
		const float sqrDist = dot( unnormL, unnormL );
			
		const float smoothFalloff = smoothDistanceAtt(sqrDist, lightData.PosRange.w);
		[branch]
		if(smoothFalloff > 0)
		{
			float coneFalloff = getAngleAtt(preparedData.L, -lightData.DirConeY.xyz, lightData.ColorConeX.w, lightData.DirConeY.w);
			[branch]
			if(coneFalloff > 0)
			{
				coneFalloff *= smoothFalloff;
	
				const float NoL = dot(gbuffer.normal, L);
				
				// specular
				const float e = clamp(dot(lightData.DirConeY.xyz, mData.reflect), -1, -0.0001f);
				const float3 planeRay = gbuffer.wpos - mData.reflect * DoUL / e;
				const float3 newL = planeRay - lightData.PosRange.xyz;
		
				const float SphereAngle = clamp( -e * lightData.DirUpAreaX.w / max(sqrt( sqrDist ), lightData.DirUpAreaX.w), 0, 0.5 );
				const float specEnergy = Square( mData.aGGX / saturate( mData.aGGX + SphereAngle ) );
		
				const float3 specL = normalize(unnormL + normalize(newL) * clamp(length(newL), 0, lightData.DirUpAreaX.w));
				
				//diffuse
				const float3 diffL = normalize(unnormL + gbuffer.normal * lightData.DirUpAreaX.w * (1 - saturate(NoL)));
			
				// Disk evaluation
				const float sinSigmaSqr = lightData.DirSideAreaY.w / (lightData.DirSideAreaY.w + max(sqrDist, lightData.DirSideAreaY.w));
				float noDirIlluminance;
				float illuminance = illuminanceSphereOrDisk( NoL, sinSigmaSqr, noDirIlluminance );
		
				coneFalloff *= saturate(dot(lightData.DirConeY.xyz, -L));
		
				illuminance *= coneFalloff;
				noDirIlluminance *= coneFalloff;
			
				[branch] if(scatter)
				{
					results.scattering = noDirIlluminance * lightData.ColorConeX.rgb * directScattering(gbuffer, mData, materialParams, L, ViewVector, lightAmount);
					exec = true;
				}

				[branch]
				if(illuminance > 0)
				{
					exec = true;
		
					const float diffNoL = saturate(dot(gbuffer.normal, diffL));	
					const float3 diffH = normalize(ViewVector + diffL);
					const float diffVoH = saturate(dot(ViewVector, diffH));

					const float specNoL = saturate( dot(gbuffer.normal, specL) );
					const float3 specH = normalize(ViewVector + specL);
					const float specNoH = saturate( dot(gbuffer.normal, specH) + NOH_EPCILON );
					const float specVoH = saturate( dot(ViewVector, specH) );
			
					results.diffuse = results.specular = illuminance * lightData.ColorConeX.rgb;
					results.specular *= specEnergy * directSpecularBRDF(gbuffer.reflectivity, mData.R, specNoH, mData.NoV, specNoL, specVoH, specH, gbuffer.tangent, gbuffer.binormal, mData.avgRSq);
					results.diffuse *= directDiffuseBRDF(gbuffer.albedo, mData.avgR, mData.NoV, diffNoL, diffVoH);
				}
			}
		}
	}
	return exec;
}

bool CalculateRectLight(in SpotLightBuffer lightData, in LightPrepared preparedData, in GBufferData gbuffer, in DataForLightCompute mData,
						in MaterialParams materialParams, float3 ViewVector, float lightAmount, bool scatter, out LightComponents results)
{
	results = (LightComponents)0;
	bool exec = false;
	
	const float3 unnormL = lightData.PosRange.xyz - gbuffer.wpos;
	const float3 L = normalize(unnormL);
	const float DoUL = dot(lightData.DirConeY.xyz, -unnormL);

	[branch]
	if(DoUL > 0)
	{			
		const float sqrDist = dot( unnormL, unnormL );
			
		const float smoothFalloff = smoothDistanceAtt(sqrDist, lightData.PosRange.w);
		[branch]
		if(smoothFalloff > 0)
		{
			float coneFalloff = getAngleAtt(preparedData.L, -lightData.DirConeY.xyz, lightData.ColorConeX.w, lightData.DirConeY.w);
			[branch]
			if(coneFalloff > 0)
			{
				coneFalloff *= smoothFalloff;
					
				// specular				
				const float RLengthL = rcp( max(sqrt( sqrDist ), lightData.DirUpAreaX.w) );
		
				const float e = clamp(dot(lightData.DirConeY.xyz, mData.reflect), -1, -0.0001f);
				const float3 planeRay = gbuffer.wpos - mData.reflect * DoUL / e;
				const float3 newL = planeRay - lightData.PosRange.xyz;
		
				const float LineXAngle = clamp( -e * lightData.VirtposAreaZ.w * RLengthL, 0, 0.5 );
				const float LineYAngle = clamp( -e * lightData.DirSideAreaY.w * RLengthL, 0, 0.5 );
				const float specEnergy = mData.sqr_aGGX / ( saturate( mData.aGGX + LineXAngle ) * saturate( mData.aGGX + LineYAngle ) );
			
				const float3 specL = normalize(
					unnormL + clamp(dot(newL, lightData.DirSideAreaY.xyz), -lightData.DirSideAreaY.w, lightData.DirSideAreaY.w) * 
					lightData.DirSideAreaY.xyz + clamp(dot(newL, lightData.DirUpAreaX.xyz), -lightData.VirtposAreaZ.w, lightData.VirtposAreaZ.w) * lightData.DirUpAreaX.xyz);
			
				//diffuse
				const float3 diffL = normalize( unnormL + gbuffer.normal * lightData.DirUpAreaX.w * (1 - saturate(dot(gbuffer.normal, L))) );	
				
				// Rect evaluation
				float noDirIlluminance;
				float illuminance = illuminanceRect(gbuffer.wpos, lightData.PosRange.xyz, L, gbuffer.normal, lightData.DirConeY.xyz, 
					lightData.DirSideAreaY.xyz * lightData.DirSideAreaY.w, lightData.DirUpAreaX.xyz * lightData.VirtposAreaZ.w, noDirIlluminance);
				illuminance = max(0, illuminance);
				noDirIlluminance = max(0, noDirIlluminance);

				illuminance *= coneFalloff;
				noDirIlluminance *= coneFalloff;
			
				[branch] if(scatter)
				{
					results.scattering = noDirIlluminance * lightData.ColorConeX.rgb * directScattering(gbuffer, mData, materialParams, L, ViewVector, lightAmount);
					exec = true;
				}

				[branch]
				if(illuminance > 0)
				{	
					exec = true;

					const float diffNoL = saturate(dot(gbuffer.normal, diffL));	
					const float3 diffH = normalize(ViewVector + diffL);
					const float diffVoH = saturate(dot(ViewVector, diffH));

					const float specNoL = saturate( dot(gbuffer.normal, specL) );
					const float3 specH = normalize(ViewVector + specL);
					const float specNoH = saturate( dot(gbuffer.normal, specH) + NOH_EPCILON );
					const float specVoH = saturate( dot(ViewVector, specH) );
			
					results.diffuse = results.specular = illuminance * lightData.ColorConeX.rgb;
					results.specular *= specEnergy * directSpecularBRDF(gbuffer.reflectivity, mData.R, specNoH, mData.NoV, specNoL, specVoH, specH, gbuffer.tangent, gbuffer.binormal, mData.avgRSq);
					results.diffuse *= directDiffuseBRDF(gbuffer.albedo, mData.avgR, mData.NoV, diffNoL, diffVoH);
				}
			}
		}
	}
	return exec;
}

LightPrepared PreparePointLight(in PointLightBuffer lightData, in GBufferData gbuffer)
{		
	LightPrepared result = (LightPrepared)0;
	result.unnormL = lightData.PosRange.xyz - gbuffer.wpos;
	result.L = normalize(result.unnormL);
	return result;
}

bool CalculatePointLight(in PointLightBuffer lightData, in LightPrepared preparedData, in GBufferData gbuffer, in DataForLightCompute mData, 
						in MaterialParams materialParams, float3 ViewVector, float lightAmount, bool scatter, out LightComponents results)
{	
	results = (LightComponents)0;
	bool exec = false;
	
	const float illuminance = getDistanceAtt( preparedData.unnormL, lightData.PosRange.w );
	[branch]
	if(illuminance > 0)
	{
		const float3 colorIlluminance = illuminance * lightData.Color.rgb;
		
		[branch] if(scatter)
		{
			results.scattering = colorIlluminance * directScattering(gbuffer, mData, materialParams, preparedData.L, ViewVector, lightAmount);
			exec = true;
		}

		const float NoL = saturate( dot(gbuffer.normal, preparedData.L) );
		[branch]
		if( NoL > 0.0f )
		{
			exec = true;

			const float3 H = normalize( ViewVector + preparedData.L );
			const float VoH = saturate( dot(ViewVector, H) );
			const float NoH = saturate( dot(gbuffer.normal, H) + NOH_EPCILON );
	
			results.diffuse = results.specular = colorIlluminance * NoL;
			results.diffuse *= directDiffuseBRDF(gbuffer.albedo, mData.avgR, mData.NoV, NoL, VoH);
			results.specular *= directSpecularBRDF(gbuffer.reflectivity, mData.R, NoH, mData.NoV, NoL, VoH, H, gbuffer.tangent, gbuffer.binormal, mData.avgRSq);
		}
	}
	return exec;
}

bool CalculateSphereLight(in PointLightBuffer lightData, in LightPrepared preparedData, in GBufferData gbuffer, in DataForLightCompute mData,
						in MaterialParams materialParams, float3 ViewVector, float lightAmount, bool scatter, out LightComponents results)
{	
	results = (LightComponents)0;
	bool exec = false;
	
	const float sqrDist = dot( preparedData.unnormL, preparedData.unnormL );
			
	const float smoothFalloff = smoothDistanceAtt(sqrDist, lightData.PosRange.w);
	[branch]
	if(smoothFalloff > 0)
	{
		// specular
		const float SphereAngle = clamp( lightData.AreaInfo.x / max(sqrt( sqrDist ), lightData.AreaInfo.x), 0, 0.5 );
		const float specEnergy = Square( mData.aGGX / saturate( mData.aGGX + SphereAngle ) );
		
		const float3 centerToRay = dot(preparedData.unnormL, mData.reflect) * mData.reflect - preparedData.unnormL;
		const float3 specL = normalize(preparedData.unnormL + centerToRay * saturate(lightData.AreaInfo.x / sqrt(dot(centerToRay, centerToRay))));
			
		//diffuse
		const float3 diffL = normalize(preparedData.unnormL + gbuffer.normal * lightData.AreaInfo.x * (1 - saturate(dot(gbuffer.normal, preparedData.L))));	
				
		// Sphere evaluation
		const float cosTheta = clamp( dot(gbuffer.normal, preparedData.L), -0.999, 0.999);
		const float sinSigmaSqr = min( lightData.AreaInfo.y / sqrDist, 0.9999f );
		float noDirIlluminance;
		float illuminance = illuminanceSphereOrDisk( cosTheta, sinSigmaSqr, noDirIlluminance );
		
		noDirIlluminance *= smoothFalloff;
		illuminance *= smoothFalloff;

		[branch] if(scatter)
		{
			results.scattering = noDirIlluminance * lightData.Color.rgb * directScattering(gbuffer, mData, materialParams, preparedData.L, ViewVector, lightAmount);
			exec = true;
		}

		[branch]
		if(illuminance > 0)
		{
			exec = true;

			const float diffNoL = saturate(dot(gbuffer.normal, diffL));	
			const float3 diffH = normalize(ViewVector + diffL);
			const float diffVoH = saturate(dot(ViewVector, diffH));

			const float specNoL = saturate( dot(gbuffer.normal, specL) );
			const float3 specH = normalize(ViewVector + specL);
			const float specNoH = saturate( dot(gbuffer.normal, specH) + NOH_EPCILON );
			const float specVoH = saturate( dot(ViewVector, specH) );
	
			results.diffuse = results.specular = illuminance * lightData.Color.rgb;
			results.diffuse *= directDiffuseBRDF(gbuffer.albedo, mData.avgR, mData.NoV, diffNoL, diffVoH);
			results.specular *= specEnergy * directSpecularBRDF(gbuffer.reflectivity, mData.R, specNoH, mData.NoV, specNoL, specVoH, specH, gbuffer.tangent, gbuffer.binormal, mData.avgRSq);
		}
	}
	return exec;
}

bool CalculateTubeLight(in PointLightBuffer lightData, in LightPrepared preparedData, in GBufferData gbuffer, in DataForLightCompute mData, 
						in MaterialParams materialParams, float3 ViewVector, float lightAmount, bool scatter, out LightComponents results)
{	
	results = (LightComponents)0;
	bool exec = false;
	
	const float sqrDist = dot( preparedData.unnormL, preparedData.unnormL );
			
	const float smoothFalloff = smoothDistanceAtt(sqrDist, lightData.PosRange.w);
	[branch]
	if(smoothFalloff > 0)
	{
		// specular
		const float LineAngle = saturate( lightData.AreaInfo.y / max(sqrt( sqrDist ), lightData.AreaInfo.x) );
		float specEnergy = mData.aGGX / saturate( mData.aGGX + 0.5 * LineAngle );
				
		// Closest point on line segment to ray
		const float3 Ld = lightData.DirAreaA.xyz * lightData.AreaInfo.y;
		const float3 halfLd = 0.5 * Ld;
		const float3 L0 = preparedData.unnormL - halfLd;

		// Shortest distance
		const float b = dot( mData.reflect, Ld );
		const float t = saturate( dot( L0, b * mData.reflect - Ld ) / (lightData.AreaInfo.w - b * b) );
		float3 specL = L0 + t * Ld;
		
		// sphere
		const float SphereAngle = clamp( lightData.AreaInfo.x / max(sqrt( dot( specL, specL ) ), lightData.AreaInfo.x), 0, 0.5 );
		specEnergy *= Square( mData.aGGX / saturate( mData.aGGX + SphereAngle ) );
		
		const float3 centerToRay = dot(specL, mData.reflect) * mData.reflect - specL;
		specL = normalize(specL + centerToRay * saturate(lightData.AreaInfo.x / sqrt(dot(centerToRay, centerToRay))));
			
		// diffuse
		const float3 diffL = normalize( preparedData.unnormL + gbuffer.normal * lightData.DirAreaA.w * (1 - saturate(dot(gbuffer.normal, preparedData.L))) );
		
		float noDirIlluminance;
		float illuminance = illuminanceTube( lightData.PosRange.xyz, gbuffer.wpos, gbuffer.normal, lightData.AreaInfo.x, lightData.AreaInfo.z, preparedData.L, L0, Ld, lightData.AreaInfo.w, noDirIlluminance );

		noDirIlluminance *= smoothFalloff;
		illuminance *= smoothFalloff;

		[branch] if(scatter)
		{
			results.scattering = noDirIlluminance * lightData.Color.rgb * directScattering(gbuffer, mData, materialParams, preparedData.L, ViewVector, lightAmount);
			exec = true;
		}

		[branch]
		if(illuminance > 0)
		{
			exec = true;

			const float diffNoL = saturate(dot(gbuffer.normal, diffL));	
			const float3 diffH = normalize(ViewVector + diffL);
			const float diffVoH = saturate(dot(ViewVector, diffH));

			const float specNoL = saturate( dot(gbuffer.normal, specL) );
			const float3 specH = normalize(ViewVector + specL);
			const float specNoH = saturate( dot(gbuffer.normal, specH) + NOH_EPCILON );
			const float specVoH = saturate( dot(ViewVector, specH) );
	
			results.diffuse = results.specular = illuminance * lightData.Color.rgb;
			results.diffuse *= directDiffuseBRDF(gbuffer.albedo, mData.avgR, mData.NoV, diffNoL, diffVoH);
			results.specular *= specEnergy * directSpecularBRDF(gbuffer.reflectivity, mData.R, specNoH, mData.NoV, specNoL, specVoH, specH, gbuffer.tangent, gbuffer.binormal, mData.avgRSq);
		}
	}
	return exec;
}

bool CalculateDirLight(sampler samp, Texture2DArray <float> shadowmap, in DirLightBuffer lightData, in GBufferData gbuffer, in DataForLightCompute mData, 
						in MaterialParams materialParams, float3 ViewVector, float3 depthFix, bool scatter, out LightComponents results)
{	
	results = (LightComponents)0;
	
	const float3 L = -lightData.DirAreaY.xyz;
		
	const float LoR = saturate(dot(L, mData.reflect));
	const float3 projR = mData.reflect - LoR * L;
	const float3 specL = LoR < lightData.DirAreaY.w ? normalize(lightData.DirAreaY.w * L + normalize(projR) * lightData.ColorAreaX.w) : mData.reflect;
			
	const float3 H = normalize(ViewVector + L);
	const float VoH = saturate(dot(ViewVector, H));
	const float NoL = saturate(dot(gbuffer.normal, L));
		
	const float3 specH = normalize(ViewVector + specL);
	const float specNoH = saturate( dot(gbuffer.normal, specH) + NOH_EPCILON );
	const float specVoH = saturate( dot(ViewVector, specH) );
	const float specNoL = saturate( dot(gbuffer.normal, specL) );

	float3 colorLight = lightData.ColorAreaX.rgb;

#if DEBUG_CASCADE_LIGHTS == 0

	float2 lightAmount = DirlightShadow(samp, shadowmap, lightData, L, gbuffer, depthFix, scatter);
	[branch] if( lightAmount.x > 0 || lightAmount.y > 0 )
	{
		[branch] if(scatter)
			results.scattering = colorLight * directScattering(gbuffer, mData, materialParams, L, ViewVector, lightAmount.y);
	
		[branch]
		if(specNoL > 0.0f)
		{
			[branch]
			if(lightAmount.x > 0)
			{
				colorLight *= lightAmount.x;

				results.diffuse = colorLight * NoL * directDiffuseBRDF(gbuffer.albedo, mData.avgR, mData.NoV, NoL, VoH);
				results.specular = colorLight * specNoL * directSpecularBRDF(gbuffer.reflectivity, mData.R, specNoH, mData.NoV, specNoL, specVoH, specH, gbuffer.tangent, gbuffer.binormal, mData.avgRSq);
			}
		}
	}

#else
	results.diffuse = DirlightShadow(samp, shadowmap, lightData, L, gbuffer, depthFix);
#endif
	
	return true;
}

// LIGHT CALCULATION
#ifdef FULL_LIGHT
LightComponents ProcessLights(sampler samp, Texture2DArray <float> shadowmap, in GBufferData gbuffer, in DataForLightCompute mData, 
						in MaterialParams materialParams, float3 ViewVector, float linDepth)
{
	// SHADOW DEPTH FIX // TODO: remove?
#define NORMAL_OFFSET_MAX 10
	float3 shadowDepthFix;
	shadowDepthFix.x = clamp(0.15 * linDepth, 1, 1 + NORMAL_OFFSET_MAX);
	shadowDepthFix.y = saturate((shadowDepthFix.x - 1) * 1.0 / NORMAL_OFFSET_MAX);
	shadowDepthFix.x = 1;//pow(shadowDepthFix.x, 0.75);
	shadowDepthFix.z = max(0.5 * linDepth, 2);
	float dirDepthFix = max(0.1 * linDepth, 1);
	
	bool scatter = materialParams.ior > 0;

	const float lightAmountFake = exp(-gbuffer.thickness);

	LightComponents directLight = (LightComponents)0;

#ifndef TEMP_FAST_COMPILE 
	[loop] // spot
	for(int i_spt=0; i_spt < g_lightCount.spot_count; i_spt++)
	{
		SpotLightBuffer lightData = g_spotLightBuffer[ g_lightIDs[SPOT_L_ID(i_spt)] ];
		LightPrepared prepared = PrepareSpotLight(lightData, gbuffer);

		LightComponents lightResult;
		bool exec = false;

		if (int(lightData.Type.x) == LIGHT_TYPE_SPOT)
			exec = CalculateSpotLight(lightData, prepared, gbuffer, mData, materialParams, ViewVector, lightAmountFake, scatter, lightResult);
		else if (int(lightData.Type.x) == LIGHT_TYPE_DISK)
			exec = CalculateDiskLight(lightData, prepared, gbuffer, mData, materialParams, ViewVector, lightAmountFake, scatter, lightResult);
		else
			exec = CalculateRectLight(lightData, prepared, gbuffer, mData, materialParams, ViewVector, lightAmountFake, scatter, lightResult);

		if(exec)
			directLight.Append(lightResult);
	}
#endif// TEMP_FAST_COMPILE 
	[loop] // caster spot
	for(int ic_spt=0; ic_spt < g_lightCount.caster_spot_count; ic_spt++)
	{
		SpotCasterBuffer lightData = g_spotCasterBuffer[ g_lightIDs[SPOT_C_ID(ic_spt)] ];
		SpotLightBuffer lightDataShort = (SpotLightBuffer)0;
		lightDataShort.Construct(lightData);

		LightPrepared prepared = PrepareSpotLight(lightDataShort, gbuffer);

		float2 lightAmount = SpotlightShadow(samp, shadowmap, prepared, lightData, gbuffer, shadowDepthFix, scatter);
		[branch] if( lightAmount.x == 0 && lightAmount.y == 0 )
			continue;

		LightComponents lightResult;
		bool exec = false;

		if (int(lightDataShort.Type.x) == LIGHT_TYPE_SPOT)
			exec = CalculateSpotLight(lightDataShort, prepared, gbuffer, mData, materialParams, ViewVector, lightAmount.y, scatter, lightResult);
		else if (int(lightDataShort.Type.x) == LIGHT_TYPE_DISK)
			exec = CalculateDiskLight(lightDataShort, prepared, gbuffer, mData, materialParams, ViewVector, lightAmount.y, scatter, lightResult);
		else
			exec = CalculateRectLight(lightDataShort, prepared, gbuffer, mData, materialParams, ViewVector, lightAmount.y, scatter, lightResult);

		if (exec)
			directLight.AppendShadowed(lightResult, lightAmount.x);
	}
#ifndef TEMP_FAST_COMPILE 
	
	[loop] // point
	for(int i_p=0; i_p < g_lightCount.point_count; i_p++)
	{
		PointLightBuffer lightData = g_pointLightBuffer[ g_lightIDs[POINT_L_ID(i_p)] ];
		LightPrepared prepared = PreparePointLight(lightData, gbuffer);

		LightComponents lightResult;
		bool exec = false;

		if (int(lightData.Type.x) == LIGHT_TYPE_POINT)
			exec = CalculatePointLight(lightData, prepared, gbuffer, mData, materialParams, ViewVector, lightAmountFake, scatter, lightResult);
		else if (int(lightData.Type.x) == LIGHT_TYPE_SPHERE)
			exec = CalculateSphereLight(lightData, prepared, gbuffer, mData, materialParams, ViewVector, lightAmountFake, scatter, lightResult);
		else
			exec = CalculateTubeLight(lightData, prepared, gbuffer, mData, materialParams, ViewVector, lightAmountFake, scatter, lightResult);

		if(exec)
			directLight.Append(lightResult);
	}

	[loop] // caster point
	for(int ic_p=0; ic_p < g_lightCount.caster_point_count; ic_p++)
	{
		PointCasterBuffer lightData = g_pointCasterBuffer[ g_lightIDs[POINT_C_ID(ic_p)] ];
		PointLightBuffer lightDataShort = (PointLightBuffer)0;
		lightDataShort.Construct(lightData);

		LightPrepared prepared = PreparePointLight(lightDataShort, gbuffer);
		
		float2 lightAmount = PointlightShadow(samp, shadowmap, prepared, lightData, gbuffer, shadowDepthFix, scatter);
		[branch] if( lightAmount.x == 0 && lightAmount.y == 0 )
			continue;

		LightComponents lightResult;
		bool exec = false;

		if (int(lightDataShort.Type.x) == LIGHT_TYPE_POINT)
			exec = CalculatePointLight(lightDataShort, prepared, gbuffer, mData, materialParams, ViewVector, lightAmount.y, scatter, lightResult);
		else if (int(lightDataShort.Type.x) == LIGHT_TYPE_SPHERE)
			exec = CalculateSphereLight(lightDataShort, prepared, gbuffer, mData, materialParams, ViewVector, lightAmount.y, scatter, lightResult);
		else
			exec = CalculateTubeLight(lightDataShort, prepared, gbuffer, mData, materialParams, ViewVector, lightAmount.y, scatter, lightResult);

		if(exec)		
			directLight.AppendShadowed(lightResult, lightAmount.x);
	}
	
	[loop] // dir
	for(int i_dir=0; i_dir < g_lightCount.dir_count; i_dir++)
	{
		DirLightBuffer lightData = g_dirLightBuffer[ g_lightIDs[DIR_ID(i_dir)] ];
		
		LightComponents lightResult;
		[branch] if( !CalculateDirLight( samp, shadowmap, lightData, gbuffer, mData, materialParams, ViewVector, shadowDepthFix, scatter, lightResult ) )
			continue;
		
		directLight.Append(lightResult);
	}
#endif// TEMP_FAST_COMPILE 
	return directLight;
}
#endif

float3 ProcessLightsVoxel(sampler samp, Texture2DArray <float> shadowsAtlas, float shadowBias, 
						float3 albedo, float3 normal, float3 emissive, float3 wpos,
						StructuredBuffer<SpotVoxelBuffer> spotLightInjectBuffer, int spotCount, 
						StructuredBuffer<PointVoxelBuffer> pointLightInjectBuffer, int pointCount,
						StructuredBuffer<DirVoxelBuffer> dirLightInjectBuffer, int dirCount)
{
	const float4 shadowWpos = float4(wpos + normal * shadowBias, 1.0f);
	
	float3 emittance = 0;

	[loop]
	for(int spotID = 0; spotID < spotCount; spotID++)
	{
		const SpotVoxelBuffer spotLightData = spotLightInjectBuffer[spotID];
	
		const float3 unnormL = spotLightData.PosRange.xyz - wpos;
		
		const float DoUL = dot(spotLightData.DirConeY.xyz, -unnormL);
		if(DoUL <= 0)
			continue;
		 
		float illuminance = getDistanceAtt( unnormL, spotLightData.PosRange.w );
		if(illuminance <= 0) 
			continue;
		
		const float3 L = normalize(unnormL);
			 
		const float NoL = saturate(dot(normal, L));
		if(NoL == 0.0f)
			continue;

		const float3 virtUnnormL = spotLightData.Virtpos.xyz - wpos;
		const float3 virtL = normalize(virtUnnormL);

		illuminance *= getAngleAtt(virtL, -spotLightData.DirConeY.xyz, spotLightData.ColorConeX.w, spotLightData.DirConeY.w);
		if(illuminance <= 0)
			continue; 

		const float light_blocked = GetVoxelSpotShadow(samp, shadowsAtlas, shadowWpos, spotLightData);
		const float3 colorIlluminance = light_blocked * illuminance * spotLightData.ColorConeX.rgb;
		
		emittance += colorIlluminance * NoL;
	}

	[loop]
	for(int pointID=0; pointID < pointCount; pointID++)
	{
		const PointVoxelBuffer pointLightData = pointLightInjectBuffer[pointID];
	
		const float3 unnormL = pointLightData.PosRange.xyz - wpos;
		
		const float illuminance = getDistanceAtt( unnormL, pointLightData.PosRange.w );
		if(illuminance == 0)
			continue;
		
		const float3 L = normalize(unnormL);
		const float NoL = saturate(dot(normal, L));
			
		if(NoL == 0.0f)
			continue;
		
		const float light_blocked = GetVoxelPointShadow(samp, shadowsAtlas, shadowWpos, pointLightData);
		const float3 colorIlluminance = light_blocked * illuminance * pointLightData.ColorShadowmapProj.rgb;
			
		emittance += colorIlluminance * NoL;
	}

	[loop]
	for(int dirID = 0; dirID < dirCount; dirID++)
	{
		const DirVoxelBuffer dirLightData = dirLightInjectBuffer[dirID];
		
		const float3 L = -dirLightData.Dir.xyz;
		
		const float NoL = saturate(dot(normal, L));
		if(NoL == 0.0f)
			continue;

		const float light_blocked = GetVoxelDirShadow(samp, shadowsAtlas, shadowWpos, dirLightData);
		const float3 colorIlluminance = light_blocked * dirLightData.Color.rgb;
		
		emittance += colorIlluminance * NoL;
	}

	emittance = emittance * Diffuse_Lambert(albedo) + emissive;

	return emittance;
}