#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"

#include "pixel_input.hlsl"

SamplerState samplerAnisotropicWrap : register(s0);

PO_Gbuffer OpaquePS(PI_Mesh input, bool front: SV_IsFrontFace)
{
	if(!AlphatestSample(samplerAnisotropicWrap, input.tex))
		discard;
	
	float3 albedo = AlbedoSample(samplerAnisotropicWrap, input.tex);
	float3 normal = NormalSample(samplerAnisotropicWrap, input.tex, input.normal, input.tangent, input.binormal );
	float2 roughtness = RoughnessSample(samplerAnisotropicWrap, input.tex);
	float3 specular = SpecularSample(samplerAnisotropicWrap, input.tex, albedo);
	float3 emissive = EmissiveSample(samplerAnisotropicWrap, input.tex);
	float ao = AOSample(samplerAnisotropicWrap, input.tex);
	float4 subsurface = SSSSample(samplerAnisotropicWrap, input.tex);
	
	// normal final
	if(!front)normal = -normal;
	float3 nTangent = normalize(cross(normal, cross(input.tangent, normal)));
	
	float3 vnorm = normalize(cross(ddx(input.worldPos.xyz), ddy(input.worldPos.xyz)));
	
	PO_Gbuffer res;
	res.albedo_roughY = float4(albedo, roughtness.y);
	res.tbn = EncodeTBNasFloat4(normal, nTangent);
	res.vnormXY = vnorm.xy;
	res.spec_roughX = float4(specular, roughtness.x);
	res.emiss_vnormZ = float4(emissive, vnorm.z);
	res.id = iddata.x;
	res.subs_thick = float4(subsurface.rgb * saturate(1 - subsurface.a), 0); // a - unused
	res.ao = ao;
	
	return res;
}

// shadow
void AlphatestShadowPS(PI_Mesh input)
{
	if(!AlphatestSample(samplerAnisotropicWrap, input.tex))
		discard;
}