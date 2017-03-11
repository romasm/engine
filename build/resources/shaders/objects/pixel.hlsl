#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"
 
#include "pixel_input.hlsl"

SamplerState samplerAnisotropicWrap : register(s0);

cbuffer matrixBuffer : register(b3)
{ 
	matrix worldMatrix;
	matrix normalMatrix;  
};     
            
PO_Gbuffer OpaquePS(PI_Mesh input, bool front: SV_IsFrontFace)
{
	if(!AlphatestCalculate(samplerAnisotropicWrap, input.tex))
		discard;
	 
	float3 albedo = AlbedoCalculate(samplerAnisotropicWrap, input.tex);
	float3 normal = NormalCalculate(samplerAnisotropicWrap, input.tex, input.normal, input.tangent, input.binormal, normalMatrix);
	float2 roughtness = RoughnessCalculate(samplerAnisotropicWrap, input.tex);
	float3 reflectivity = ReflectivityCalculate(samplerAnisotropicWrap, input.tex, albedo);
	float3 emissive = EmissiveCalculate(samplerAnisotropicWrap, input.tex);
	float ao = AOCalculate(samplerAnisotropicWrap, input.tex);
	float4 subsurface;
	subsurface.rgb = SubsurfaceCalculate(samplerAnisotropicWrap, input.tex);
	subsurface.a = ThicknessCalculate(samplerAnisotropicWrap, input.tex);
	  
	// normal final 
	if(!front)normal = -normal;
	float3 nTangent = normalize(cross(normal, cross(input.tangent, normal)));
	
	float3 vnorm = normalize(cross(ddx(input.worldPos.xyz), ddy(input.worldPos.xyz)));
	
	PO_Gbuffer res;
	res.albedo_roughY = float4(albedo, roughtness.y);
	res.tbn = EncodeTBNasFloat4(normal, nTangent);
	res.vnormXY = vnorm.xy;
	res.spec_roughX = float4(reflectivity, roughtness.x);
	res.emiss_vnormZ = float4(emissive, vnorm.z);
	res.id = iddata.x;
	res.subs_thick = subsurface;
	res.ao = ao;
	
	return res;
}

// shadow
void AlphatestShadowPS(PI_Mesh input)
{
	if(!AlphatestCalculate(samplerAnisotropicWrap, input.tex))
		discard;
}