#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"

#include "pixel_input.hlsl"

SamplerState samplerTrilinearWrap : register(s0);

RWTexture3D <uint> opacityVolume : register(u1);  

cbuffer volumeBuffer : register(b4)
{
	matrix volumeVP[3];

	float4 volumeOffsetSize;
	float4 volumeScaleResDir;
};

float VoxelizationOpaquePS(PI_Mesh_Voxel input, bool front: SV_IsFrontFace,
						   uint subsampleIndex : SV_SampleIndex, uint subsampleCoverage : SV_Coverage) : SV_TARGET
{
	uint cover = (1 << subsampleIndex) & subsampleCoverage;
	if( cover == 0 )
		discard;

	bool opaque = AlphatestSample(samplerTrilinearWrap, input.tex);
	if(!opaque)
		discard;

	float3 albedo = AlbedoSample(samplerTrilinearWrap, input.tex);
	float3 normal = NormalSample(samplerTrilinearWrap, input.tex, input.normal, input.tangent, input.binormal );
	float3 emissive = EmissiveSample(samplerTrilinearWrap, input.tex);

	float3 specular = SpecularSample(samplerTrilinearWrap, input.tex, albedo);
	albedo = max(albedo, specular);
		
	normal = normalize(normal);
	
	// to voxel!!!!!
	// albedo, normal, emissive

	uint3 uavCoords = uint3(input.worldPos.xyz);
	uavCoords.y += uint(volumeScaleResDir.z) * input.planeId;

	if(!front)
		uavCoords.y += uint(volumeScaleResDir.y);
	
	uint bitShift = (uavCoords.x % 4) * 8;
	uavCoords.x /= 4;
	InterlockedAdd( opacityVolume[uavCoords], 1 << bitShift );
	
	discard;
	return 0.0;
}

[instance(3)]
[maxvertexcount(3)]
void VoxelizationGS( triangle GI_Mesh input[3], inout TriangleStream<PI_Mesh_Voxel> outputStream, uint instanceId : SV_GSInstanceID )
{
	PI_Mesh_Voxel output = (PI_Mesh_Voxel)0;
	for ( uint i = 0; i < 3; i++ )
	{
		output.worldPos.xyz = (input[i].position.xyz - volumeOffsetSize.xyz) * volumeScaleResDir.x;
		output.worldPos.w = 1.0f;
		output.position = mul(float4(input[i].position, 1.0f), volumeVP[instanceId]);
		
		output.tex = input[i].tex;
		output.normal = input[i].normal; 
		output.tangent = input[i].tangent; 
		output.binormal = input[i].binormal; 

		output.planeId = instanceId; 

		outputStream.Append( output );
	}
	outputStream.RestartStrip(); 
}