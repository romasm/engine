
#define BRICK_ADRESS_BITS 24
#define BRICK_XY_BITS 12
#define BRICK_X_MASK 0x00fff000
#define BRICK_Y_MASK 0x00000fff
#define BRICK_DEPTH_MASK 0xff000000
#define BRICK_RESOLUTION_SAMPLE 2
#define BRICK_RESOLUTION_SAMPLE_INV 1.0 / BRICK_RESOLUTION_SAMPLE

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

float3 GetGIAddres(float3 chunkPosFrac, uint2 brickOffset, uint brickDepth)
{
	float3 brickInOffset = frac(chunkPosFrac * brickDepth);
	brickInOffset *= g_giSampleData.brickSampleSize;

	float3 brickSampleAdress = brickInOffset;
	brickSampleAdress.xy += brickOffset * g_giSampleData.brickAtlasOffset.xy;

	brickSampleAdress += g_giSampleData.halfBrickVoxelSize;
	return brickSampleAdress;
}

//static const float3 boxPoints[8] = {
//	float3(0,0,0),
//	float3(1,0,0),
//	float3(0,1,0),
//	float3(1,1,0),
//	float3(0,0,1),
//	float3(1,0,1),
//	float3(0,1,1),
//	float3(1,1,1)
//};
//
//float3 GetGIAddresWithWeight(float3 chunkPosFrac, uint2 brickOffset, uint brickDepth, float3 normal)
//{
//	float3 brickInOffset = frac(chunkPosFrac * brickDepth);
//
//	float3 trilinearPosTrunk = brickInOffset * BRICK_RESOLUTION_SAMPLE;
//	float3 trilinearLerpPos = frac(trilinearPosTrunk);
//	trilinearPosTrunk -= trilinearLerpPos;
//	/*
//	float totalWeights = 0;
//	float weights[8];
//	float3 offsetVects[8];
//	[unroll]
//	for (int i = 0; i < 8; i++)
//	{
//		offsetVects[i] = boxPoints[i] - trilinearLerpPos;
//		weights[i] = max(0, dot(normalize(offsetVects[i]), normal));
//
//		totalWeights += weights[i];
//	}
//
//	float totalWeightsRcp = 1.0 / totalWeights;
//	[unroll]
//	for (int k = 0; k < 8; k++)
//	{
//		trilinearLerpPos += offsetVects[k] * weights[k] * totalWeightsRcp;
//	}
//	
//	brickInOffset = (trilinearPosTrunk + saturate(trilinearLerpPos)) * BRICK_RESOLUTION_SAMPLE_INV;
//	*/
//
//	/*float3 clampMax = trilinearLerpPos * 1.0;
//	float3 clampMin = saturate((trilinearLerpPos - 0.0) * 1.0) - 1.0;
//	float3 normalWeight = clamp(normal, clampMin, clampMax);
//	trilinearLerpPos = saturate(trilinearLerpPos + normalWeight);*/
//
//	brickInOffset = (trilinearPosTrunk + trilinearLerpPos) * BRICK_RESOLUTION_SAMPLE_INV;
//
//	brickInOffset *= g_giSampleData.brickSampleSize;
//
//	float3 brickSampleAdress = brickInOffset;
//	brickSampleAdress.xy += brickOffset * g_giSampleData.brickAtlasOffset.xy;
//
//	brickSampleAdress += g_giSampleData.halfBrickVoxelSize;
//	return brickSampleAdress;
//}

SGAmpl ReadBrickSG(float3 brickSampleAdress)
{
	SGAmpl sg;
	[unroll]
	for (int i = 0; i < SG_COUNT; i++)
	{
		sg.A[i] = g_giBricks.SampleLevel(samplerBilinearVolumeClamp, brickSampleAdress, 0).rgb;
		brickSampleAdress.z += g_giSampleData.brickAtlasOffset.z;
	}
	return sg;
}

#ifndef GI_HELPERS_ONLY
float EvaluateSGIndirect(GBufferData gbuffer, DataForLightCompute mData, float3 V, out float3 diffuse)
{
	float3 wposOffset = gbuffer.wpos + mData.dominantNormalDiffuse * g_giSampleData.minHalfVoxelSize;

	float3 chunkPos = (wposOffset - g_giSampleData.minCorner) * g_giSampleData.chunkSizeRcp;

	diffuse = 0;

	// check if outside
	if (chunkPos.x <= 0 || chunkPos.y <= 0 || chunkPos.z <= 0 ||
		chunkPos.x >= (float)g_giSampleData.chunksCount.x || chunkPos.y >= (float)g_giSampleData.chunksCount.y ||
		chunkPos.z >= (float)g_giSampleData.chunksCount.z)
		return 0;

	// TODO: half voxel size fading
	float fading = 1.0;
	
	float3 chunkPosFloor = floor(chunkPos);
	float3 chunkPosFrac = chunkPos - chunkPosFloor;
	
	uint2 brickOffset;
	uint brickDepth;
	GetBrickAddres(chunkPosFloor, chunkPosFrac, brickOffset, brickDepth);	
	float3 brickSampleAdress = GetGIAddres(chunkPosFrac, brickOffset, brickDepth);
	SGAmpl sg = ReadBrickSG(brickSampleAdress);

	ComputeSGLighting(diffuse, mData.dominantNormalDiffuse, sg, g_giSampleData.sgBasis, g_giSampleData.sgHelpers0, g_giSampleData.sgHelpers1);
	
	//diffuse = lerp(diffuse, brickInOffset, 0.999);

	return fading;
}
#endif