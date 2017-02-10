
float4 CalcutaleMediumTransmittanceLight(SamplerState depthSamp, Texture2D <float2> sceneDepth, 
										 SamplerState samp, Texture2D <float4> sceneColor, float2 uv, 
										 MediumData mediumData, DataForLightCompute mData, GBufferData gbuffer, float3 V)
{
	// deform
	const float ior_air = 1.000277f;
	const float medium_ior = 1.5373f;
	//const float medium_absorb = 96.276f;
	const float medium_absorb = 9.276f;
	
	const float medium_thickness = 0.06f;
	
	float refract_sin_sqr = (ior_air / medium_ior) * (1 - mData.NoV * mData.NoV);
	float refract_cos_sqr = 1 - refract_sin_sqr;
	float refract_cos = sqrt(refract_cos_sqr);
	float refract_sin = sqrt(refract_sin_sqr);
	
	float3 binorm = cross(V, gbuffer.normal);
	float3 tangent = normalize(cross(gbuffer.normal, binorm));
	float3 refraction_dir = tangent * refract_sin - gbuffer.normal * refract_cos;
		
	float travel_dist = medium_thickness / refract_cos;
	
	float3 refract_ray = normalize(refraction_dir * travel_dist + V);
	
	float3 cam_tangent = normalize(cross(V, float3(0,1,0))); // temp
	float3 cam_binorm = normalize(cross(V, cam_tangent));
	
	const float fov_h = 0.47943f;
	const float fov_w = 0.77637f;
	
	float cam_h_cos = dot(refract_ray, cam_binorm);
	float cam_w_cos = dot(refract_ray, cam_tangent);
	
	float offset_h = clamp(cam_h_cos/fov_h, -1.0f, 1.0f);
	float offset_w = clamp(cam_w_cos/fov_w, -1.0f, 1.0f);
	
	float2 deform = /*float2(0.5f,0.5f) + */float2(offset_w, offset_h);


	float3 scene = sceneColor.SampleLevel(samp, uv + deform, (g_hizMipCount - 2) * mData.avgR).rgb;

	float3 absorbtion = exp(-medium_absorb * (float3(1,1,1) - gbuffer.albedo) * travel_dist);

	float3 temp = mediumData.insideColor * (mediumData.opacity + mediumData.insideRoughness + mediumData.absorption + mediumData.thickness);
	return float4(temp + scene * absorbtion, 0.9);
}