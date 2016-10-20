
// Gather filter
float GatherFilter(float3 UV, float2 reprojUV, float halfPix, float depth)  
{  
	float4 shadMapDepth = shadows.GatherRed(samplerPointClamp, UV);
	float4 compare = float4(shadMapDepth.x < depth,
							shadMapDepth.y < depth,
							shadMapDepth.z < depth,
							shadMapDepth.w < depth);

	if(!any(compare))
		return 1;
	
	float2 fracUV = abs( frac(reprojUV / (halfPix * 0.5) + float2(0.002, 0.002)) );
	fracUV.x = fracUV.x > 0.5 ? 1.5 - fracUV.x : 0.5 - fracUV.x;
	fracUV.y = fracUV.y > 0.5 ? 1.5 - fracUV.y : 0.5 - fracUV.y;

	float l1 = lerp(compare.y, compare.x, fracUV.x);
	float l2 = lerp(compare.z, compare.w, fracUV.x);
	return 1 - lerp(l1, l2, fracUV.y); 
}

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

static const float2 reproj[2] =
{
	float2(0.5f,-0.5f),
	float2(0.5f,0.5f)
};

float SpotlightShadow(float4 wpos, float zInLight, float3 normal, float NoL, matrix vp_mat, float4 adress, float2 texelSize, float3 depthFix)
{
	float4 lightViewProjPos = mul(wpos, vp_mat);
		
	float normalOffsetScale = clamp(1 - NoL, depthFix.y, 1);
	
	float shadowMapTexelSize = texelSize.x * zInLight * texelSize.y;
		
	float4 shadowOffset = float4(normal * normalShadowOffsetSpot * normalOffsetScale * shadowMapTexelSize * depthFix.x, 0);
		
	float4 correctedWpos = wpos + shadowOffset;
	float4 uvOffset = mul(correctedWpos, vp_mat);
	lightViewProjPos.xy = uvOffset.xy;
		
	float lvp_rcp = rcp(lightViewProjPos.w);
	float2 reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
		
	if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
		return 0;
			
	float3 shadowmapCoords;
	shadowmapCoords.xy = adress.xy + reprojCoords.xy * adress.z;
	shadowmapCoords.z = adress.w;
	
	//float shadowDepth = shadows.SampleLevel(samplerClamp, shadowmapCoords, 0).r;
	const float resBiasScale = max(2, (texelSize.x * SHADOWS_RES_BIAS_SCALE) * 0.2);
	lightViewProjPos.z -= shadowBiasSpot * min(10, depthFix.z * resBiasScale);
	
	float depthView = lightViewProjPos.z * lvp_rcp;
	
	return GatherFilter(shadowmapCoords, reprojCoords.xy, texelSize.x, depthView);
}

float AreaSpotlightShadow(float4 wpos, float zInLight, float3 normal, float NoL, matrix vp_mat, float4 adress, float2 texelSize, float near, float3 depthFix)
{
	float4 lightViewProjPos = mul(wpos, vp_mat);
		
	float normalOffsetScale = clamp(1 - NoL, depthFix.y, 1);
	
	float shadowMapTexelSize = texelSize.x * zInLight * texelSize.y;
		
	float4 shadowOffset = float4(normal * normalShadowOffsetSpot * normalOffsetScale * shadowMapTexelSize * depthFix.x, 0);
		
	float4 correctedWpos = wpos + shadowOffset;
	float4 uvOffset = mul(correctedWpos, vp_mat);
	lightViewProjPos.xy = uvOffset.xy;
		
	float lvp_rcp = rcp(lightViewProjPos.w);
	float2 reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
		
	if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
		return false;
			
	float3 shadowmapCoords;
	shadowmapCoords.xy = adress.xy + reprojCoords.xy * adress.z;
	shadowmapCoords.z = adress.w;
	
	lightViewProjPos.z -= shadowBiasSpotArea * near * depthFix.z;
	float depthView = lightViewProjPos.z * lvp_rcp;

	return GatherFilter(shadowmapCoords, reprojCoords.xy, texelSize.x, depthView);
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

float PointlightShadow(float3 wpos, float3 posInLight, float3 pos, float3 normal, float NoL, matrix proj_mat, 
	float4 adress0, float4 adress1, float4 adress2, float4 adress3, float4 adress4, float4 adress5, float texelProj,
	float4 texelSize0, float2 texelSize1, float3 depthFix)
{
	const float zInLSq[3] = {posInLight.x, posInLight.z, posInLight.y};
	float zInLSqAbs[3];
	[unroll]for(uint q=0; q<3; q++) zInLSqAbs[q] = abs(zInLSq[q]);
	
	const float normalOffsetScale = clamp(1 - NoL, depthFix.y, 1);
	float3 shadowmapCoords = 0;
	float2 reprojCoords = 0;
	float halfPix = 0;
	float depthView = 0;
	
	[branch]
	if(zInLSqAbs[0]>=zInLSqAbs[1] && zInLSqAbs[0]>=zInLSqAbs[2])
	{
		[branch]
		if(zInLSq[0]>0)
		{
			const float4 pil = float4(-posInLight.z,posInLight.y,posInLight.x,1);
			
			const float shadowMapTexelSize = texelProj * texelSize0.r * pil.z;
			halfPix = texelSize0.r;

			float3 shadowOffset = normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			float3 corWpos = (wpos + shadowOffset) - pos;
			
			float4 uvOffset = mul(float4(-corWpos.z,corWpos.y,corWpos.x, 1), proj_mat);
			float4 lightViewProjPos = mul(pil, proj_mat);
			lightViewProjPos.xy = uvOffset.xy;
				
			const float lvp_rcp = rcp(lightViewProjPos.w);
			reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
			
			// REMOVE
			if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
				return false;
					
			shadowmapCoords.xy = adress0.xy + reprojCoords.xy * adress0.z;
			shadowmapCoords.z = adress0.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
		else
		{
			const float4 pil = float4(posInLight.z,posInLight.y,-posInLight.x,1);
			
			const float shadowMapTexelSize = texelProj * texelSize0.g * pil.z;
			halfPix = texelSize0.g;
			
			float3 shadowOffset = normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			float3 corWpos = (wpos + shadowOffset) - pos;
			
			float4 uvOffset = mul(float4(corWpos.z,corWpos.y,-corWpos.x, 1), proj_mat);
			float4 lightViewProjPos = mul(pil, proj_mat);
			lightViewProjPos.xy = uvOffset.xy;
				
			const float lvp_rcp = rcp(lightViewProjPos.w);
			reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
			
			// REMOVE
			if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
				return false;
					
			shadowmapCoords.xy = adress1.xy + reprojCoords.xy * adress1.z;
			shadowmapCoords.z = adress1.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
	}
	else if(zInLSqAbs[1]>=zInLSqAbs[2])
	{
		[branch]
		if(zInLSq[1]>0)
		{
			const float4 pil = float4(posInLight,1);
			
			const float shadowMapTexelSize = texelProj * texelSize0.b * pil.z;
			halfPix = texelSize0.b;
			
			float3 shadowOffset = normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			float3 corWpos = (wpos + shadowOffset) - pos;
			
			float4 uvOffset = mul(float4(corWpos, 1), proj_mat);
			float4 lightViewProjPos = mul(pil, proj_mat);
			lightViewProjPos.xy = uvOffset.xy;
				
			const float lvp_rcp = rcp(lightViewProjPos.w);
			reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
			
			// REMOVE
			if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
				return false;
			
			shadowmapCoords.xy = adress2.xy + reprojCoords.xy * adress2.z;
			shadowmapCoords.z = adress2.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
		else
		{
			const float4 pil = float4(-posInLight.x,posInLight.y,-posInLight.z,1);
			
			const float shadowMapTexelSize = texelProj * texelSize0.a * pil.z;
			halfPix = texelSize0.a;
			
			float3 shadowOffset = normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			float3 corWpos = (wpos + shadowOffset) - pos;
			
			float4 uvOffset = mul(float4(-corWpos.x,corWpos.y,-corWpos.z, 1), proj_mat);
			float4 lightViewProjPos = mul(pil, proj_mat);
			lightViewProjPos.xy = uvOffset.xy;
				
			const float lvp_rcp = rcp(lightViewProjPos.w);
			reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
			
			// REMOVE
			if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
				return false;
			
			shadowmapCoords.xy = adress3.xy + reprojCoords.xy * adress3.z;
			shadowmapCoords.z = adress3.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
	}
	else
	{
		[branch]
		if(zInLSq[2]>0)
		{
			const float4 pil = float4(posInLight.z,posInLight.x,posInLight.y,1);
			
			const float shadowMapTexelSize = texelProj * texelSize1.r * pil.z;
			halfPix = texelSize1.r;
			
			float3 shadowOffset = normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			float3 corWpos = (wpos + shadowOffset) - pos;
			
			float4 uvOffset = mul(float4(corWpos.z,corWpos.x,corWpos.y, 1), proj_mat);
			float4 lightViewProjPos = mul(pil, proj_mat);
			lightViewProjPos.xy = uvOffset.xy;
				
			const float lvp_rcp = rcp(lightViewProjPos.w);
			reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
			
			// REMOVE
			if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
				return false;
					
			shadowmapCoords.xy = adress4.xy + reprojCoords.xy * adress4.z;
			shadowmapCoords.z = adress4.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
		else
		{
			const float4 pil = float4(posInLight.z,-posInLight.x,-posInLight.y,1);
			
			const float shadowMapTexelSize = texelProj * texelSize1.g * pil.z;
			halfPix = texelSize1.g;
			
			float3 shadowOffset = normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			float3 corWpos = (wpos + shadowOffset) - pos;
			
			float4 uvOffset = mul(float4(corWpos.z,-corWpos.x,-corWpos.y, 1), proj_mat);
			float4 lightViewProjPos = mul(pil, proj_mat);
			lightViewProjPos.xy = uvOffset.xy;
				
			const float lvp_rcp = rcp(lightViewProjPos.w);
			reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
			
			// REMOVE
			if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
				return false;
					
			shadowmapCoords.xy = adress5.xy + reprojCoords.xy * adress5.z;
			shadowmapCoords.z = adress5.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
	}

	return GatherFilter(shadowmapCoords, reprojCoords.xy, halfPix, depthView);
}

float TubelightShadow(float3 wpos, float3 normal, float NoL, matrix proj_mat, matrix view_mat, 
	float4 adress0, float4 adress1, float4 adress2, float4 adress3, float4 adress4, float4 adress5, 
	float texelProj, float4 texelSize0, float2 texelSize1, float3 depthFix)
{
	const float4 posInLight = mul(float4(wpos,1), view_mat);

	const float zInLSq[3] = {posInLight.z, posInLight.x, posInLight.y};
	float zInLSqAbs[3];
	[unroll]for(uint q=0; q<3; q++) zInLSqAbs[q] = abs(zInLSq[q]);
	
	const float normalOffsetScale = clamp(1 - NoL, depthFix.y, 1);
	float3 shadowmapCoords = 0;
	float2 reprojCoords = 0;
	float halfPix = 0;
	float depthView = 0;
	
	[branch]
	if(zInLSqAbs[0]>=zInLSqAbs[1] && zInLSqAbs[0]>=zInLSqAbs[2])
	{
		[branch]
		if(zInLSq[0]>0)
		{
			const float4 pil = float4(posInLight.x,posInLight.y,posInLight.z,1);
			
			const float shadowMapTexelSize = texelProj * texelSize0.r * pil.z;
			halfPix = texelSize0.r;
			
			float3 shadowOffset = normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			float4 corWpos = mul(float4(wpos + shadowOffset, 1), view_mat);
			
			float4 uvOffset = mul(float4(corWpos.x,corWpos.y,corWpos.z, 1), proj_mat);
			float4 lightViewProjPos = mul(pil, proj_mat);
			lightViewProjPos.xy = uvOffset.xy;
				
			const float lvp_rcp = rcp(lightViewProjPos.w);
			reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
			
			// REMOVE
			if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
				return false;
					
			shadowmapCoords.xy = adress0.xy + reprojCoords.xy * adress0.z;
			shadowmapCoords.z = adress0.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
		else
		{
			const float4 pil = float4(-posInLight.x,posInLight.y,-posInLight.z,1);
			
			const float shadowMapTexelSize = texelProj * texelSize0.g * pil.z;
			halfPix = texelSize0.g;
			
			float3 shadowOffset = normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			float4 corWpos = mul(float4(wpos + shadowOffset, 1), view_mat);
			
			float4 uvOffset = mul(float4(-corWpos.x,corWpos.y,-corWpos.z, 1), proj_mat);
			float4 lightViewProjPos = mul(pil, proj_mat);
			lightViewProjPos.xy = uvOffset.xy;
				
			const float lvp_rcp = rcp(lightViewProjPos.w);
			reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
			
			// REMOVE
			if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
				return false;
					
			shadowmapCoords.xy = adress1.xy + reprojCoords.xy * adress1.z;
			shadowmapCoords.z = adress1.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
	}
	else if(zInLSqAbs[1]>=zInLSqAbs[2])
	{
		[branch]
		if(zInLSq[1]>0)
		{
			const float4 pil = float4(-posInLight.z,posInLight.y,posInLight.x,1);
			
			const float shadowMapTexelSize = texelProj * texelSize0.b * pil.z;
			halfPix = texelSize0.b;
			
			float3 shadowOffset = normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			float4 corWpos = mul(float4(wpos + shadowOffset, 1), view_mat);
			
			float4 uvOffset = mul(float4(-corWpos.z,corWpos.y,corWpos.x, 1), proj_mat);
			float4 lightViewProjPos = mul(pil, proj_mat);
			lightViewProjPos.xy = uvOffset.xy;
				
			const float lvp_rcp = rcp(lightViewProjPos.w);
			reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
			
			// REMOVE
			if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
				return false;
			
			shadowmapCoords.xy = adress2.xy + reprojCoords.xy * adress2.z;
			shadowmapCoords.z = adress2.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
		else
		{
			const float4 pil = float4(posInLight.z,posInLight.y,-posInLight.x,1);
			
			const float shadowMapTexelSize = texelProj * texelSize0.a * pil.z;
			halfPix = texelSize0.a;
			
			float3 shadowOffset = normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			float4 corWpos = mul(float4(wpos + shadowOffset, 1), view_mat);
			
			float4 uvOffset = mul(float4(corWpos.z,corWpos.y,-corWpos.x, 1), proj_mat);
			float4 lightViewProjPos = mul(pil, proj_mat);
			lightViewProjPos.xy = uvOffset.xy;
				
			const float lvp_rcp = rcp(lightViewProjPos.w);
			reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
			
			// REMOVE
			if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
				return false;
			
			shadowmapCoords.xy = adress3.xy + reprojCoords.xy * adress3.z;
			shadowmapCoords.z = adress3.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
	}
	else
	{
		[branch]
		if(zInLSq[2]>0)
		{
			const float4 pil = float4(-posInLight.x,posInLight.z,posInLight.y,1);
			
			const float shadowMapTexelSize = texelProj * texelSize1.r * pil.z;
			halfPix = texelSize1.r;
			
			float3 shadowOffset = normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			float4 corWpos = mul(float4(wpos + shadowOffset, 1), view_mat);
			
			float4 uvOffset = mul(float4(-corWpos.x,corWpos.z,corWpos.y, 1), proj_mat);
			float4 lightViewProjPos = mul(pil, proj_mat);
			lightViewProjPos.xy = uvOffset.xy;
				
			const float lvp_rcp = rcp(lightViewProjPos.w);
			reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
			
			// REMOVE
			if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
				return false;
					
			shadowmapCoords.xy = adress4.xy + reprojCoords.xy * adress4.z;
			shadowmapCoords.z = adress4.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
		else
		{
			const float4 pil = float4(-posInLight.x,-posInLight.z,-posInLight.y,1);
			
			const float shadowMapTexelSize = texelProj * texelSize1.g * pil.z;
			halfPix = texelSize1.g;
			
			float3 shadowOffset = normal * normalShadowOffsetPoint * normalOffsetScale * shadowMapTexelSize * depthFix.x;
			float4 corWpos = mul(float4(wpos + shadowOffset, 1), view_mat);
			
			float4 uvOffset = mul(float4(-corWpos.x,-corWpos.z,-corWpos.y, 1), proj_mat);
			float4 lightViewProjPos = mul(pil, proj_mat);
			lightViewProjPos.xy = uvOffset.xy;
				
			const float lvp_rcp = rcp(lightViewProjPos.w);
			reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
			
			// REMOVE
			if(reprojCoords.x < 0 || reprojCoords.x > 1 || reprojCoords.y < 0 || reprojCoords.y > 1 || lightViewProjPos.z < 0)
				return false;
					
			shadowmapCoords.xy = adress5.xy + reprojCoords.xy * adress5.z;
			shadowmapCoords.z = adress5.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
	}
	
	return GatherFilter(shadowmapCoords, reprojCoords.xy, halfPix, depthView);
}

float DirlightShadow(float3 wpos, float3 dir, float3 pos0, float3 pos1, float3 pos2, float3 pos3, float3 normal, float NoL, 
	matrix vp0, matrix vp1, matrix vp2, matrix vp3, float4 adress0, float4 adress1, float4 adress2, float4 adress3, float3 depthFix, float4 texelSizes)
{
	// temp
#if DEBUG_CASCADE_LIGHTS != 0
	float res = 0;
#endif

	float4 wpos4 = float4(wpos, 1.0);

	matrix viewproj = 0;
	float3 LPos = 0;
	float4 adress = 0;
	float halfPix = 0;

	float4 lightViewProjPos = mul(wpos4, vp0);
	float normalShadowOffsetDir = normalShadowOffsetDir0;
	//float shadowBiasDir = shadowBiasDir0;
	
	if(abs(lightViewProjPos.x) < 1 && abs(lightViewProjPos.y) < 1 && lightViewProjPos.z < 1) // 0 cascede
	{
		viewproj = vp0;
		LPos = pos0;
		adress = adress0;
#if DEBUG_CASCADE_LIGHTS != 0
		res = 1.0;
#endif
		normalShadowOffsetDir = normalShadowOffsetDir0;
		//shadowBiasDir = shadowBiasDir0;
		halfPix = texelSizes.x;
	}
	else
	{
		lightViewProjPos = mul(wpos4, vp1);
		if(abs(lightViewProjPos.x) < 1 && abs(lightViewProjPos.y) < 1 && lightViewProjPos.z < 1) // 1 cascede
		{
			viewproj = vp1;
			LPos = pos1;
			adress = adress1;
#if DEBUG_CASCADE_LIGHTS != 0
			res = 0.75;
#endif
			normalShadowOffsetDir = normalShadowOffsetDir1;
			//shadowBiasDir = shadowBiasDir1;
			halfPix = texelSizes.y;
		}
		else
		{
			lightViewProjPos = mul(wpos4, vp2);
			if(abs(lightViewProjPos.x) < 1 && abs(lightViewProjPos.y) < 1 && lightViewProjPos.z < 1) // 2 cascede
			{
				viewproj = vp2;
				LPos = pos2;
				adress = adress2;
#if DEBUG_CASCADE_LIGHTS != 0
				res = 0.5;
#endif
				normalShadowOffsetDir = normalShadowOffsetDir2;
				//shadowBiasDir = shadowBiasDir2;
				halfPix = texelSizes.z;
			}
			else // 3 cascade
			{
				lightViewProjPos = mul(wpos4, vp3);
				
				viewproj = vp3;
				LPos = pos3;
				adress = adress3;
#if DEBUG_CASCADE_LIGHTS != 0
				res = 0.25;
#endif
				normalShadowOffsetDir = normalShadowOffsetDir3;
				//shadowBiasDir = shadowBiasDir3;
				halfPix = texelSizes.w;
			}
		}
	}

	float normalOffsetScale = clamp(1 - NoL, depthFix.y, 1);
		
	float4 shadowOffset = float4(normal * normalShadowOffsetDir * normalOffsetScale * depthFix.x, 0);
		
	float4 correctedWpos = wpos4 + shadowOffset;
	float4 uvOffset = mul(correctedWpos, viewproj);
	lightViewProjPos.xy = uvOffset.xy;
		
	float lvp_rcp = rcp(lightViewProjPos.w);
	float2 reprojCoords = reproj[0] * lightViewProjPos.xy * lvp_rcp + reproj[1];
			
	float3 shadowmapCoords;
	shadowmapCoords.xy = adress.xy + reprojCoords.xy * adress.z;
	shadowmapCoords.z = adress.w;
	
	lightViewProjPos.z -= shadowBiasDir0 * depthFix.z;
	float depthView = lightViewProjPos.z * lvp_rcp;

#if DEBUG_CASCADE_LIGHTS != 0
	return res * GatherFilter(shadowmapCoords, reprojCoords.xy, halfPix, depthView);
#else
	return GatherFilter(shadowmapCoords, reprojCoords.xy, halfPix, depthView);
#endif
}

/*
bool SpotlightShadow(out float shadowmap, uint lnum, float3 WP, matrix m_view, float dist, uint FilterType, float bias, float lbr, float samples, float light_size, bool noise, float2 noise_coord, float pix, float2 border)
{
	shadowmap = 0;
	bool output = true;
	
	float2 projectTexCoord = 0;
	if(!ReprojectCoords(WP, m_view, projectTexCoord, border))
	{
		output = false;
		return output;
	}
	
	float4 t_samp = 0;

	if(FilterType == E_SHADOW_FILTER_NONE)
	{
		if(noise)
		{
			float2 noise_vect = noiseTex.SampleLevel(samplerWarp, noise_coord, 0).rg;
			noise_vect = (noise_vect * 2 - 1) * light_size * pix * 0.5; 
			projectTexCoord += noise_vect;
		}
	
		t_samp = shadows[lnum].SampleLevel(samplerClampFilter, projectTexCoord, 0);
		if(t_samp.r < dist - bias)
			output = false;
		else
			shadowmap = 1;
	}
	else if(FilterType == E_SHADOW_FILTER_SOFTPCF)
	{
		float lightDepthValue = dist - bias;
		shadowmap = 1.0 - RandomBlurfilter(projectTexCoord, lightDepthValue, shadows[lnum], light_size, samples, noise_coord, noise);
	}
	else if(FilterType == E_SHADOW_FILTER_PENUMBRA)
	{
		float lightDepthValue = dist - bias;
		float seach_dist = light_size * lbr / lightDepthValue;// 3/lbr

		float blocker = findBlocker(projectTexCoord, lightDepthValue, shadows[lnum], seach_dist, samples, noise_coord, noise);
		if (blocker < -998) 
		{
			shadowmap = 1;
		}
		else
		{
			float penumbra = estimatePenumbra(lightDepthValue, blocker, light_size);
			penumbra = clamp(penumbra, pix, seach_dist/2);
			shadowmap = 1.0 - RandomBlurfilter(projectTexCoord, lightDepthValue, shadows[lnum], penumbra, samples, noise_coord, noise);
		}
	}

	return output;
}

bool DirlightShadow(out float shadowmap, uint lnum, float3 WP, float3 pos_0, float3 pos_1, float3 pos_2, float3 pos_3, matrix m_view_0, matrix m_view_1, matrix m_view_2, matrix m_view_3, uint FilterType, float bias, float lbr, float samples, float light_size, bool noise, float2 noise_coord, float pix, float2 border)
{
	shadowmap = 0;
	bool output = true;
	float dist = 0;
	float cascade_bias = 0;
	
	float2 projectTexCoord = 0;
	if(ReprojectCoords(WP, m_view_0, projectTexCoord, border))
	{
		projectTexCoord = projectTexCoord * float2(0.5,0.5);
		dist = length(pos_0 - WP);
		cascade_bias = bias;
	}
	else if(ReprojectCoords(WP, m_view_1, projectTexCoord, border))
	{
		projectTexCoord = projectTexCoord * float2(0.5,0.5) + float2(0.5,0);
		dist = length(pos_1 - WP);
		cascade_bias = bias * 4;
	}
	else if(ReprojectCoords(WP, m_view_2, projectTexCoord, border))
	{
		projectTexCoord = projectTexCoord * float2(0.5,0.5) + float2(0,0.5);
		dist = length(pos_2 - WP);
		cascade_bias = bias * 8;
	}
	else if(ReprojectCoords(WP, m_view_3, projectTexCoord, border))
	{
		projectTexCoord = projectTexCoord * float2(0.5,0.5) + float2(0.5,0.5);
		dist = length(pos_3 - WP);
		cascade_bias = bias * 16;
	}
	else
	{
		shadowmap = 1;
		output = true;
	}
	
	if(shadowmap == 0)
	{
		float4 t_samp = 0;

		if(FilterType == E_SHADOW_FILTER_NONE)
		{
			if(noise)
			{
				float2 noise_vect = noiseTex.SampleLevel(samplerWarp, noise_coord, 0).rg;
				noise_vect = (noise_vect * 2 - 1) * light_size * pix * 0.5; 
				projectTexCoord += noise_vect;
			}
		
			t_samp = shadows[lnum].SampleLevel(samplerClampFilter, projectTexCoord, 0);
			if(t_samp.r < dist - cascade_bias)
				output = false;
			else
				shadowmap = 1;
		}
		else if(FilterType == E_SHADOW_FILTER_SOFTPCF)
		{
			float lightDepthValue = dist - cascade_bias;
			shadowmap = 1.0 - RandomBlurfilter(projectTexCoord, lightDepthValue, shadows[lnum], light_size, samples, noise_coord, noise);
		}
		else if(FilterType == E_SHADOW_FILTER_PENUMBRA)
		{
			float lightDepthValue = dist - cascade_bias;
			float seach_dist = light_size * lbr / lightDepthValue;// 3/lbr

			float blocker = findBlocker(projectTexCoord, lightDepthValue, shadows[lnum], seach_dist, samples, noise_coord, noise);
			if (blocker < -998) 
			{
				shadowmap = 1;
			}
			else
			{
				float penumbra = estimatePenumbra(lightDepthValue, blocker, light_size);
				penumbra = clamp(penumbra, pix, seach_dist/2);
				shadowmap = 1.0 - RandomBlurfilter(projectTexCoord, lightDepthValue, shadows[lnum], penumbra, samples, noise_coord, noise);
			}
		}
	}

	return output;
}

bool PointlightShadow(out float shadowmap, uint lnum, float3 WP, float dist, matrix m_view_0, matrix m_view_1, matrix m_view_2, matrix m_view_3, matrix m_view_4, matrix m_view_5, uint FilterType, float bias, float lbr, float samples, float light_size, bool noise, float2 noise_coord, float pix, float2 border)
{
	shadowmap = 0;
	bool output = true;
	
	float2 projectTexCoord = 0;
	if(ReprojectCoords(WP, m_view_0, projectTexCoord, border))
	{
		projectTexCoord = projectTexCoord * float2(0.3333333,0.5);
	}
	else if(ReprojectCoords(WP, m_view_1, projectTexCoord, border))
	{
		projectTexCoord = projectTexCoord * float2(0.3333333,0.5) + float2(0.3333333,0);
	}
	else if(ReprojectCoords(WP, m_view_2, projectTexCoord, border))
	{
		projectTexCoord = projectTexCoord * float2(0.3333333,0.5) + float2(0.6666667,0);
	}
	else if(ReprojectCoords(WP, m_view_3, projectTexCoord, border))
	{
		projectTexCoord = projectTexCoord * float2(0.3333333,0.5) + float2(0,0.5);
	}
	else if(ReprojectCoords(WP, m_view_4, projectTexCoord, border))
	{
		projectTexCoord = projectTexCoord * float2(0.3333333,0.5) + float2(0.3333333,0.5);
	}
	else if(ReprojectCoords(WP, m_view_5, projectTexCoord, border))
	{
		projectTexCoord = projectTexCoord * float2(0.3333333,0.5) + float2(0.6666667,0.5);
	}
	else
	{
		output = false;
	}
	
	if(output)
	{
		float4 t_samp = 0;

		if(FilterType == E_SHADOW_FILTER_NONE)
		{
			if(noise)
			{
				float2 noise_vect = noiseTex.SampleLevel(samplerWarp, noise_coord, 0).rg;
				noise_vect = (noise_vect * 2 - 1) * light_size * pix * 0.5; 
				projectTexCoord += noise_vect;
			}
		
			t_samp = shadows[lnum].SampleLevel(samplerClampFilter, projectTexCoord, 0);
			if(t_samp.r < dist - bias)
				output = false;
			else
				shadowmap = 1;
		}
		else if(FilterType == E_SHADOW_FILTER_SOFTPCF)
		{
			float lightDepthValue = dist - bias;
			shadowmap = 1.0 - RandomBlurfilter(projectTexCoord, lightDepthValue, shadows[lnum], light_size, samples, noise_coord, noise);
		}
		else if(FilterType == E_SHADOW_FILTER_PENUMBRA)
		{
			float lightDepthValue = dist - bias;
			float seach_dist = light_size * lbr / lightDepthValue;// 3/lbr

			float blocker = findBlocker(projectTexCoord, lightDepthValue, shadows[lnum], seach_dist, samples, noise_coord, noise);
			if (blocker < -998) 
			{
				shadowmap = 1;
			}
			else
			{
				float penumbra = estimatePenumbra(lightDepthValue, blocker, light_size);
				penumbra = clamp(penumbra, pix, seach_dist/2);
				shadowmap = 1.0 - RandomBlurfilter(projectTexCoord, lightDepthValue, shadows[lnum], penumbra, samples, noise_coord, noise);
			}
		}
	}

	return output;
}

LightCalcOutput CalcSpotLight(float3 WP, float3 V, float3 N, float3 T, float3 B, float3 S, float2 R, float avgR, float aGGX, float3 A, float3 SS, MaterialParamsStructBuffer params, SpotLightBuffer lData, float NoV, float3 Refl)
{
	LightCalcOutput res;
	res.diffuse = 0;
	res.specular = 0;
	res.nope = true;
	res.distL = 0;
	
	const float3 unnormL = lData.Pos - WP;
	const float3 L = normalize(unnormL);
	
	bool back = false;
	bool nope = false;
	
	const float DdotUL = dot(lData.Dir, -unnormL);
	if(DdotUL <= 0)
		return res;
	
	float illuminance = 0;
	float noDirIlluminance = 0;
	
	float3 specL = unnormL;
	float3 specH = 0;
	float specNoH = 0;
	float specVoH = 0;
	float specNoL = 0;
	float specEnergy = 1;
	
	float3 diffL = unnormL;
	float3 diffH = 0;
	float diffVoH = 0;
	float diffNoL = 0;
	
	if(lData.AreaInfo.x > 0)
	{
		const float sqrDist = dot( unnormL, unnormL );
		float smoothFalloff = smoothDistanceAtt(sqrDist, lData.Range);
		if(smoothFalloff == 0)nope = true;
		
		float coneFalloff = getAngleAtt(normalize(lData.VirtualPos - WP), -lData.Dir, lData.Cone.r, lData.Cone.g);
		if(coneFalloff == 0)nope = true;
			
		coneFalloff *= smoothFalloff;
	
		if(!nope)
		{
			res.distL = sqrt( sqrDist );
			const float RLengthL = rcp( max(res.distL, lData.AreaInfo.x) );
			const float3 newRefl = Refl;//getSpecularDominantDirArea(N, Refl, avgR); 
			
			const float radiusSqr = Square( lData.AreaInfo.x );
			const float NdotL = dot(N, L);
			
			const float e = clamp(dot(lData.Dir, newRefl), -1, -0.01f);
			const float3 planeRay = WP - newRefl * DdotUL / e;
			const float3 newL = planeRay - lData.Pos;
			
			if(lData.AreaInfo.y == 0)
			{	
				// specular
				const float SphereAngle = clamp( -e * lData.AreaInfo.x * RLengthL, 0, 0.5 );
				specEnergy *= Square( aGGX / saturate( aGGX + SphereAngle ) );
			
				specL = specL + normalize(newL) * clamp(length(newL), 0, lData.AreaInfo.x);
					
				//diffuse
				diffL = diffL + lerp( N * lData.AreaInfo.x, float3(0,0,0), saturate(NdotL) );	
					
				// Disk evaluation
				const float sinSigmaSqr = radiusSqr / (radiusSqr + max(sqrDist, radiusSqr));
				illuminance = illuminanceSphereOrDisk( NdotL, sinSigmaSqr, noDirIlluminance );
			}
			else
			{
				// specular				
				const float LineXAngle = clamp( -e * lData.AreaInfo.z * RLengthL, 0, 0.5 );
				const float LineYAngle = clamp( -e * lData.AreaInfo.y * RLengthL, 0, 0.5 );
				specEnergy *= Square( aGGX ) / ( saturate( aGGX + LineXAngle ) * saturate( aGGX + LineYAngle ) );
				
				const float3 lightLeft = normalize(cross(lData.Dir, lData.DirUp)); // remove to c++ ??????
				
				specL = specL + clamp(dot(newL, lightLeft), -lData.AreaInfo.y, lData.AreaInfo.y) * lightLeft + clamp(dot(newL, lData.DirUp), -lData.AreaInfo.z, lData.AreaInfo.z) * lData.DirUp;
				
				//diffuse
				diffL = diffL + lerp( N * sqrt(radiusSqr + Square(2 * lData.AreaInfo.y)), float3(0,0,0), saturate(NdotL) );	
					
				// Rect evaluation
				illuminance = illuminanceRect(WP, lData.Pos, L, N, lData.Dir, lightLeft * lData.AreaInfo.y, lData.DirUp * lData.AreaInfo.z, noDirIlluminance);
			}
		
			coneFalloff *= saturate((dot(lData.Dir, -L) - 0.02) * 1.02); // clamp angle 89
							
			illuminance *= coneFalloff;
			noDirIlluminance *= coneFalloff;
					
			diffL = normalize(diffL);
			diffNoL = saturate(dot(N, diffL));	
			diffH = normalize(V + diffL);
			diffVoH = saturate(dot(V, diffH));

			specL = normalize(specL);
			specNoL = saturate( dot(N, specL) );
			specH = normalize(V + specL);
			specNoH = saturate( dot(N, specH) + 0.00001f );
			specVoH = saturate( dot(V, specH) );
		}
	}
	else
	{
		diffNoL = saturate(dot(N, L));
		if(diffNoL == 0.0f)
		{
			if(params.subscattering == 0)
				nope = true;
			else
				back = true;
		}
		
		if(!nope)
		{
			noDirIlluminance = getDistanceAtt( unnormL, lData.Range );
			if(noDirIlluminance == 0)nope = true;
			
			float coneFalloff = getAngleAtt(L, -lData.Dir, lData.Cone.r, lData.Cone.g);
			if(coneFalloff == 0)nope = true;
			
			noDirIlluminance *= coneFalloff;
			
			illuminance = noDirIlluminance * diffNoL;
			
			diffL = L;
			diffH = normalize(V + diffL);
			diffVoH = saturate(dot(V, diffH));
			diffNoL = saturate(dot(N, diffL));	
			
			specL = diffL;
			specH = diffH;
			specNoH = saturate( dot(N, specH) + 0.00001f );
			specVoH = diffVoH;
			specNoL = diffNoL;
		}
	}
	
	if(nope)return res;
	
	float3 colorIlluminance = illuminance * lData.Color.rgb;
	if(!back)
	{
		res.specular = colorIlluminance * specEnergy * directSpecularBRDF(S, R, specNoH, NoV, specNoL, specVoH, specH, T, B, avgR);	
		res.diffuse = colorIlluminance * directDiffuseBRDF(A, avgR, NoV, diffNoL, diffVoH);
	}
	if(params.subscattering != 0)
		res.diffuse += noDirIlluminance * lData.Color.rgb * directSubScattering(SS, params, diffL, N, V);
	
	res.nope = false;
	return res;
}

LightCalcOutput CalcSpotLightWithShadow(float3 WP, float3 V, float3 N, float3 T, float3 B, float3 S, float2 R, float avgR, float aGGX, float3 A, float3 SS, MaterialParamsStructBuffer params, LightShadowStructBuffer lData, float2 noise_coord, uint lnum, float NoV, float3 Refl)
{	
	LightCalcOutput res;
	res.diffuse = 0;
	res.specular = 0;

	SpotLightBuffer nlData;
	nlData.Dir = lData.Dir;
	nlData.Color = lData.Color;
	nlData.AreaInfo = lData.Pos_1;
	nlData.DirUp = lData.Pos_2;
	nlData.VirtualPos = lData.Pos_3;
	nlData.Range = lData.Range;
	nlData.Pos = lData.Pos;
	nlData.Cone = lData.Cone;
	
	res = CalcSpotLight(WP, V, N, T, B, S, R, avgR, aGGX, A, SS, params, nlData, NoV, Refl);
	if(res.nope)
		return res;
		
	float distL = length(lData.Pos_3 - WP);
		
	float shadowmap = 1.0f;
	const float pixMulPi = lData.ShadowPix*PIDIV2;
	const float2 borders = float2(0,1);
	if(!SpotlightShadow(shadowmap, lnum, WP, lData.lightViewMatrix, distL, lData.FilterType, lData.Bias, lData.LBR, lData.FilterSamples, lData.FilterSize, lData.noise, noise_coord, lData.ShadowPix, borders))
	{
		res.diffuse = 0;
		res.specular = 0;
		return res;
	}
	
	res.diffuse *= shadowmap;
	res.specular *= shadowmap;
	
	return res;
}

LightCalcOutput CalcPointLight(float3 WP, float3 V, float3 N, float3 T, float3 B, float3 S, float2 R, float avgR, float aGGX, float3 A, float3 SS, MaterialParamsStructBuffer params, PointLightBuffer lData, float NoV, float3 Refl)
{	
	LightCalcOutput res;
	res.diffuse = 0;
	res.specular = 0;
	res.nope = true;
	res.distL = 0;
	
	const float3 unnormL = lData.Pos - WP;
	const float3 L = normalize(unnormL);
	
	bool back = false;
	bool nope = false;
	
	float illuminance = 0;
	float noDirIlluminance = 0;
	
	float3 specL = unnormL;
	float3 specH = 0;
	float specNoH = 0;
	float specVoH = 0;
	float specNoL = 0;
	float specEnergy = 1;
	
	float3 diffL = unnormL;
	float3 diffH = 0;
	float diffVoH = 0;
	float diffNoL = 0;
	
	if(lData.AreaInfo.x > 0 || lData.AreaInfo.y > 0)
	{
		const float sqrDist = dot( unnormL, unnormL );
		float smoothFalloff = smoothDistanceAtt(sqrDist, lData.Range);
		if(smoothFalloff == 0)nope = true;

		if(!nope)
		{
			res.distL = sqrt( sqrDist );
			float RLengthL = rcp( max(res.distL, lData.AreaInfo.x) );
			const float3 newRefl = Refl;//getSpecularDominantDirArea(N, Refl, avgR); 
			
			const float radiusSqr = Square( lData.AreaInfo.x );
			const float NdotL = dot(N, L);
			
			if(lData.AreaInfo.y > 0) // to do: артефакты в спеке на больших рафнесах!!!!!!!!!!!!!!!!!
			{
				// specular
				const float LineAngle = saturate( lData.AreaInfo.y * RLengthL );
				specEnergy *= aGGX / saturate( aGGX + 0.5 * LineAngle );

				// Closest point on line segment to ray
				const float3 Ld = lData.Dir * lData.AreaInfo.y;
				const float3 halfLd = 0.5 * Ld;
				const float3 L0 = unnormL - halfLd;

				// Shortest distance
				const float sqrLength = Square( lData.AreaInfo.y );
				const float b = dot( newRefl, Ld );
				const float t = saturate( dot( L0, b * newRefl - Ld ) / (sqrLength - b*b) );

				specL = L0 + t * Ld;
				
				RLengthL = rcp( max(sqrt( dot( specL, specL ) ), lData.AreaInfo.x) );
				
				const float SphereAngle = clamp( lData.AreaInfo.x * RLengthL, 0, 0.5 );
				specEnergy *= Square( aGGX / saturate( aGGX + SphereAngle ) );
			
				const float3 centerToRay = dot(specL, newRefl) * newRefl - specL;
				specL = specL + centerToRay * saturate(lData.AreaInfo.x * rsqrt(dot(centerToRay, centerToRay)));
				
				// diffuse
				illuminance = illuminanceTube( lData.Pos, WP, N, lData.AreaInfo.x, radiusSqr, L, L0, Ld, sqrLength, noDirIlluminance );
				
				diffL = diffL + lerp( N * (lData.AreaInfo.y + 2 * lData.AreaInfo.x), float3(0,0,0), saturate(NdotL) );					
				//diffL = N * res.distL; // Lambert hacks
			}
			else
			{		
				// specular
				const float SphereAngle = clamp( lData.AreaInfo.x * RLengthL, 0, 0.5 );
				specEnergy *= Square( aGGX / saturate( aGGX + SphereAngle ) );
			
				const float3 centerToRay = dot(specL, newRefl) * newRefl - specL;
				specL = specL + centerToRay * saturate(lData.AreaInfo.x * rsqrt(dot(centerToRay, centerToRay)));
				
				//diffuse
				diffL = diffL + lerp( N * lData.AreaInfo.x, float3(0,0,0), saturate(NdotL) );	
					
				// Sphere evaluation
				const float cosTheta = clamp( NdotL, -0.999, 0.999);
				const float sinSigmaSqr = min( radiusSqr / sqrDist, 0.9999f );
				illuminance = illuminanceSphereOrDisk( cosTheta, sinSigmaSqr, noDirIlluminance );
			}
					
			illuminance *= smoothFalloff;
			noDirIlluminance *= smoothFalloff;
			
			diffL = normalize(diffL);
			diffNoL = saturate(dot(N, diffL));	
			diffH = normalize(V + diffL);
			diffVoH = saturate(dot(V, diffH));

			specL = normalize(specL);
			specNoL = saturate( dot(N, specL) );
			specH = normalize(V + specL);
			specNoH = saturate( dot(N, specH) + 0.00001f );
			specVoH = saturate( dot(V, specH) );
		}
	}
	else
	{
		diffNoL = saturate(dot(N, L));
		if(diffNoL == 0.0f)
		{
			if(params.subscattering == 0)
				nope = true;
			else
				back = true;
		}
		
		if(!nope)
		{
			noDirIlluminance = getDistanceAtt( unnormL, lData.Range );
			if(noDirIlluminance == 0)nope = true;
			illuminance = noDirIlluminance * diffNoL;
			
			diffL = L;
			diffH = normalize(V + diffL);
			diffVoH = saturate(dot(V, diffH));
			diffNoL = saturate(dot(N, diffL));	
			
			specL = diffL;
			specH = diffH;
			specNoH = saturate( dot(N, specH) + 0.00001f );
			specVoH = diffVoH;
			specNoL = diffNoL;
		}
	}
	
	if(nope)return res;
	
	float3 colorIlluminance = illuminance * lData.Color.rgb;
	if(!back)
	{
		res.specular = colorIlluminance * specEnergy * directSpecularBRDF(S, R, specNoH, NoV, specNoL, specVoH, specH, T, B, avgR);	
		res.diffuse = colorIlluminance * directDiffuseBRDF(A, avgR, NoV, diffNoL, diffVoH);
	}
	if(params.subscattering != 0)
		res.diffuse += noDirIlluminance * lData.Color.rgb * directSubScattering(SS, params, diffL, N, V);
	
	res.nope = false;
	return res;
}

LightCalcOutput CalcPointLightWithShadow(float3 WP, float3 V, float3 N, float3 T, float3 B, float3 S, float2 R, float avgR, float aGGX, float3 A, float3 SS, MaterialParamsStructBuffer params, LightShadowStructBuffer lData, float2 noise_coord, uint lnum, float NoV, float3 Refl)
{	
	LightCalcOutput res;
	res.diffuse = 0;
	res.specular = 0;

	PointLightBuffer nlData;
	nlData.Dir = lData.Dir;
	nlData.Color = lData.Color;
	nlData.AreaInfo = lData.Pos_1;
	nlData.Range = lData.Range;
	nlData.Pos = lData.Pos;
	
	res = CalcPointLight(WP, V, N, T, B, S, R, avgR, aGGX, A, SS, params, nlData, NoV, Refl);
	if(res.nope)
		return res;
		
	float distL = 0;
	if(res.distL != 0)
		distL = res.distL;
	else
		distL = length(lData.Pos - WP);
		
	float shadowmap = 1.0f;
	const float pixMulPi = lData.ShadowPix*PIDIV2;
	if(!PointlightShadow(shadowmap, lnum, WP, distL, lData.lightViewMatrix, lData.lightViewMatrix_1, lData.lightViewMatrix_2, lData.lightViewMatrix_3, lData.lightViewMatrix_4, lData.lightViewMatrix_5, lData.FilterType, lData.Bias, lData.LBR, lData.FilterSamples, lData.FilterSize, lData.noise, noise_coord, lData.ShadowPix, float2(pixMulPi,1.0f-pixMulPi)))
	{
		res.diffuse = 0;
		res.specular = 0;
		return res;
	}
	
	res.diffuse *= shadowmap;
	res.specular *= shadowmap;
	
	return res;
}

LightCalcOutput CalcDirLight(float3 V, float3 N, float3 T, float3 B, float3 S, float2 R, float avgR, float3 A, float3 SS, MaterialParamsStructBuffer params, DirLightBuffer lData, float NoV, float3 Refl)
{	
	LightCalcOutput res;
	res.diffuse = 0;
	res.specular = 0;
	
	float3 L = -lData.Dir;
	
	const float LdotRefl = saturate(dot(L , Refl));
	const float3 projRefl = Refl - LdotRefl * L;
	const float3 specL = LdotRefl < lData.AreaInfo.g ? normalize(lData.AreaInfo.g * L + normalize(projRefl) * lData.AreaInfo.r) : Refl;
	
	const float3 H = normalize(V + L);
	const float VoH = saturate( dot(V, H) );
	const float NoL = saturate(dot(N, L));
	
	const float3 specH = normalize(V + specL);
	const float specNoH = saturate( dot(N, specH) + 0.00001f );
	const float specVoH = saturate( dot(V, specH) );
	const float specNoL = saturate( dot(N, specL) );
	
	bool back = false;
	if(specNoL<=0.0f)
	{
		if(params.subscattering == 0)
			return res;
		else
			back = true;
	}
	
	if(params.subscattering == 0)
	{
		res.specular = specNoL * lData.Color.rgb * directSpecularBRDF(S, R, specNoH, NoV, specNoL, specVoH, specH, T, B, avgR);	
		res.diffuse = NoL * lData.Color.rgb * directDiffuseBRDF(A, avgR, NoV, NoL, VoH);
	}
	else
	{
		if(!back)
		{
			res.specular = specNoL * lData.Color.rgb * directSpecularBRDF(S, R, specNoH, NoV, specNoL, specVoH, specH, T, B, avgR);	
			res.diffuse = NoL * lData.Color.rgb * directDiffuseBRDF(A, avgR, NoV, NoL, VoH);
		}
		res.diffuse += lData.Color.rgb * directSubScattering(SS, params, L, N, V);
	}
		
	return res;
}

LightCalcOutput CalcDirLightWithShadow(float3 WP, float3 V, float3 N, float3 T, float3 B, float3 S, float2 R, float avgR, float3 A, float3 SS, MaterialParamsStructBuffer params, LightShadowStructBuffer lData, float2 noise_coord, uint lnum, float NoV, float3 Refl)
{	
	LightCalcOutput res;
	res.diffuse = 0;
	res.specular = 0;
	
	float shadowmap = 1.0f;
	if(!DirlightShadow(shadowmap, lnum, WP, lData.Pos, lData.Pos_1, lData.Pos_2, lData.Pos_3, lData.lightViewMatrix, lData.lightViewMatrix_1, lData.lightViewMatrix_2, lData.lightViewMatrix_3, lData.FilterType, lData.Bias, lData.LBR, lData.FilterSamples, lData.FilterSize, lData.noise, noise_coord, lData.ShadowPix, float2(lData.ShadowPix,1-lData.ShadowPix)))
		return res;

	DirLightBuffer nlData;
	nlData.Dir = lData.Dir;
	nlData.Color = lData.Color;
	nlData.AreaInfo.xy = lData.Cone;
	
	res = CalcDirLight(V, N, T, B, S, R, avgR, A, SS, params, nlData, NoV, Refl);
	res.diffuse *= shadowmap;
	res.specular *= shadowmap;
	
	return res;
}*/