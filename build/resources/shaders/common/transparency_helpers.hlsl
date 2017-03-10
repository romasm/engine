
float4 RefractScene(SamplerState samp, Texture2D <float4> sceneColor, float2 uv, MediumData mediumData, DataForLightCompute mData, GBufferData gbuffer, float3 V, float ior)
{
	float refractCos = sqrt( 1 - ior * ior * ( 1 - mData.NoV * mData.NoV ) );
	float3 refractionDir = ior * V + ( ior * mData.NoV - refractCos ) * gbuffer.normal;
	
	float travelDist = mediumData.thickness / refractCos;
	
	float3 refractedPoint = refractionDir * travelDist + gbuffer.wpos;
	float2 refractedScreenUV = WorldToScreen(float4(refractedPoint, 1.0));

	// TEMP
	float refractionRoughness = max(mData.avgR, mediumData.insideRoughness);
	refractionRoughness = pow(refractionRoughness, 0.7);
	float mipLevel = (g_hizMipCount - 2) * refractionRoughness; // depend of travelDist?

	float3 color = sceneColor.SampleLevel(samp, refractedScreenUV, mipLevel).rgb;
	return float4(color, travelDist);
}

float4 CalcutaleMediumTransmittanceLight(SamplerState depthSamp, Texture2D <float2> sceneDepth, 
										 SamplerState samp, Texture2D <float4> sceneColor, float2 uv, 
										 MediumData mediumData, DataForLightCompute mData, GBufferData gbuffer, float3 V)
{
	// deform
	//const float ior_air = 1.000277f;
	//const float medium_ior = 1.5373f;	
	//const float n = ior_air / medium_ior;

	float4 color_g = RefractScene(samp, sceneColor, uv, mediumData, mData, gbuffer, V, mediumData.invIOR.g);
	
	float4 color_r = color_g;
	float4 color_b = color_g;
	[branch]
	if(mediumData.invIOR.r != mediumData.invIOR.b)
	{
		color_r = RefractScene(samp, sceneColor, uv, mediumData, mData, gbuffer, V, mediumData.invIOR.r);
		color_b = RefractScene(samp, sceneColor, uv, mediumData, mData, gbuffer, V, mediumData.invIOR.b);
	}

	float3 color_fin = float3(color_r.r, color_g.g, color_b.b);
	float3 travelDist = float3(color_r.a, color_g.a, color_b.a);

	float3 absorbtion = exp(-mediumData.absorption * (1 - mediumData.insideColor) * travelDist);
	
	return float4(color_fin * absorbtion, 1 - mediumData.opacity);
}