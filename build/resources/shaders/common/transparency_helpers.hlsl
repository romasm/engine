
#define ROUGHNESS_DISTANCE_INV 1.0f / 1.0f

float BackRefraction(float iorMedium, float R0, float3 V, float3 backNormal, float2 uv)
{
	float NoV = abs(dot(backNormal, V));

	float refractCosSqr = 1 - iorMedium * iorMedium * ( 1 - NoV * NoV );
	//float3 refractedRay = iorMedium * V + ( iorMedium * NoV - sqrt( max(0.001, refractCosSqr) ) ) * backNormal;
	//return float4(refractedRay, refractCosSqr);
	return refractCosSqr;
}

float4 EstimateRefraction(SamplerState samp, Texture2D <float4> sceneColor,
					float2 uv, MediumData mediumData, DataForLightCompute mData, GBufferData gbuffer, 
					float3 V, out float2 samplePoint)
{
	float refractCos = sqrt( 1 - mediumData.invIOR.g * mediumData.invIOR.g * ( 1 - mData.NoV * mData.NoV ) );
	float3 refractedRay = mediumData.invIOR.g * (-V) + ( mediumData.invIOR.g * mData.NoV - refractCos ) * gbuffer.normal;
	
	float travelDist = mediumData.thickness / refractCos;
	
	float3 refractedPoint = refractedRay * travelDist + gbuffer.wpos;
	samplePoint = WorldToScreen(float4(refractedPoint, 1.0));

	float depth = sceneColor.SampleLevel(samp, samplePoint, 0).a;
	depth = DepthToLinear(depth);

	float distance = max(0, depth - mediumData.frontDepth);

	return float4(refractedRay, distance);
}

float4 RefractScene(SamplerState samp, Texture2D <float4> sceneColor,  
					float2 uv, MediumData mediumData, DataForLightCompute mData, GBufferData gbuffer, 
					float3 V, float ior, float mipLevel)
{
	float refractCos = sqrt( 1 - ior * ior * ( 1 - mData.NoV * mData.NoV ) );
	float3 refractionDir = ior * (-V) + ( ior * mData.NoV - refractCos ) * gbuffer.normal;
	
	float travelDist = mediumData.thickness / refractCos;
	
	float3 refractedPoint = gbuffer.wpos + refractionDir * travelDist;
	float2 refractedScreenUV = WorldToScreen(float4(refractedPoint, 1.0));

	// TODO: what if sample mip?
	float depth = sceneColor.SampleLevel(samp, refractedScreenUV, 0).a;
	float3 color = 0;
	[branch]
	if(depth < gbuffer.depth)
		refractedScreenUV = uv;
	
	color = sceneColor.SampleLevel(samp, refractedScreenUV, mipLevel).rgb;
	return float4(color, refractCos);
}


float3 CalcutaleMediumTransmittedLight(SamplerState samp, Texture2D <float4> sceneColor, float2 uv, 
										 MediumData mediumData, DataForLightCompute mData, GBufferData gbuffer, float3 V)
{
	float2 samplePoint;
	float4 refractionRay = EstimateRefraction(samp, sceneColor, uv, mediumData, mData, gbuffer, V, samplePoint);
		
	// energy conservation on refraction
	float iorMedium = 1.0 / mediumData.invIOR.g;
	float R0 = IORtoR0( iorMedium );
	float energyFactor = saturate(1 - F_SchlickOriginal( R0, abs(dot(refractionRay.xyz, gbuffer.normal)) ));

	// back refraction
	float backRefraction = BackRefraction(iorMedium, R0, refractionRay.xyz, mediumData.backNormal, uv);

	// roughness cone
	float refractionRoughness = max(mData.avgR, mediumData.insideRoughness);
	float refractionRoughnessDistance = saturate(refractionRay.w * ROUGHNESS_DISTANCE_INV) * refractionRoughness;
	refractionRoughness = lerp(refractionRoughnessDistance, refractionRoughness, refractionRoughness);
	float mipLevel = refractionRoughness * (g_hizMipCount - 2);

	// thickness
	float refractedDepth = sceneColor.SampleLevel(samp, samplePoint, mipLevel).a;
	refractedDepth = DepthToLinear(refractedDepth);
	float hitBack = refractedDepth - mediumData.backDepth < 0 ? 0 : 1;
	float backDepth;
	[branch]
	if(refractedDepth < mediumData.frontDepth)
	{
		backDepth = mediumData.backDepth;
		hitBack = 1;
	}
	else
		backDepth = min(mediumData.backDepth, refractedDepth );

	float thicknessFinal = max(0, backDepth - mediumData.frontDepth);
	hitBack *= thicknessFinal != 0.0;

	// refraction
	float4 color_g = RefractScene(samp, sceneColor, uv, mediumData, mData, gbuffer, 
		V, mediumData.invIOR.g, mipLevel);
	
	float4 color_r = color_g;
	float4 color_b = color_g;
	[branch]
	if(mediumData.invIOR.r != mediumData.invIOR.b)
	{
		color_r = RefractScene(samp, sceneColor, uv, mediumData, mData, gbuffer, 
			V, mediumData.invIOR.r, mipLevel);
		color_b = RefractScene(samp, sceneColor, uv, mediumData, mData, gbuffer, 
			V, mediumData.invIOR.b, mipLevel);
	}
	
	float3 color_fin = float3(color_r.r, color_g.g, color_b.b) * energyFactor;
	float3 travelDist = thicknessFinal / float3(color_r.a, color_g.a, color_b.a);

	// back energy fake
	float fakeBackFactor = backRefraction;
	fakeBackFactor = -sin( ( saturate(-fakeBackFactor * (mediumData.invIOR.g * mediumData.invIOR.g)) - 0.5) * PI ) * 0.5 + 0.5;

	refractionRoughness *= 4.0;
	fakeBackFactor = lerp(fakeBackFactor, 1, saturate(refractionRoughness));
	fakeBackFactor = lerp(1, fakeBackFactor, hitBack * mediumData.tirAmount);
	
	color_fin = lerp(mediumData.absorption * color_fin, color_fin, saturate(fakeBackFactor));
	fakeBackFactor *= energyFactor;

	// Lambert-Beer law
	float3 attenFactor = -mediumData.attenuation * (1 - mediumData.absorption);
	float3 attenuation = exp(attenFactor * travelDist);
	float3 outColor = color_fin * attenuation * fakeBackFactor;
	
	return outColor;
}