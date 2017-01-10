
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

#define PCF_DEPTH_TEST_SENCE 1000000.0f
#define PCF_PIXEL 1.0f / SHADOWS_BUFFER_RES

float2 horzPCF(float4 horzSamples[3], float uvFracX)
{
	float uvFravXInv = 1.0 - uvFracX;
	
	float2 res;

	res.x = horzSamples[0].w * uvFravXInv;
	res.y = horzSamples[0].x * uvFravXInv;
	res.x += horzSamples[0].z;
	res.y += horzSamples[0].y;

	res.x += horzSamples[1].w;
	res.y += horzSamples[1].x;
	res.x += horzSamples[1].z;
	res.y += horzSamples[1].y;

	res.x += horzSamples[2].w;
	res.y += horzSamples[2].x;
	res.x += horzSamples[2].z * uvFracX;
	res.y += horzSamples[2].y * uvFracX;

	return res;
}

float PCF_Filter(float3 UV, float depth, float mapScale, float sharpen)
{
	float2 uvInTexels = UV.xy * float2(SHADOWS_BUFFER_RES, SHADOWS_BUFFER_RES) - 0.5f;
	float2 uvFrac = frac(uvInTexels);
	float2 texelPos = floor(uvInTexels);

	float3 shadowCoords = float3( (texelPos + 0.5f) * PCF_PIXEL, UV.z );

	//float avgDepthShadow = 0;

	float2 vertSamples[3];

	[unroll]
	for(int i = -1; i <= 1; i++)
	{
		float4 horzSamples[3];

		[unroll]
		for(int j = -1; j <= 1; j++)
		{
			float4 shadowSample = shadows.Gather( samplerPointClamp, shadowCoords, int2(j, i) * 2 );
			//avgDepthShadow += shadowSample;
			horzSamples[j + 1] = clamp( (shadowSample - depth) * PCF_DEPTH_TEST_SENCE + 1.0f, 0.0f, 2.0f/*acne fading*/ );
		}
		vertSamples[i + 1] = horzPCF(horzSamples, uvFrac.x);
	}

	float shadow = vertSamples[0].x * (1 - uvFrac.y) + vertSamples[0].y;
	shadow += vertSamples[1].x + vertSamples[1].y;
	shadow += vertSamples[2].x + vertSamples[2].y * uvFrac.y;
	shadow *= 0.04f;
	
	//avgDepthShadow *= 0.1111111f;
	//sharpen = lerp(4 * sharpen, sharpen, saturate((depth - avgDepthShadow) * 5000.0f));

	return saturate( (saturate(shadow) - 0.5f) * sharpen + 0.5f );
}
/*
float PCF_Filter(float3 UV, float depth, float mapScale)
{
	float filterWidth = max(PCF_WIDTH * PCF_PIXEL * mapScale, PCF_PIXEL);

	float stepSize = 2 * filterWidth / PCF_NUM_SAMPLES;
	UV.xy -= float2(filterWidth, filterWidth);
	
	float sum = 0;
	for(int i=0; i<PCF_NUM_SAMPLES; i++)
		for(int j=0; j<PCF_NUM_SAMPLES; j++) 
		{
			float3 uv = UV;
			uv.xy += float2(i * stepSize, j * stepSize);
			float shadMapDepth = shadows.SampleLevel(samplerBilinearClamp, uv, 0).r;
			float shad = depth < shadMapDepth;
			sum += shad;
        }

	return sum / (PCF_NUM_SAMPLES * PCF_NUM_SAMPLES);
}*/

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
	
	const float resBiasScale = max(2, (texelSize.x * SHADOWS_RES_BIAS_SCALE) * 0.2); 
	lightViewProjPos.z -= shadowBiasSpot * min(10, depthFix.z * resBiasScale);
	float depthView = lightViewProjPos.z * lvp_rcp;
	
	return PCF_Filter(shadowmapCoords, depthView, adress.z, 1.0f);
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

	return PCF_Filter(shadowmapCoords, depthView, adress.z, 1.0f);
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
	
	float adress;
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
			adress = adress0.z;
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
			adress = adress1.z;
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
			adress = adress2.z;
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
			adress = adress3.z;
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
			adress = adress4.z;
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
			adress = adress5.z;
			shadowmapCoords.z = adress5.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
	}

	return PCF_Filter(shadowmapCoords, depthView, adress, 1.0f);
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
	
	float adress;
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
			adress = adress0.z;
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
			adress = adress1.z;
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
			adress = adress2.z;
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
			adress = adress3.z;
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
			adress = adress4.z;
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
			adress = adress5.z;
			shadowmapCoords.z = adress5.w;
			lightViewProjPos.z -= shadowBiasPoint * depthFix.z;
			depthView = lightViewProjPos.z * lvp_rcp;
		}
	}
	
	return PCF_Filter(shadowmapCoords, depthView, adress, 1.0f);
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
		//LPos = pos0;
		adress = adress0;
#if DEBUG_CASCADE_LIGHTS != 0
		res = 1.0;
#endif
		normalShadowOffsetDir = normalShadowOffsetDir0;
		//shadowBiasDir = shadowBiasDir0;
		//halfPix = texelSizes.x;
	}
	else
	{
		lightViewProjPos = mul(wpos4, vp1);
		if(abs(lightViewProjPos.x) < 1 && abs(lightViewProjPos.y) < 1 && lightViewProjPos.z < 1) // 1 cascede
		{
			viewproj = vp1;
			//LPos = pos1;
			adress = adress1;
#if DEBUG_CASCADE_LIGHTS != 0
			res = 0.75;
#endif
			normalShadowOffsetDir = normalShadowOffsetDir1;
			//shadowBiasDir = shadowBiasDir1;
			//halfPix = texelSizes.y;
		}
		else
		{
			lightViewProjPos = mul(wpos4, vp2);
			if(abs(lightViewProjPos.x) < 1 && abs(lightViewProjPos.y) < 1 && lightViewProjPos.z < 1) // 2 cascede
			{
				viewproj = vp2;
				//LPos = pos2;
				adress = adress2;
#if DEBUG_CASCADE_LIGHTS != 0
				res = 0.5;
#endif
				normalShadowOffsetDir = normalShadowOffsetDir2;
				//shadowBiasDir = shadowBiasDir2;
				//halfPix = texelSizes.z;
			}
			else // 3 cascade
			{
				lightViewProjPos = mul(wpos4, vp3);
				
				viewproj = vp3;
				//LPos = pos3;
				adress = adress3;
#if DEBUG_CASCADE_LIGHTS != 0
				res = 0.25;
#endif
				normalShadowOffsetDir = normalShadowOffsetDir3;
				//shadowBiasDir = shadowBiasDir3;
				//halfPix = texelSizes.w;
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
	return res * PCF_Filter(shadowmapCoords, depthView, adress.z, 1.0f);
#else
	return PCF_Filter(shadowmapCoords, depthView, adress.z, 1.0f);
#endif
}