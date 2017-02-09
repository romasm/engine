
float4 CalcutaleMediumTransmittanceLight(SamplerState depthSamp, Texture2D <float2> sceneDepth, 
										 SamplerState samp, Texture2D <float4> sceneColor, MediumData mediumData)
{
	float3 temp = mediumData.insideColor * (mediumData.opacity + mediumData.insideRoughness + mediumData.absorption + mediumData.thickness);
	return float4(temp, mediumData.thickness);
}