
#define BRICK_ADRESS_BITS 24
#define BRICK_XY_BITS 12
#define BRICK_X_MASK 0x00fff000
#define BRICK_Y_MASK 0x00000fff
#define BRICK_DEPTH_MASK 0xff000000

float EvaluateSHIndirect(GBufferData gbuffer, out float3 diffuse)
{
	float3 chunkPos = (gbuffer.wpos - g_giSampleData.minCorner) * g_giSampleData.chunkSizeRcp;
	int4 chunkAdress = int4((int3)floor(chunkPos), 0);

	diffuse = 0;

	// check if outside
	if (chunkAdress.x < 0 || chunkAdress.y < 0 || chunkAdress.z < 0 ||
		chunkAdress.x >= (int)g_giSampleData.chunksCount.x || chunkAdress.y >= (int)g_giSampleData.chunksCount.y ||
		chunkAdress.z >= (int)g_giSampleData.chunksCount.z)
		return 0;

	// TODO: half voxel size fading
	float fading = 1.0;


	uint4 lookupArrayAdress = g_giChunks.Load(chunkAdress); // .w - lookup resolution
	uint lookupRes = lookupArrayAdress.w;

	float3 lookupPosRelative = chunkPos - chunkAdress.xyz;
	float3 lookupPos = lookupPosRelative * lookupRes;
	int3 lookupInOffset = (int3)floor(lookupPos);

	int4 lookupAdress = int4(lookupInOffset + lookupArrayAdress.xyz, 0);

	uint rawBrickAdress = g_giLookups.Load(lookupAdress);

	uint2 brickOffset = uint2((rawBrickAdress & BRICK_X_MASK) >> BRICK_XY_BITS, rawBrickAdress & BRICK_Y_MASK);
	uint brickDepth = ((rawBrickAdress & BRICK_DEPTH_MASK) >> BRICK_ADRESS_BITS); // 2 ^ depth
	
	float3 brickInOffset = floor(lookupPosRelative * brickDepth);
	brickInOffset *= g_giSampleData.brickSampleSize;

	float3 brickSampleAdress = brickInOffset;
	brickSampleAdress.xy += brickOffset * g_giSampleData.brickAtlasOffset.xy;

	brickSampleAdress += g_giSampleData.halfBrickVoxelSize;

	SHcoef sh;
	[unroll]
	for (int i = 0; i < 9; i++)
	{
		sh.L[i] = g_giBricks.SampleLevel(samplerBilinearVolumeClamp, brickSampleAdress, 0).rgb;
		brickSampleAdress.z += g_giSampleData.brickAtlasOffset.z;
	}

	float3 color = ReconstrucColor(sh, gbuffer.normal);

	diffuse = color * gbuffer.albedo;

	return fading;
}