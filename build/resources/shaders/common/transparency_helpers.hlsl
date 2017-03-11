
float4 EstimateRefraction(SamplerState depthSamp, Texture2D <float2> sceneDepth,
					float2 uv, MediumData mediumData, DataForLightCompute mData, GBufferData gbuffer, 
					float3 V, out float2 samplePoint)
{
	float refractCos = sqrt( 1 - mediumData.invIOR.g * mediumData.invIOR.g * ( 1 - mData.NoV * mData.NoV ) );
	float3 refractedRay = mediumData.invIOR.g * (-V) + ( mediumData.invIOR.g * mData.NoV - refractCos ) * gbuffer.normal;
	
	float travelDist = mediumData.thickness / refractCos;
	
	float3 refractedPoint = refractedRay * travelDist + gbuffer.wpos;
	samplePoint = WorldToScreen(float4(refractedPoint, 1.0));

	float depth = sceneDepth.SampleLevel(depthSamp, UVforSamplePow2(samplePoint), 0).r;
	depth = DepthToLinear(depth);

	float distance = max(0, depth - mediumData.frontDepth);

	return float4(refractedRay, distance);
}

float4 RefractScene(SamplerState depthSamp, Texture2D <float2> sceneDepth, 
					SamplerState samp, Texture2D <float4> sceneColor,  
					float2 uv, MediumData mediumData, DataForLightCompute mData, GBufferData gbuffer, 
					float3 V, float ior, float mipLevel)
{
	float refractCos = sqrt( 1 - ior * ior * ( 1 - mData.NoV * mData.NoV ) );
	float3 refractionDir = ior * (-V) + ( ior * mData.NoV - refractCos ) * gbuffer.normal;
	
	float travelDist = mediumData.thickness / refractCos;
	
	float3 refractedPoint = refractionDir * travelDist + gbuffer.wpos;
	float2 refractedScreenUV = WorldToScreen(float4(refractedPoint, 1.0));

	float depth = sceneDepth.SampleLevel(depthSamp, UVforSamplePow2(refractedScreenUV), 0).r;
	float3 color = 0;
	[branch]
	if(depth < gbuffer.depth)
		color = sceneColor.SampleLevel(samp, uv, mipLevel).rgb;
	else
		color = sceneColor.SampleLevel(samp, refractedScreenUV, mipLevel).rgb;
	
	return float4(color, refractCos);
}

float3 CalcutaleMediumTransmittedLight(SamplerState depthSamp, Texture2D <float2> sceneDepth, 
										 SamplerState samp, Texture2D <float4> sceneColor, float2 uv, 
										 MediumData mediumData, DataForLightCompute mData, GBufferData gbuffer, float3 V)
{
	float2 samplePoint;
	float4 refractionRay = EstimateRefraction(depthSamp, sceneDepth, uv, mediumData, mData, gbuffer, V, samplePoint);
		
	// energy conservation on refraction
	float R0 = IORtoR0( 1.0 / mediumData.invIOR.g );
	float energyFactor = saturate(1 - F_SchlickOriginal( R0, abs(dot(refractionRay.xyz, gbuffer.normal)) ));

	// roughness cone
	float refractionRoughness = max(mData.avgR, mediumData.insideRoughness);
	float mipLevel = saturate(refractionRay.w) * refractionRoughness;
	mipLevel *= (g_hizMipCount - 2);
	
	// refraction
	float4 color_g = RefractScene(depthSamp, sceneDepth, samp, sceneColor, uv, mediumData, mData, gbuffer, 
		V, mediumData.invIOR.g, mipLevel);
	
	float4 color_r = color_g;
	float4 color_b = color_g;
	[branch]
	if(mediumData.invIOR.r != mediumData.invIOR.b)
	{
		float3 tempRay;
		float3 tempPoint;
		color_r = RefractScene(depthSamp, sceneDepth, samp, sceneColor, uv, mediumData, mData, gbuffer, 
			V, mediumData.invIOR.r, mipLevel);
		color_b = RefractScene(depthSamp, sceneDepth, samp, sceneColor, uv, mediumData, mData, gbuffer, 
			V, mediumData.invIOR.b, mipLevel);
	}
	
	// thickness
	float refractedDepth = sceneDepth.SampleLevel(samp, UVforSamplePow2(samplePoint), mipLevel).g;
	float backDepth = min(mediumData.backDepth, DepthToLinear(refractedDepth) );
	float thicknessFinal = max(0, backDepth - mediumData.frontDepth);

	float3 color_fin = float3(color_r.r, color_g.g, color_b.b) * energyFactor;
	float3 travelDist = thicknessFinal / float3(color_r.a, color_g.a, color_b.a);

	// Lambert-Beer law
	float3 attenuation = exp(-mediumData.attenuation * (1 - mediumData.absorption) * travelDist);
	float3 outColor = color_fin * attenuation;

	return outColor * energyFactor; // TODO: correct energy conservation based on back normals from ddBackDepth
}