
#define BRICK_ADRESS_BITS 24
#define BRICK_XY_BITS 12
#define BRICK_X_MASK 0x00fff000
#define BRICK_Y_MASK 0x00000fff
#define BRICK_DEPTH_MASK 0xff000000

void GetBrickAddres(float3 chunkPosFloor, float3 chunkPosFrac, out uint2 brickOffset, out uint brickDepth)
{
	uint4 lookupArrayAdress = g_giChunks.Load(int4((int3)chunkPosFloor, 0)); // .w - lookup resolution
	uint lookupRes = lookupArrayAdress.w;

	float3 lookupPos = chunkPosFrac * lookupRes;
	int3 lookupInOffset = (int3)floor(lookupPos);

	int4 lookupAdress = int4(lookupInOffset + lookupArrayAdress.xyz, 0);

	uint rawBrickAdress = g_giLookups.Load(lookupAdress);

	brickOffset = uint2((rawBrickAdress & BRICK_X_MASK) >> BRICK_XY_BITS, rawBrickAdress & BRICK_Y_MASK);
	brickDepth = ((rawBrickAdress & BRICK_DEPTH_MASK) >> BRICK_ADRESS_BITS); // 2 ^ depth
}

#ifndef GI_HELPERS_ONLY
float EvaluateSHIndirect(GBufferData gbuffer, float NoV, float Roughness, float3 V, out float3 diffuse)
{
	float3 chunkPos = (gbuffer.wpos - g_giSampleData.minCorner) * g_giSampleData.chunkSizeRcp;

	diffuse = 0;

	// check if outside
	if (chunkPos.x < 0 || chunkPos.y < 0 || chunkPos.z < 0 ||
		chunkPos.x >= g_giSampleData.chunksCount.x || chunkPos.y >= g_giSampleData.chunksCount.y ||
		chunkPos.z >= g_giSampleData.chunksCount.z)
		return 0;

	// TODO: half voxel size fading
	float fading = 1.0;

	float3 chunkPosFloor = floor(chunkPos);
	float3 chunkPosFrac = chunkPos - chunkPosFloor;

	uint2 brickOffset;
	uint brickDepth;
	GetBrickAddres(chunkPosFloor, chunkPosFrac, brickOffset, brickDepth);
	
	float3 brickInOffset = frac(chunkPosFrac * brickDepth);
	brickInOffset *= g_giSampleData.brickSampleSize;

	float3 brickSampleAdress = brickInOffset;
	brickSampleAdress.xy += brickOffset * g_giSampleData.brickAtlasOffset.xy;

	brickSampleAdress += g_giSampleData.halfBrickVoxelSize;

	SHcoef3 sh;
	[unroll]
	for (int i = 0; i < 9; i++)
	{
		sh.L[i] = g_giBricks.SampleLevel(samplerBilinearVolumeClamp, brickSampleAdress, 0).rgb;
		brickSampleAdress.z += g_giSampleData.brickAtlasOffset.z;
	}

	const float3 dominantN = getDiffuseDominantDir(gbuffer.normal, V, NoV, Roughness);
	const float3 color = ReconstrucColor(sh, dominantN);

	diffuse = color;// * gbuffer.albedo * gbuffer.ao;

	//diffuse = lerp(diffuse, brickInOffset, 0.999);

	return fading;
}
#endif