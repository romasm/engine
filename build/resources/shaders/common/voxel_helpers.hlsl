
#define VCT_MESH_MAX_INSTANCE 128
#define VCT_CLIPMAP_COUNT_MAX 20

#define VOXEL_FACES_COUNT 6

#define VOXEL_SUBSAMPLES_COUNT 8
#define VOXEL_SUBSAMPLES_COUNT_RCP (1.0f / VOXEL_SUBSAMPLES_COUNT)

#define COLOR_RANGE 255.0f 
#define COLOR_RANGE_RCP (1.0f / COLOR_RANGE)

static const float emissiveFalloff = 6.0; 

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

float4 DecodeVoxelData(float4 data)
{
	[flatten] 
	if(data.a > 0)
		data.rgb = (data.rgb / data.a);
	data.a = saturate(data.a);
	return data;

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

#define VOXEL_VIS_THRESH 0.95f
float4 VoxelConeTrace(float3 origin, float3 direction, float aperture, float3 surfaceNormal, VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX], 
					  VolumeTraceData volumeTraceData, Texture3D <float4> volumeEmittance, SamplerState volumeSampler)
{
	const float apertureDouble = 2.0f * aperture;

	uint startLevel;
	//float startLevelFade = 1.0;
	[unroll]
	for(startLevel = 0; startLevel < volumeTraceData.clipmapCount; startLevel++)
	{
		float3 startCoords = (origin - volumeData[startLevel].cornerOffset) * volumeData[startLevel].worldSizeRcp;

		float maxStartCoord = max(startCoords.x, max(startCoords.y, startCoords.z));
		float minStartCoord = min(startCoords.x, min(startCoords.y, startCoords.z));
		[branch]
		if(maxStartCoord <= 1.0f && minStartCoord >= 0.0f)
		{
			//startLevelFade = saturate(min(1 - maxStartCoord, minStartCoord) * volumeData[startLevel].volumeRes);
			break;
		}
	}
	if( startLevel == volumeTraceData.clipmapCount )
		return 0;

	// TODO: ???
	float3 coneStart = origin + surfaceNormal * volumeData[startLevel].voxelSize;

	float distance = volumeData[startLevel].voxelSize;
	float4 coneColor = 0;
		
	int i = 0;
	float currentLevel = (float)startLevel;
	[loop]
	while(coneColor.a < VOXEL_VIS_THRESH && i <= VOXEL_CONE_TRACING_MAX_STEPS && uint(currentLevel) < volumeTraceData.levelsCount)
    {
		i++;

        float3 currentConePos = coneStart + direction * distance;

		/*
		float3 sampleCoords = (currentConePos - volumeData[0].cornerOffset) * volumeData[0].worldSizeRcp;

		sampleCoords.x *= volumeTraceData.xVolumeSizeRcp;
		sampleCoords.xy += volumeData[0].levelOffsetTex;

		float4 voxelSample = DecodeVoxelData(volumeEmittance.SampleLevel(volumeSampler, sampleCoords, 0));
				
		const float occluded = 1.0f - coneColor.a;
		coneColor.rgb = lerp(coneColor.rgb, voxelSample.rgb, occluded);//occluded * voxelSample;
        
		coneColor.a += voxelSample.a * occluded;

        distance += 0.2;
		*/
		
        float diameter = apertureDouble * distance;
        float level = log2(diameter * volumeData[0].voxelSizeRcp);
		currentLevel = clamp(level, currentLevel, volumeTraceData.maxLevel);
		 
		float levelUpDown[2];
		levelUpDown[0] = floor(currentLevel);
		levelUpDown[1] = ceil(currentLevel);

		uint iDownLevel = (uint)levelUpDown[0];

		float3 sampleCoords[2];
        sampleCoords[0] = (currentConePos - volumeData[iDownLevel].cornerOffset) * volumeData[iDownLevel].worldSizeRcp;

		float maxCoord = max(sampleCoords[0].x, max(sampleCoords[0].y, sampleCoords[0].z));
		float minCoord = min(sampleCoords[0].x, min(sampleCoords[0].y, sampleCoords[0].z));
		[branch]
		if(maxCoord > 1.0f || minCoord < 0.0f)
		{
			currentLevel = levelUpDown[0] + 1.0;
			continue;
		}

		uint iUpLevel = (uint)levelUpDown[1];
		sampleCoords[1] = (currentConePos - volumeData[iUpLevel].cornerOffset) * volumeData[iUpLevel].worldSizeRcp;
		
		float levelLerp = 0.0;//frac(currentLevel);
				
		float4 voxelSample[2];
		[unroll]
		for(int voxelLevel = 0; voxelLevel < 2; voxelLevel++)
		{
			float3 coordsLevel = sampleCoords[voxelLevel];
			coordsLevel.x *= volumeTraceData.xVolumeSizeRcp;
			coordsLevel.xy += volumeData[levelUpDown[voxelLevel]].levelOffsetTex;

			voxelSample[voxelLevel] = DecodeVoxelData(volumeEmittance.SampleLevel(volumeSampler, coordsLevel, 0));
		}
		
		const float4 finalColor = lerp(voxelSample[0], voxelSample[1], levelLerp);
		
		const float occluded = 1.0f - coneColor.a;
		coneColor.rgb += finalColor.rgb * occluded * finalColor.a;// * emissiveFalloff;
		coneColor.a += finalColor.a * occluded;
        
        distance += diameter * VOXEL_CONE_TRACING_STEP;
    }
	
	coneColor.a = saturate(coneColor.a / VOXEL_VIS_THRESH);
	return coneColor;
}

#define RAY_TRACE_DISTANCE 350.0f
#define RAY_TRACE_NEARCLIP 0.2f

#define RAY_TRACE_EPCILON 0.00005f
#define RAY_TRACE_MAX_I 512

int4 GetVoxelOnRay(float3 origin, float3 ray, VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX], VolumeTraceData volumeTraceData, uint minLevel, 
				   Texture3D <float4> voxelEmittance, out float3 collideWS)
{
	collideWS = 0;
	
	uint currentLevel = minLevel;
		
	float3 epcilon = RAY_TRACE_EPCILON;
	float3 step;
	step.x = ray.x >= 0 ? 1 : -1;
	step.y = ray.y >= 0 ? 1 : -1;
	step.z = ray.z >= 0 ? 1 : -1;

	epcilon *= step;

	step = saturate(step);
	
	float3 samplePoint = origin + ray * RAY_TRACE_NEARCLIP;
	float3 prevVoxel = 0;

	ray *= RAY_TRACE_DISTANCE;
	
	int i = 0;
	[loop]
	while( i < RAY_TRACE_MAX_I && currentLevel < volumeTraceData.levelsCount )
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
		
		float3 voxelSnap = floor(inVolumePoint * volumeData[currentLevel].scaleHelper);

		int4 voxelCoords = int4(voxelSnap, 0);
		voxelCoords.xy += volumeData[currentLevel].levelOffset;
		voxelSnap *= volumeData[currentLevel].voxelSize;
		voxelSnap += volumeData[currentLevel].cornerOffset;
		prevVoxel = voxelSnap;

		[branch]
		if( voxelEmittance.Load(voxelCoords).w > 0.0f )
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
	if( i == RAY_TRACE_MAX_I || currentLevel >= volumeTraceData.levelsCount )
		return -1;

	float3 voxelExtend = volumeData[currentLevel].voxelSize * 0.5f;
	float3 originInVoxel = origin - (prevVoxel + voxelExtend);
	float2 voxelIntersections = RayBoxIntersect( originInVoxel, ray, -voxelExtend, voxelExtend );
	collideWS = origin + ray * voxelIntersections.x;

	prevVoxel = (prevVoxel - volumeData[currentLevel].cornerOffset) * volumeData[currentLevel].scaleHelper;
	prevVoxel.xy += volumeData[currentLevel].levelOffset;

	return int4(prevVoxel, 0);	
}

float4 GetVoxelLightOnRay(float3 origin, float3 ray, float viewLength, VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX], 
						  VolumeTraceData volumeTraceData, uint minLevel, Texture3D <float4> voxelEmittance, float valueScale)
{
	uint currentLevel = minLevel;
	
	float3 epcilon = RAY_TRACE_EPCILON;
	float3 step;
	step.x = ray.x >= 0 ? 1 : -1;
	step.y = ray.y >= 0 ? 1 : -1;
	step.z = ray.z >= 0 ? 1 : -1;

	epcilon *= step;

	step = saturate(step);
	
	const float traceLength = min(viewLength / RAY_TRACE_DISTANCE, 1.0);
	float currentLength = RAY_TRACE_NEARCLIP / RAY_TRACE_DISTANCE;

	ray *= RAY_TRACE_DISTANCE;
	
	float4 totalColor = 0;
	int i = 0;
	[loop]
	while( i < RAY_TRACE_MAX_I && currentLevel < volumeTraceData.levelsCount && currentLength < traceLength )
	{
		const float3 samplePoint = origin + ray * currentLength;
		float3 inVolumePoint = samplePoint - volumeData[currentLevel].cornerOffset;
		inVolumePoint += epcilon * volumeData[currentLevel].voxelSize;
		
		[branch]
		if( inVolumePoint.x > volumeData[currentLevel].worldSize || inVolumePoint.x < 0 ||
			inVolumePoint.y > volumeData[currentLevel].worldSize || inVolumePoint.y < 0 ||
			inVolumePoint.z > volumeData[currentLevel].worldSize || inVolumePoint.z < 0 )
		{
			currentLevel++;
			continue;
		}
		
		float3 voxelSnap = floor(inVolumePoint * volumeData[currentLevel].scaleHelper);

		int4 voxelCoords = int4(voxelSnap, 0);
		voxelCoords.xy += volumeData[currentLevel].levelOffset;
		
		voxelSnap *= volumeData[currentLevel].voxelSize;
		voxelSnap += volumeData[currentLevel].cornerOffset;
				
		const float3 voxelExtend = volumeData[currentLevel].voxelSize * 0.5f;

		const float3 originInVoxel = origin - (voxelSnap + voxelExtend);
		const float2 voxelIntersections = RayBoxIntersect( originInVoxel, ray, -voxelExtend, voxelExtend );
		
		float3 entry = origin + ray * voxelIntersections.x - voxelSnap;
		entry.x = ray.x < 0 ? (volumeData[currentLevel].voxelSize - entry.x) : entry.x;
		entry.y = ray.y < 0 ? (volumeData[currentLevel].voxelSize - entry.y) : entry.y;
		entry.z = ray.z < 0 ? (volumeData[currentLevel].voxelSize - entry.z) : entry.z;
		const float minEntry = min(entry.x, min(entry.y, entry.z));
	
		uint faceID;
		[branch]
		if(minEntry == entry.x) faceID = ray.x < 0 ? 1 : 0;
		else if(minEntry == entry.y) faceID = ray.y < 0 ? 3 : 2;
		else faceID = ray.z < 0 ? 5 : 4;

		voxelCoords.y += volumeData[0].volumeRes * faceID;

		const float4 sample = voxelEmittance.Load(voxelCoords);
		totalColor.rgb += (1 - totalColor.a) * sample.rgb;
		totalColor.a = saturate(totalColor.a + /*sample.a * */valueScale);
		[branch]
		if( totalColor.a >= 0.5 )
			break;

		voxelSnap += step * volumeData[currentLevel].voxelSize;
		
		const float3 currentRay = voxelSnap - origin;
		const float3 delta = currentRay / ray;
		currentLength = min(delta.x, min(delta.y, delta.z));
		
		i++;
	}		
	return totalColor;	
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

LightComponentsWeight CalculateVCTLight(sampler samp, Texture3D <float4> Emittance, VolumeData vData[VCT_CLIPMAP_COUNT_MAX], VolumeTraceData vTraceData, 
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
        
		float4 VCTdiffuse = VoxelConeTrace(gbuffer.wpos, coneDirection, apertureDiffuse, gbuffer.normal, vData, vTraceData, Emittance, samp);
		diffuse += VCTdiffuse * diffuseConeWeightsCheap[diffuseCones];
	}

	result.diffuse = diffuse.rgb * diffuseBrdf * gbuffer.ao;
	result.diffuseW = diffuse.a;

	// specular
	float3 coneReflDirection = normalize(mData.reflect);

	float apertureSpecular = tan( clamp( PIDIV2 * mData.avgR, 0.0174533f, PI) );
	float4 specular = VoxelConeTrace(gbuffer.wpos, coneReflDirection, apertureSpecular, gbuffer.normal, vData, vTraceData, Emittance, samp);
	 	
	result.specular = specular.rgb * specularBrdf * SO;
	result.specularW = specular.a;
		
	// TODO
	result.scattering = 0;
	result.scatteringW = 0;

	return result;
}

LightComponentsWeight GetIndirectLight(sampler samp, Texture3D <float4> lightVolume, Texture3D <float4> emittanceVolume, VolumeData vData[VCT_CLIPMAP_COUNT_MAX], VolumeTraceData vTraceData, 
								  in GBufferData gbuffer, in DataForLightCompute mData, in float3 specularBrdf, in float3 diffuseBrdf, in float SO)
{
	LightComponentsWeight result = (LightComponentsWeight)0;

	// diffuse
	uint level = 0;
	float3 coordsLevel;
	[unroll]
	for(level = 0; level <= vTraceData.maxLevel; level++)
	{
		const float3 diffPos = gbuffer.wpos + gbuffer.normal * vData[level].voxelDiag;
		coordsLevel = (diffPos - vData[level].cornerOffset) * vData[level].worldSizeRcp;
		[flatten]
		if( !any(coordsLevel - saturate(coordsLevel)) )
			break;
	}

	[flatten]
	if(level <= vTraceData.maxLevel)
	{
		coordsLevel.xy *= float2(vTraceData.xVolumeSizeRcp, VOXEL_FACE_COUNT_RCP);
		coordsLevel.xy += vData[level].levelOffsetTex;
				
		float dirWeight[3];
		dirWeight[0] = gbuffer.normal.x * gbuffer.normal.x;
		dirWeight[1] = gbuffer.normal.y * gbuffer.normal.y;
		dirWeight[2] = gbuffer.normal.z * gbuffer.normal.z;

		float faces[3];
		faces[0] = (gbuffer.normal.x >= 0) ? 0 : 1;
		faces[1] = (gbuffer.normal.y >= 0) ? 2 : 3;
		faces[2] = (gbuffer.normal.z >= 0) ? 4 : 5;
		faces[0] *= VOXEL_FACE_COUNT_RCP;
		faces[1] *= VOXEL_FACE_COUNT_RCP;
		faces[2] *= VOXEL_FACE_COUNT_RCP;
		
		float4 diffuse = 0;
		[unroll]
		for(int f = 0; f < 3; f++)
		{
			float3 faceCoords = coordsLevel;
			faceCoords.y += faces[f];
			diffuse += lightVolume.SampleLevel(samp, faceCoords, 0) * dirWeight[f];
		}

		result.diffuse = diffuse.rgb * diffuseBrdf;
		result.diffuseW = saturate(/*(1 - diffuse.a) */ gbuffer.ao);
	}

	// specular
	float3 coneReflDirection = normalize(mData.reflect);

	float apertureSpecular = tan( clamp( PIDIV2 * mData.avgR, 0.0174533f, PI) );
	float4 specular = VoxelConeTrace(gbuffer.wpos, coneReflDirection, apertureSpecular, gbuffer.vertex_normal, vData, vTraceData, emittanceVolume, samp);
	 
	result.specular = specular.rgb * specularBrdf * SO;
	result.specularW = specular.a;
	
	// TODO
	//result.scattering = 0;
	//result.scatteringW = 0;

	return result;
}

float4 GetIndirectVoxel(sampler samp, Texture3D <float4> lightVolume, VolumeData vData[VCT_CLIPMAP_COUNT_MAX], VolumeTraceData vTraceData, 
								  float3 wpos, float3 normal, float3 diffuseBrdf)
{
	float4 result = 0;

	uint level = 0;
	float3 coordsLevel;
	[unroll]
	for(level = 0; level <= vTraceData.maxLevel; level++)
	{
		const float3 diffPos = wpos + normal * vData[level].voxelDiag;
		coordsLevel = (diffPos - vData[level].cornerOffset) * vData[level].worldSizeRcp;
		[flatten]
		if( !any(coordsLevel - saturate(coordsLevel)) )
			break;
	}

	[flatten]
	if(level <= vTraceData.maxLevel)
	{
		coordsLevel.xy *= float2(vTraceData.xVolumeSizeRcp, VOXEL_FACE_COUNT_RCP);
		coordsLevel.xy += vData[level].levelOffsetTex;
				
		float dirWeight[3];
		dirWeight[0] = normal.x * normal.x;
		dirWeight[1] = normal.y * normal.y;
		dirWeight[2] = normal.z * normal.z;

		float faces[3];
		faces[0] = (normal.x >= 0) ? 0 : 1;
		faces[1] = (normal.y >= 0) ? 2 : 3;
		faces[2] = (normal.z >= 0) ? 4 : 5;
		faces[0] *= VOXEL_FACE_COUNT_RCP;
		faces[1] *= VOXEL_FACE_COUNT_RCP;
		faces[2] *= VOXEL_FACE_COUNT_RCP;
		
		float4 diffuse = 0;
		[unroll]
		for(int f = 0; f < 3; f++)
		{
			float3 faceCoords = coordsLevel;
			faceCoords.y += faces[f];
			diffuse += lightVolume.SampleLevel(samp, faceCoords, 0) * dirWeight[f];
		}

		result.rgb = diffuse.rgb * diffuseBrdf;
		result.a = saturate(1 - diffuse.a);
	}

	return result;
}

// TEMP
float3 GetIndirectBrdfVoxel(float3 albedo)
{
	// for NoV = 1, R = 0.5 diffuse brdf ~= 0.82
	return albedo * INV_PI;// * 0.82;
}

float3 CalcutaleDistantProbVoxel(sampler cubeSampler, TextureCube cubemap, float3 normal, float3 diffuseBrdf)
{
	float3 diffuse = cubemap.SampleLevel(cubeSampler, normal, 0).rgb;
	diffuse *= diffuseBrdf; 
	return diffuse;
}