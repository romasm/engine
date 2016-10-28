#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"

#include "pixel_input.hlsl"

SamplerState samplerTrilinearWrap : register(s0);

RWTexture3D <uint> opacityVolume : register(u1);  

float VoxelizationOpaquePS(PI_Mesh input, bool front: SV_IsFrontFace) : SV_TARGET
{
	if(!AlphatestSample(samplerTrilinearWrap, input.tex))
		discard;

	float3 albedo = AlbedoSample(samplerTrilinearWrap, input.tex);
	float3 normal = NormalSample(samplerTrilinearWrap, input.tex, input.normal, input.tangent, input.binormal );
	float3 emissive = EmissiveSample(samplerTrilinearWrap, input.tex);

	float3 specular = SpecularSample(samplerTrilinearWrap, input.tex, albedo);
	albedo = max(albedo, specular);
		
	// normal final
	normal = normalize(normal);
	if(!front)normal = -normal;
	
	// to voxel!!!!!
	// albedo, normal, emissive

	uint3 uavCoords = uint3(input.worldPos.xyz);

	if(!front)
		discard;

	//opacityVolume[uavCoords] += 1;
	uint bitShift = (uavCoords.x % 4) * 8;
	uavCoords.x /= 4;
	InterlockedAdd( opacityVolume[uavCoords], 1 << bitShift );
	
	discard;
	return 0.0;
}
