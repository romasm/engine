#include "../common/math.hlsl"
#include "../common/structs.hlsl"

#define GROUP_THREAD_COUNT_X 4
#define GROUP_THREAD_COUNT_Y 4
#define GROUP_THREAD_COUNT_Z 2

RWTexture3D <float4> bricksAtlas : register(u0);

Texture3D <float4> bricksTempAtlas : register(t0);

[numthreads( GROUP_THREAD_COUNT_X, GROUP_THREAD_COUNT_Y, GROUP_THREAD_COUNT_Z )]
void Copy3D(uint3 threadID : SV_DispatchThreadID)
{
	uint width, height, depth;
	bricksTempAtlas.GetDimensions(width, height, depth);
	if (threadID.x >= width || threadID.y >= height || threadID.z >= depth)
		return;
	
	bricksAtlas[threadID] = bricksTempAtlas.Load(int4(threadID, 0));
}