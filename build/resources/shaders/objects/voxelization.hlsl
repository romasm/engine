#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"

#include "pixel_input.hlsl"

SamplerState samplerTrilinearWrap : register(s0);

RWTexture3D <uint> opacityVolume : register(u1);  
RWTexture3D <uint> colorVolume0 : register(u2);  
RWTexture3D <uint> colorVolume1 : register(u3);  
RWTexture3D <uint> normalVolume : register(u4);  

cbuffer volumeBuffer : register(b4)
{
	matrix volumeVP[3];

	float4 volumeOffsetSize;
	float4 volumeScaleResDir;
};

// pixel
float VoxelizationOpaquePS(PI_Mesh_Voxel input, bool front: SV_IsFrontFace,
						   uint subsampleIndex : SV_SampleIndex, uint subsampleCoverage : SV_Coverage) : SV_TARGET
{
	uint cover = (1 << subsampleIndex) & subsampleCoverage;
	if( cover == 0 )
		discard;

	// material data
	bool opaque = AlphatestSample(samplerTrilinearWrap, input.tex);
	if(!opaque)
		discard;

	float3 albedo = AlbedoSample(samplerTrilinearWrap, input.tex);

	float3 normal = NormalSample(samplerTrilinearWrap, input.tex, input.normal, input.tangent, input.binormal );
	
	float3 emissive = EmissiveSample(samplerTrilinearWrap, input.tex);

	float3 specular = SpecularSample(samplerTrilinearWrap, input.tex, albedo);
	albedo = max(albedo, specular);

	float NoP[3];
	NoP[0] = normal.x;
	NoP[1] = normal.y;
	NoP[2] = normal.z;
	albedo *= abs(NoP[input.planeId]);
	
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
	uint3 uavCoords = uint3(input.worldPos.xyz);
	uavCoords.y += uint(volumeScaleResDir.z) * input.planeId;

	if(!front)
		uavCoords.y += uint(volumeScaleResDir.y);
	
	// write
	InterlockedAdd( opacityVolume[uavCoords], (voxelNormal.z << 16) + 1 );
	
	InterlockedAdd( colorVolume0[uavCoords], emitValue.x );
	InterlockedAdd( colorVolume1[uavCoords], emitValue.y );

	InterlockedAdd( normalVolume[uavCoords], (voxelNormal.x << 16) + voxelNormal.y );

	discard;
	return 0.0;
}

// geomatry
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