
float4 CalcutaleMediumTransmittanceLight(SamplerState depthSamp, Texture2D <float2> sceneDepth, 
										 SamplerState samp, Texture2D <float4> sceneColor, float2 uv, 
										 MediumData mediumData, DataForLightCompute mData, GBufferData gbuffer, float3 V)
{
	// deform
	const float ior_air = 1.000277f;
	const float medium_ior = 1.5373f;
	//const float medium_absorb = 96.276f;
	const float medium_absorb = 9.276f;
		
	float refract_sin_sqr = (ior_air / medium_ior) * (1 - mData.NoV * mData.NoV);
	float refract_cos_sqr = 1 - refract_sin_sqr;
	float refract_cos = sqrt(refract_cos_sqr);
	float refract_sin = sqrt(refract_sin_sqr);
	
	float3 binorm = cross(V, gbuffer.normal);
	float3 tangent = normalize(cross(gbuffer.normal, binorm));
	float3 refractionDir = tangent * refract_sin - gbuffer.normal * refract_cos;
		
	float travelDist = mediumData.thickness / refract_cos;
	
	float3 refractedPoint = refractionDir * travelDist + gbuffer.wpos;
	float2 refractedScreenUV = WorldToScreen(float4(refractedPoint, 1.0));
	
	float refractionRoughness = max(mData.avgR, mediumData.insideRoughness);
	refractionRoughness = pow(refractionRoughness, 0.7);

	float mipLevel = (g_hizMipCount - 2) * refractionRoughness; // depend of travelDist?
	float3 scene = sceneColor.SampleLevel(samp, refractedScreenUV, mipLevel).rgb;

	float3 absorbtion = exp(-mediumData.absorption * (1 - mediumData.insideColor) * travelDist);
	
	return float4(scene * absorbtion, 1 - mediumData.opacity);
}