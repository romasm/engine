
#define VCT_MESH_MAX_INSTANCE 128

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
    1.0f / 4.0f,
    3.0f / 20.0f,
    3.0f / 20.0f,
    3.0f / 20.0f,
    3.0f / 20.0f,
    3.0f / 20.0f,
};

#define VOXEL_CONE_TRACING_STEP 0.5f
#define VOXEL_CONE_TRACING_MAX_STEPS 64

#define VOXEL_LEVEL_COUNT 6
#define VOXEL_LEVEL_COUNT_RCP 1.0f / VOXEL_LEVEL_COUNT

#define VOXEL_FACE_COUNT_RCP 1.0f / 6

float4 VoxelConeTrace(float3 origin, float3 direction, float aperture, float3 surfaceNormal, float voxelDiag, float4 volumeData, 
					  Texture3D <float4> volumeEmittance, SamplerState volumeSampler)
{
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
	const float voxelDiagRcp = rcp(voxelDiag);

	float3 coneStart = origin + surfaceNormal * voxelDiag;

	float maxConeStart = max(coneStart.x, max(coneStart.y, coneStart.z));
	float minConeStart = min(coneStart.x, min(coneStart.y, coneStart.z));
	if(maxConeStart > 10.0f || minConeStart < 0.0f)
		return 0;
	
	float distance = voxelDiag;
	float4 coneColor = 0;
	float level = 0;

	int i = 0;
	while(coneColor.a < 1.0f/* && level < VOXEL_LEVEL_COUNT*/ && i <= VOXEL_CONE_TRACING_MAX_STEPS)
    {
        float3 currentConePos = coneStart + direction * distance;

        float diameter = apertureDouble * distance;
        level = log2(diameter * voxelDiagRcp);
		level = clamp(level, 0.0f, VOXEL_LEVEL_COUNT);
        
        float3 sampleCoords = (currentConePos - volumeData.xyz) * volumeData.w;
		sampleCoords = clamp(sampleCoords, 0, 1);
		/*float maxCoord = max(sampleCoords.x, max(sampleCoords.y, sampleCoords.z));
		float minCoord = min(sampleCoords.x, min(sampleCoords.y, sampleCoords.z));
		if(maxCoord > 1.0f || minCoord < 0.0f)
			break;*/

		sampleCoords.xy *= float2(VOXEL_LEVEL_COUNT_RCP, VOXEL_FACE_COUNT_RCP);

		float levelUpDown[2];
		levelUpDown[0] = ceil(level);
		levelUpDown[1] = floor(level);

		float levelLerp = saturate(level - levelUpDown[1]);
		
		float4 voxelSample[2];
		[unroll]
		for(int voxelLevel = 0; voxelLevel < 2; voxelLevel++)
		{
			float3 coordsLevel = sampleCoords;
			coordsLevel /= exp2(levelUpDown[voxelLevel]);
			coordsLevel.x += VOXEL_LEVEL_COUNT_RCP * levelUpDown[voxelLevel];

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
        coneColor.rgb += (1.0f - coneColor.a) * (finalColor.rgb /* finalColor.a*/);
		coneColor.a += finalColor.a;
        
        distance += diameter * VOXEL_CONE_TRACING_STEP;
		i++;
    }

	coneColor.a = saturate(coneColor.a);
	return coneColor;
}

#define RAY_TRACE_DISTANCE 25.0f
#define RAY_TRACE_EPCILON 0.00001f
#define RAY_TRACE_MAX_I 512

int4 GetVoxelOnRay(float3 origin, float3 ray, float volumeSize, uint volumeRes, uint level, Texture3D <float4> voxelEmittance, out float3 collideWS)
{
	collideWS = 0;

	const float levelOffset = level * volumeRes;
	const uint volumeLevelRes = volumeRes / exp2(level);

	const float3 voxelSize = volumeSize / volumeLevelRes;
	const float3 voxelSizeRcp = 1.0f / voxelSize;
	const float3 boxExtend = float3(volumeSize, volumeSize, volumeSize) * 0.5f;

	ray *= RAY_TRACE_DISTANCE;
	float3 originInBox = origin - boxExtend;

	float2 intersections = RayBoxIntersect( originInBox, ray, -boxExtend, boxExtend );

	if ( intersections.y <= intersections.x )
		return -1;
	
	float3 epcilon = RAY_TRACE_EPCILON;
	float3 step;
	step.x = ray.x >= 0 ? 1 : -1;
	step.y = ray.y >= 0 ? 1 : -1;
	step.z = ray.z >= 0 ? 1 : -1;

	epcilon *= step;

	step = saturate(step);
	step *= voxelSize;
	
	float3 samplePoint = origin + ray * intersections.x + epcilon;
	float3 voxelSnap = 0;
	float3 prevVoxel = 0;
	
	int i = 0;
	[loop]
	while( i < RAY_TRACE_MAX_I )
	{
		voxelSnap = floor(samplePoint * voxelSizeRcp);

		int4 voxelCoords = int4(voxelSnap, 0);
		voxelCoords.x += levelOffset;
		float anyValue = 0;
		[unroll]
		for(int j = 0; j < 6; j++)
		{
			anyValue += voxelEmittance.Load(voxelCoords).w;
			voxelCoords.y += volumeRes;
		}
		
		voxelSnap *= voxelSize;
		prevVoxel = voxelSnap;

		[branch]
		if( anyValue > 0.0f )
			break;

		voxelSnap += step;

		float3 delta = (voxelSnap - origin) / ray;
		float d = min(delta.x, min(delta.y, delta.z));

		[branch]
		if( intersections.y - 0.000001 <= d )
			return -1;

		samplePoint = origin + ray * d + epcilon;
		i++;
	}
	
	[branch]
	if( i == RAY_TRACE_MAX_I )
		return -1;

	float3 voxelExtend = voxelSize * 0.5f;
	float3 originInVoxel = origin - (prevVoxel + voxelExtend);
	float2 voxelIntersections = RayBoxIntersect( originInVoxel, ray, -voxelExtend, voxelExtend );
	collideWS = origin + ray * voxelIntersections.x;
		
	float3 entry = collideWS - prevVoxel;
	entry.x = ray.x < 0 ? (voxelSize.x - entry.x) : entry.x;
	entry.y = ray.y < 0 ? (voxelSize.y - entry.y) : entry.y;
	entry.z = ray.z < 0 ? (voxelSize.z - entry.z) : entry.z;
	float minEntry = min(entry.x, min(entry.y, entry.z));
	
	uint faceID;
	[branch]
	if(minEntry == entry.x) faceID = ray.x < 0 ? 1 : 0;
	else if(minEntry == entry.y) faceID = ray.y < 0 ? 3 : 2;
	else faceID = ray.z < 0 ? 5 : 4;

	prevVoxel *= voxelSizeRcp;
	prevVoxel.y += volumeRes * faceID;
	prevVoxel.x += levelOffset;

	return int4(prevVoxel, 0);	
}