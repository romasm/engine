#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"
#include "../common/voxel_helpers.hlsl"

#include "pixel_input.hlsl"

SamplerState samplerTrilinearWrap : register(s0);

RWTexture3D <uint> opacityVolume : register(u1);  
RWTexture3D <uint> colorVolume0 : register(u2);  
RWTexture3D <uint> colorVolume1 : register(u3);  
RWTexture3D <uint> normalVolume : register(u4);  

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

// pixel
float VoxelizationOpaquePS(PI_Mesh_Voxel input, bool front: SV_IsFrontFace, uint subsampleIndex : SV_SampleIndex, 
						   uint subsampleCoverage : SV_Coverage, uint instID : SV_InstanceID) : SV_TARGET
{
	uint cover = (1 << subsampleIndex) & subsampleCoverage;
	if( cover == 0 )
		discard;

	// material data
	bool opaque = AlphatestSample(samplerTrilinearWrap, input.tex);
	if(!opaque)
		discard;

	float3 albedo = AlbedoCalculate(samplerTrilinearWrap, input.tex);

	float3 normal = NormalCalculate(samplerTrilinearWrap, input.tex, input.normal, input.tangent, input.binormal, matrixPerInstance[instID].normalMatrix );
	
	float3 emissive = EmissiveCalculate(samplerTrilinearWrap, input.tex);

	float3 specular = ReflectivityCalculate(samplerTrilinearWrap, input.tex, albedo);
	albedo = max(albedo, specular);
		
	// emittance prepare
	float emissiveLum = length(emissive);
	emissive /= emissiveLum;
	float4 emittance = emissiveLum > 0.00001 ? float4(emissive, emissiveLum) : float4(albedo, 0.0);
	emittance = float4(saturate(emittance.x) * 255, saturate(emittance.y) * 255, 
		saturate(emittance.z) * 255, saturate(emittance.w / 100.0f) * 255);
	uint2 emitValue = uint2( (uint(emittance.x) << 16) + uint(emittance.y), (uint(emittance.z) << 16) + uint(emittance.w) );

	// normal prepare
	normal = normalize(normal);
	normal = (normal + 1.0f) * 0.5f;

	uint3 voxelNormal;
	voxelNormal.x = normal.x * 255;
	voxelNormal.y = normal.y * 255;
	voxelNormal.z = normal.z * 255;

	// coords 
	uint3 uavCoords = uint3(input.voxelCoords.xyz);
	uavCoords.y += volumeData[0].volumeRes * 2 * input.planeId;
	uavCoords.x += volumeData[0].volumeRes * currentLevel;

	if(!front)
		uavCoords.y += volumeData[0].volumeRes;
	
	// write
	InterlockedAdd( opacityVolume[uavCoords], (voxelNormal.z << 16) + 1 );
	
	InterlockedAdd( colorVolume0[uavCoords], emitValue.x );
	InterlockedAdd( colorVolume1[uavCoords], emitValue.y );

	InterlockedAdd( normalVolume[uavCoords], (voxelNormal.x << 16) + voxelNormal.y );

	discard;
	return 0.0;
}

// geometry
[instance(3)] 
//[maxvertexcount(18)]
[maxvertexcount(3)]
void VoxelizationGS( triangle GI_Mesh input[3], inout TriangleStream<PI_Mesh_Voxel> outputStream, uint instanceId : SV_GSInstanceID )
{
	PI_Mesh_Voxel output = (PI_Mesh_Voxel)0;

	output.planeId = instanceId; 
	//output.level = currentLevel;

	[unroll]
	for ( int i = 0; i < 3; i++ )
	{ 
		output.voxelCoords.xyz = (input[i].position.xyz - volumeData[currentLevel].cornerOffset) * volumeData[currentLevel].scaleHelper;
		output.voxelCoords.w = 1.0f;
		output.position = mul(float4(input[i].position, 1.0f), volumeVP[currentLevel][instanceId]);
		
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