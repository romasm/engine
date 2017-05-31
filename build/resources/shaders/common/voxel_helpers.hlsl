
#define VCT_MESH_MAX_INSTANCE 128
#define VCT_CLIPMAP_COUNT_MAX 12

#define VOXEL_SUBSAMPLES_COUNT 8
#define VOXEL_SUBSAMPLES_COUNT_RCP 1.0f / VOXEL_SUBSAMPLES_COUNT

#define COLOR_RANGE 255.0f 
#define COLOR_RANGE_RCP 1.0f / COLOR_RANGE

float4 DecodeVoxelColor(uint a, uint b, uint count)
{
	if(count == 0)
		return 0;

	float4 color;
	float mul = rcp(count);
	color.x = (float(a >> 16) * mul) * COLOR_RANGE_RCP;
	color.y = (float(a & 0x0000ffff) * mul) * COLOR_RANGE_RCP;
	color.z = (float(b >> 16) * mul) * COLOR_RANGE_RCP;
	color.w = (float(b & 0x0000ffff) * mul) * COLOR_RANGE_RCP * 100.0f;

	return color;
}

uint DecodeVoxelOpacity(uint a)
{
	return a & 0x0000ffff;
}

float3 DecodeVoxelNormal(uint n, uint o)
{
	float normalZ = float(o >> 16);
	uint count = o & 0x0000ffff;
		
	float3 normalXYZ = float3( float(n >> 16) * COLOR_RANGE_RCP, float(n & 0x0000ffff) * COLOR_RANGE_RCP, normalZ * COLOR_RANGE_RCP );
	normalXYZ = normalXYZ * 2.0f - (float)count;
	return normalize(normalXYZ);
}

static const float3 diffuseConeDirections[6] =
{
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 0.5f, 0.866025f),
    float3(0.823639f, 0.5f, 0.267617f),
    float3(0.509037f, 0.5f, -0.7006629f),
    float3(-0.50937f, 0.5f, -0.7006629f),
    float3(-0.823639f, 0.5f, 0.267617f)
};

static const float diffuseConeWeights[6] =
{
    5.0f / 20.0f,
    3.0f / 20.0f,
    3.0f / 20.0f,
    3.0f / 20.0f,
    3.0f / 20.0f,
    3.0f / 20.0f,
};

static const float3 diffuseConeDirectionsCheap[4] =
{
    float3(1.0f, 1.0f, 1.0f),
    float3(1.0f, 1.0f, -1.0f),
    float3(-1.0f, 1.0f, 1.0f),
    float3(-1.0f, 1.0f, -1.0f),
};

static const float diffuseConeWeightsCheap[4] =
{
    1.0f / 4.0f,
    1.0f / 4.0f,
    1.0f / 4.0f,
    1.0f / 4.0f
};

#define VOXEL_CONE_TRACING_STEP 0.5f
#define VOXEL_CONE_TRACING_MAX_STEPS 64

#define VOXEL_CONE_TRACING_LEVEL_FADE 0.05f
#define VOXEL_CONE_TRACING_LEVEL_FADE_RPC 1.0f / VOXEL_CONE_TRACING_LEVEL_FADE
#define VOXEL_CONE_TRACING_LEVEL_FADE_INV 1 - VOXEL_CONE_TRACING_LEVEL_FADE

#define VOXEL_FACE_COUNT_RCP 1.0f / 6

float4 VoxelConeTrace(float3 origin, float3 direction, float aperture, float3 surfaceNormal, VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX], 
					  Texture3D <float4> volumeEmittance, SamplerState volumeSampler)
{
	uint levelCount = volumeData[0].maxLevel + 1;

	float faces[3];
	faces[0] = (direction.x >= 0) ? 0 : 1;
    faces[1] = (direction.y >= 0) ? 2 : 3;
    faces[2] = (direction.z >= 0) ? 4 : 5;
	faces[0] *= VOXEL_FACE_COUNT_RCP;
	faces[1] *= VOXEL_FACE_COUNT_RCP;
	faces[2] *= VOXEL_FACE_COUNT_RCP;

	float dirWeight[3];
	dirWeight[0] = direction.x * direction.x;
	dirWeight[1] = direction.y * direction.y;
	dirWeight[2] = direction.z * direction.z;

	const float apertureDouble = 2.0f * aperture;

	// TODO: ???
	float3 coneStart = origin;// + surfaceNormal * volumeData[0].voxelSize;

	uint startLevel;
	[unroll]
	for(startLevel = 0; startLevel < levelCount; startLevel++)
	{
		float3 startCoords = (coneStart - volumeData[startLevel].cornerOffset) * volumeData[startLevel].worldSizeRcp;

		float maxStartCoord = max(startCoords.x, max(startCoords.y, startCoords.z));
		float minStartCoord = min(startCoords.x, min(startCoords.y, startCoords.z));
		[branch]
		if(maxStartCoord <= 1.0f && minStartCoord >= 0.0f)
			break;
	}

	if( startLevel == levelCount )
		return 0;

	float distance = volumeData[startLevel].voxelSize;
	float4 coneColor = 0;
	float level = 0;

	// temp 
	float levelCountRcp = 1.0f / levelCount;
	
	int i = 0;
	[loop]
	while(coneColor.a < 1.0f && i <= VOXEL_CONE_TRACING_MAX_STEPS && startLevel <= volumeData[0].maxLevel)
    {
        float3 currentConePos = coneStart + direction * distance;

        float diameter = apertureDouble * distance;
        level = log2(diameter * volumeData[0].voxelSizeRcp);
		level = clamp(level, startLevel, volumeData[0].maxLevel);
		
		float levelUpDown[2];
		levelUpDown[0] = ceil(level);
		levelUpDown[1] = floor(level);

		uint iDownLevel = (uint)levelUpDown[1];

		float3 sampleCoords[2];
        sampleCoords[1] = (currentConePos - volumeData[iDownLevel].cornerOffset) * volumeData[iDownLevel].worldSizeRcp;

		float maxCoord = max(sampleCoords[1].x, max(sampleCoords[1].y, sampleCoords[1].z));
		float minCoord = min(sampleCoords[1].x, min(sampleCoords[1].y, sampleCoords[1].z));
		[branch]
		if(maxCoord > 1.0f || minCoord < 0.0f)
		{
			startLevel++;
			continue;
		}

		uint iUpLevel = (uint)levelUpDown[0];
		sampleCoords[0] = (currentConePos - volumeData[iUpLevel].cornerOffset) * volumeData[iUpLevel].worldSizeRcp;
		
		float levelLerp = saturate(level - levelUpDown[1]);
		
		float4 voxelSample[2];
		[unroll]
		for(int voxelLevel = 0; voxelLevel < 2; voxelLevel++)
		{
			float3 coordsLevel = sampleCoords[voxelLevel];
			coordsLevel.xy *= float2(levelCountRcp, VOXEL_FACE_COUNT_RCP);
			coordsLevel.x += levelCountRcp * levelUpDown[voxelLevel];

			voxelSample[voxelLevel] = 0;
			[unroll]
			for(int faceID = 0; faceID < 3; faceID++)
			{
				float3 coords = coordsLevel;
				coords.y += faces[faceID];

				voxelSample[voxelLevel] += volumeEmittance.SampleLevel(volumeSampler, coords, 0) * dirWeight[faceID];
			}
		}
		        
		float4 finalColor = lerp( voxelSample[1], voxelSample[0], levelLerp);
		
		coneColor.rgb += (1.0f - coneColor.a) * finalColor.rgb; // todo: correct accumulation
		coneColor.a += finalColor.a;
        
        distance += diameter * VOXEL_CONE_TRACING_STEP;
		i++;
    }
	
	coneColor.a = saturate(coneColor.a);
	return coneColor;
}

#define RAY_TRACE_DISTANCE 350.0f
#define RAY_TRACE_NEARCLIP 0.2f

#define RAY_TRACE_EPCILON 0.00005f
#define RAY_TRACE_MAX_I 512

int4 GetVoxelOnRay(float3 origin, float3 ray, VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX], uint minLevel, Texture3D <float4> voxelEmittance, out float3 collideWS)
{
	collideWS = 0;
	
	uint levelCount = volumeData[0].maxLevel + 1;
	uint currentLevel = minLevel;
		
	float3 epcilon = RAY_TRACE_EPCILON;
	float3 step;
	step.x = ray.x >= 0 ? 1 : -1;
	step.y = ray.y >= 0 ? 1 : -1;
	step.z = ray.z >= 0 ? 1 : -1;

	epcilon *= step;

	step = saturate(step);
	
	float3 samplePoint = origin + ray * RAY_TRACE_NEARCLIP;
	float3 voxelSnap = 0;
	float3 prevVoxel = 0;

	ray *= RAY_TRACE_DISTANCE;
	
	int i = 0;
	[loop]
	while( i < RAY_TRACE_MAX_I && currentLevel < levelCount )
	{
		float3 inVolumePoint = samplePoint - volumeData[currentLevel].cornerOffset;
		
		[branch]
		if( inVolumePoint.x > volumeData[currentLevel].worldSize || inVolumePoint.x < 0 ||
			inVolumePoint.y > volumeData[currentLevel].worldSize || inVolumePoint.y < 0 ||
			inVolumePoint.z > volumeData[currentLevel].worldSize || inVolumePoint.z < 0 )
		{
			currentLevel++;
			continue;
		}
		
		voxelSnap = floor(inVolumePoint * volumeData[currentLevel].scaleHelper);

		int4 voxelCoords = int4(voxelSnap, 0);
		voxelCoords.x += volumeData[currentLevel].volumeRes * currentLevel;
		float anyValue = 0;
		[unroll]
		for(int j = 0; j < 6; j++)
		{
			anyValue += voxelEmittance.Load(voxelCoords).w;
			voxelCoords.y += volumeData[currentLevel].volumeRes;
		}
		
		voxelSnap *= volumeData[currentLevel].voxelSize;
		voxelSnap += volumeData[currentLevel].cornerOffset;
		prevVoxel = voxelSnap;

		[branch]
		if( anyValue > 0.0f )
			break;

		voxelSnap += step * volumeData[currentLevel].voxelSize;
		
		float3 currentRay = voxelSnap - origin;
		[branch]
		if( dot(currentRay, currentRay) > RAY_TRACE_DISTANCE * RAY_TRACE_DISTANCE )
			return -1;

		float3 delta = currentRay / ray;
		float d = min(delta.x, min(delta.y, delta.z));
		
		samplePoint = origin + ray * d + epcilon * volumeData[currentLevel].voxelSize;
		i++;
	}
	
	[branch]
	if( i == RAY_TRACE_MAX_I || currentLevel >= levelCount )
		return -1;

	float3 voxelExtend = volumeData[currentLevel].voxelSize * 0.5f;
	float3 originInVoxel = origin - (prevVoxel + voxelExtend);
	float2 voxelIntersections = RayBoxIntersect( originInVoxel, ray, -voxelExtend, voxelExtend );
	collideWS = origin + ray * voxelIntersections.x;
		
	float3 entry = collideWS - prevVoxel;
	entry.x = ray.x < 0 ? (volumeData[currentLevel].voxelSize - entry.x) : entry.x;
	entry.y = ray.y < 0 ? (volumeData[currentLevel].voxelSize - entry.y) : entry.y;
	entry.z = ray.z < 0 ? (volumeData[currentLevel].voxelSize - entry.z) : entry.z;
	float minEntry = min(entry.x, min(entry.y, entry.z));
	
	uint faceID;
	[branch]
	if(minEntry == entry.x) faceID = ray.x < 0 ? 1 : 0;
	else if(minEntry == entry.y) faceID = ray.y < 0 ? 3 : 2;
	else faceID = ray.z < 0 ? 5 : 4;

	prevVoxel = (prevVoxel - volumeData[currentLevel].cornerOffset) * volumeData[currentLevel].scaleHelper;
	prevVoxel.y += volumeData[currentLevel].volumeRes * faceID;
	prevVoxel.x += volumeData[currentLevel].volumeRes * currentLevel;

	return int4(prevVoxel, 0);	
}

// LIGHT CALCULATE
struct LightComponentsWeight
{
	float3 diffuse;
	float diffuseW;
	float3 specular;
	float specularW;
	float3 scattering;
	float scatteringW;
};

LightComponentsWeight CalculateVCTLight(sampler samp, Texture3D <float4> Emittance, VolumeData vData[VCT_CLIPMAP_COUNT_MAX], 
								  in GBufferData gbuffer, in DataForLightCompute mData, in float3 specularBrdf, in float3 diffuseBrdf, in float SO)
{
	LightComponentsWeight result = (LightComponentsWeight)0;

	// diffuse
	float4 diffuse = 0;
	const float apertureDiffuse = 0.57735f;

	float3 coneTangent, coneBinormal;
	[branch]
	if(abs(gbuffer.normal.y) <= 0.7)
	{
		coneTangent = normalize(cross(gbuffer.normal, float3(0,1,0)));
		coneBinormal = normalize(cross(gbuffer.normal, coneTangent));
	}
	else
	{
		coneBinormal = normalize(cross(gbuffer.normal, float3(1,0,0)));
		coneTangent = normalize(cross(gbuffer.normal, coneBinormal));
	}

	for(int diffuseCones = 0; diffuseCones < 4; diffuseCones++)
	{
		float3 coneDirection = gbuffer.normal;
		coneDirection += diffuseConeDirectionsCheap[diffuseCones].x * coneTangent + diffuseConeDirectionsCheap[diffuseCones].z * coneBinormal;
		coneDirection = normalize(coneDirection);
        
		float4 VCTdiffuse = VoxelConeTrace(gbuffer.wpos, coneDirection, apertureDiffuse, gbuffer.normal, vData, Emittance, samp);
		diffuse += VCTdiffuse * diffuseConeWeightsCheap[diffuseCones];
	}

	result.diffuse = diffuse.rgb * diffuseBrdf * gbuffer.ao;
	result.diffuseW = diffuse.a;

	// specular
	float3 coneReflDirection = normalize(mData.reflect);

	float apertureSpecular = tan( clamp( PIDIV2 * mData.avgR, 0.0174533f, PI) );
	float4 specular = VoxelConeTrace(gbuffer.wpos, coneReflDirection, apertureSpecular, gbuffer.normal, vData, Emittance, samp);
	 	
	result.specular = specular.rgb * specularBrdf * SO;
	result.specularW = specular.a;
		
	// TODO
	result.scattering = 0;
	result.scatteringW = 0;

	return result;
}