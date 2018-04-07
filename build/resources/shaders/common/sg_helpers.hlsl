
float4 EvaluateSGIndirect(GBufferData gbuffer)
{
	float3 volumePos = (gbuffer.wpos - g_giSampleData.minCorner) * g_giSampleData.worldSizeRcp;

	float4 result = g_giVolume.SampleLevel(samplerBilinearVolumeClamp, volumePos, 0);
	result.rgb *= gbuffer.albedo;

	return result;
}