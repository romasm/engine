#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"

#include "../common/common_helpers.hlsl"
#include "../common/light_structs.hlsl"
#include "../common/shadow_helpers.hlsl"
#include "../system/direct_brdf.hlsl"   
#include "../common/light_helpers.hlsl"

#include "../common/voxel_helpers.hlsl"
 
#include "pixel_input.hlsl"

SamplerState samplerTrilinearWrap : register(s0);
SamplerState samplerPointClamp : register(s1);

RWTexture3D <uint> emittanceVolume : register(u1);  

Texture2DArray <float> shadowsAtlas : register(t9);   

StructuredBuffer<SpotVoxelBuffer> spotLightInjectBuffer : register(t10); 
StructuredBuffer<PointVoxelBuffer> pointLightInjectBuffer : register(t11); 
StructuredBuffer<DirVoxelBuffer> dirLightInjectBuffer : register(t12); 
  
cbuffer matrixBuffer : register(b3)
{
	StmInstanceMatrix matrixPerInstance[VCT_MESH_MAX_INSTANCE];
}; 

cbuffer volumeMatBuffer : register(b4)
{
	matrix volumeVP[VCT_CLIPMAP_COUNT_MAX][3];
};

cbuffer volumeBuffer : register(b5)
{
	VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX];
};
 
cbuffer levelData : register(b6)
{
	uint currentLevel;
	float _padding00;
	float _padding01;
	float _padding02;
};

cbuffer lightCountBuffer : register(b7)
{
	uint spotCount;
	uint pointCount;
	uint dirCount;
	uint _padding;
};

#define MAX_WAITING_CYCLES 10
void InterlockedFloatAdd(uint3 coords, float value)
{
	uint comp;
	uint orig = emittanceVolume[coords];
	int iter = 0;
	[allow_uav_condition]
	do
	{
		comp = orig;
		const float newValue = asfloat(orig) + value;
		InterlockedCompareExchange(emittanceVolume[coords], comp, asuint(newValue), orig);
		iter++;
	}
	while(orig != comp && iter < MAX_WAITING_CYCLES);
}

void WriteVoxel(float3 emittance, int3 coords, bool front, int planeId)
{
	coords.y += volumeData[0].volumeRes * 2 * planeId;
	coords.xy += volumeData[currentLevel].levelOffset;
	if(!front)
		coords.y += volumeData[0].volumeRes;
	
	coords.x *= 4;

	InterlockedFloatAdd(coords, emittance.r);
	coords.x += 1;
	InterlockedFloatAdd(coords, emittance.g);
	coords.x += 1;
	InterlockedFloatAdd(coords, emittance.b);
	coords.x += 1;
	InterlockedFloatAdd(coords, 1.0);
}

// pixel
void VoxelizationOpaquePS(PI_Mesh_Voxel input, bool front: SV_IsFrontFace, uint subsampleIndex : SV_SampleIndex, 
						   uint subsampleCoverage : SV_Coverage, uint instID : SV_InstanceID)
{
	const uint cover = (1 << subsampleIndex) & subsampleCoverage;
	if( cover == 0 )
		discard;

	// material data
	const bool opaque = AlphatestCalculate(samplerTrilinearWrap, input.tex);
	if(!opaque)
		discard;

	float3 albedo = AlbedoCalculate(samplerTrilinearWrap, input.tex);
	const float3 normal = NormalCalculate(samplerTrilinearWrap, input.tex, input.normal, input.tangent, input.binormal, matrixPerInstance[instID].normalMatrix );
	const float3 emissive = EmissiveCalculate(samplerTrilinearWrap, input.tex);
	const float3 specular = ReflectivityCalculate(samplerTrilinearWrap, input.tex, albedo);
	albedo = max(albedo, specular);
		
	// lighting 
	const float shadowBias = volumeData[currentLevel].voxelSize * 0.5;
	const float3 emittance = ProcessLightsVoxel(samplerPointClamp, shadowsAtlas, shadowBias, albedo, normal, emissive, input.worldPosition, 
		spotLightInjectBuffer, (int)spotCount, pointLightInjectBuffer, (int)pointCount, dirLightInjectBuffer, (int)dirCount);
		
	// output
	int3 uavCoords = int3(input.voxelCoords.xyz);

	float3 shiftedCoords = input.voxelCoords.xyz;
	if(input.planeId == 0)
		shiftedCoords.x += front ? input.trisRadiusSq : -input.trisRadiusSq;
	else if(input.planeId == 1)
		shiftedCoords.y += front ? input.trisRadiusSq : -input.trisRadiusSq;
	else if(input.planeId == 2)
		shiftedCoords.z += front ? input.trisRadiusSq : -input.trisRadiusSq;

	int3 uavShiftedCoords = int3(shiftedCoords);

	[branch]
	if( any(uavShiftedCoords - uavCoords) && uavShiftedCoords.x >= 0 && uavShiftedCoords.x < (int)volumeData[currentLevel].volumeRes &&
		uavShiftedCoords.y >= 0 && uavShiftedCoords.y < (int)volumeData[currentLevel].volumeRes &&
		uavShiftedCoords.z >= 0 && uavShiftedCoords.z < (int)volumeData[currentLevel].volumeRes )
	{
		WriteVoxel(emittance, uavShiftedCoords, front, input.planeId);
	}

	WriteVoxel(emittance, uavCoords, front, input.planeId);

	discard;
}

#define RCP3 1.0 / 3.0

// geometry
[instance(3)] 
//[maxvertexcount(18)]
[maxvertexcount(3)]
void VoxelizationGS( triangle GI_Mesh input[3], inout TriangleStream<PI_Mesh_Voxel> outputStream, uint instanceId : SV_GSInstanceID )
{
	PI_Mesh_Voxel output = (PI_Mesh_Voxel)0;

	output.planeId = instanceId; 
	//output.level = currentLevel;

	const float3 trisRadiusVect = input[0].position.xyz - (input[0].position.xyz + input[1].position.xyz + input[2].position.xyz) * RCP3;
	output.trisRadiusSq = min(length(trisRadiusVect) * volumeData[currentLevel].voxelSizeRcp, 1.0);

	[unroll]
	for ( int i = 0; i < 3; i++ )
	{ 
		output.worldPosition = input[i].position.xyz;
		output.voxelCoords.xyz = (output.worldPosition - volumeData[currentLevel].cornerOffset) * volumeData[currentLevel].scaleHelper;
		output.voxelCoords.w = 1.0f;
		output.position = mul(float4(output.worldPosition, 1.0f), volumeVP[currentLevel][instanceId]);
		
		output.tex = input[i].tex;
		output.normal = input[i].normal; 
		output.tangent = input[i].tangent; 
		output.binormal = input[i].binormal;  

		outputStream.Append( output );
	}
	outputStream.RestartStrip(); 
	/*
	[loop]
	for( int level = currentLevel - 1; level <= (int)volumeData[currentLevel].maxLevel; level++ )
	{
		output.level = level; 

		[unroll]
		for ( int i = 0; i < 3; i++ )
		{ 
			output.voxelCoords.xyz = (input[i].position.xyz - volumeData[level].cornerOffset) * volumeData[level].scaleHelper * 0.5f;
			output.voxelCoords.xyz += volumeData[level].volumeOffset;
			output.voxelCoords.w = 1.0f;
			output.position = mul(float4(input[i].position, 1.0f), volumeVP[level][instanceId]);
					
			outputStream.Append( output );
		}
		outputStream.RestartStrip(); 
	}*/
}