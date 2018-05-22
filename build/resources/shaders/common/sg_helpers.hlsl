
float4 EvaluateSGIndirect(GBufferData gbuffer)
{
	float3 chunkPos = (gbuffer.wpos - g_giSampleData.minCorner) * g_giSampleData.chunkSizeRcp;
	chunkPos = floor(chunkPos);

	float4 result = g_giVolume.SampleLevel(samplerBilinearVolumeClamp, volumePos, 0);
	result.rgb *= gbuffer.albedo;

	return result;
}