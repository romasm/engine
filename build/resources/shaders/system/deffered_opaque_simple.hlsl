#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

#define GROUP_THREAD_COUNT 2

RWTexture2D <float4> diffuseOutput : register(u0);  
RWTexture2D <float4> specularFirstOutput : register(u1);  
RWTexture2D <float2> specularSecondOutput : register(u2);  

StructuredBuffer<MaterialParamsStructBuffer> MAT_PARAMS : register(t0);

Texture2D envbrdfLUT : register(t1);
 
Texture2D <float4> gb_albedo_roughY : register(t2); 
Texture2D <float4> gb_tbn : register(t3); 
Texture2D <float2> gb_vnXY : register(t4); 
Texture2D <float4> gb_spec_roughX : register(t5); 
Texture2D <float4> gb_emiss_vnZ : register(t6); 
Texture2D <uint> gb_mat_obj : register(t7); 
Texture2D <float4> gb_subs_thick : register(t8); 
Texture2D <float> gb_ao : register(t9); 

Texture2D <float2> gb_depth : register(t10);

Texture2D <float> dynamicAO : register(t11); 
Texture2D <float4> ssr_buf : register(t12); 

TextureCube envprobsDist : register(t13); 
TextureCube envprobsDistDiff : register(t14); 

SamplerState samplerPointClamp : register(s0);
SamplerState samplerBilinearClamp : register(s1);
SamplerState samplerBilinearWrap : register(s2);
SamplerState samplerTrilinearWrap : register(s3);

#define GBUFFER_READ
#include "../common/ibl_helpers.hlsl"
#include "../common/common_helpers.hlsl"
#include "light_constants.hlsl"

cbuffer camMove : register(b1)
{
	float4x4 viewProjInv_ViewProjPrev;
};

cbuffer configBuffer : register(b2)
{
	float spot_count;
	float disk_count;
	float rect_count;
	float point_count;

	float sphere_count;
	float tube_count;
	float dir_count;
	float caster_spot_count;

	float caster_disk_count;
	float caster_rect_count;
	float caster_point_count;
	float caster_sphere_count;

	float caster_tube_count;
	float distMip;
	float dirDiff;
	float dirSpec;

	float indirDiff;
	float indirSpec;
	float _padding0;
	float _padding1;
};

[numthreads( GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, 1 )]
void DefferedLightingIBL(uint3 threadID : SV_DispatchThreadID)
{
	float2 coords = pixelCoordsFromThreadID(threadID);
	GBufferData gbuffer = ReadGBuffer(samplerPointClamp, coords);
	MaterialParamsStructBuffer materialParams = ReadMaterialParams(int2(threadID.xy));

	if(materialParams.unlit == 1)
	{  
		diffuseOutput[threadID.xy] = float4(gbuffer.emissive, 0);
		return;
	}
		
	float3 ViewVector = normalize(g_CamPos - gbuffer.wpos);
		
	// IBL
	float NoV = calculateNoV( gbuffer.normal, ViewVector );
	float3 specularNormal = calculateAnisotropicNormal(gbuffer.roughness, gbuffer.normal, gbuffer.binormal, gbuffer.tangent, ViewVector);
		
	float Roughness = clamp( min(gbuffer.roughness.x, gbuffer.roughness.y), 0.0001f, 0.9999f);
	
	float3 envBrdf = envbrdfLUT.SampleLevel( samplerBilinearClamp, float2(NoV, Roughness), 0).xyz;
	float3 specularBrdf = gbuffer.reflectivity * envBrdf.x + saturate(50.0 * gbuffer.reflectivity.g) * envBrdf.y;
	float3 diffuseBrdf = gbuffer.albedo * envBrdf.z;
	
	// SPECULAR
	float3 specular = distantProbSpecular(samplerTrilinearWrap, envprobsDist, samplerBilinearWrap, envprobsDistDiff,
		specularNormal, ViewVector, NoV, Roughness, sqrt(Roughness), distMip, gbuffer.vertex_normal);

	float SO = computeSpecularOcclusion(NoV, gbuffer.ao, Roughness);
	specular *= specularBrdf * SO;
	
	float4 specularSecond = float4( (gbuffer.ssr.rgb * SO) * gbuffer.ssr.a, 1 - gbuffer.ssr.a );
	specularSecond.rgb *= specularBrdf;

	// DIFFUSE
	float3 diffuse = distantProbDiffuse(samplerBilinearWrap, envprobsDistDiff, gbuffer.normal, ViewVector, NoV, Roughness);
	diffuse *= diffuseBrdf * gbuffer.ao; 
	
	/*if(params.subscattering != 0)
	{
		res_diff.rgb += indirectSubScattering(subsurf.rgb, params, normal, ViewVector, ao, 0, envprobsDistDiff, 2, envprobsDist);
	}*/   
	// temp
	if(gbuffer.thickness == 0.111f)
		diffuse += gbuffer.subsurf * gbuffer.thickness;

	// OUTPUT
	diffuseOutput[threadID.xy] = float4( (gbuffer.emissive + diffuse) * indirDiff, specularSecond.r);
	specularFirstOutput[threadID.xy] = float4( specular * indirSpec, specularSecond.g);
	specularSecondOutput[threadID.xy] = specularSecond.ba; 
}
