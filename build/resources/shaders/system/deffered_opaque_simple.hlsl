#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"
#include "../common/ibl_helpers.hlsl"
#include "../common/common_helpers.hlsl"
#include "light_constants.hlsl"

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

	// SAMPLE
	const float4 albedo_roughY_Sample = gb_albedo_roughY.SampleLevel(samplerPointClamp, coords, 0);
	const float4 TBN = gb_tbn.SampleLevel(samplerPointClamp, coords, 0);
	const float2 vertex_normal_Sample = gb_vnXY.SampleLevel(samplerPointClamp, coords, 0).xy;
	const float4 spec_roughX_Sample = gb_spec_roughX.SampleLevel(samplerPointClamp, coords, 0);
	const float4 emiss_vnZ_Sample = gb_emiss_vnZ.SampleLevel(samplerPointClamp, coords, 0);
	const float4 subsurf_thick_Sample = gb_subs_thick.SampleLevel(samplerPointClamp, coords, 0);
	const float materiaAO_Sample = gb_ao.SampleLevel(samplerPointClamp, coords, 0).r;
	const float sceneAO_Sample = dynamicAO.SampleLevel(samplerPointClamp, coords, 0).r;
	const float depth = gb_depth.SampleLevel(samplerPointClamp, UVforSamplePow2(coords), 0).r;
	const float4 SSR = ssr_buf.SampleLevel(samplerPointClamp, coords, 0);
	
	const uint matID_objID = gb_mat_obj.Load(int3(threadID.x, threadID.y, 0));
	const uint matID = GetMatID(matID_objID);
	const uint objID = GetObjID(matID_objID);
	MaterialParamsStructBuffer materialParams = MAT_PARAMS[matID];

	// PREPARE DATA
	float3 vertex_normal = float3(vertex_normal_Sample, emiss_vnZ_Sample.a);
	const float3 wpos = GetWPos(coords, depth);
	
	float AO = min( materiaAO_Sample, sceneAO_Sample );	
	
	float3 normal;
	float3 tangent;   
	float3 binormal;
	DecodeTBNfromFloat4(tangent, binormal, normal, TBN);
			
	float3 albedo = albedo_roughY_Sample.rgb;
	float3 spec = spec_roughX_Sample.rgb;
	float3 emissive = emiss_vnZ_Sample.rgb;
	float3 subsurf = subsurf_thick_Sample.rgb;
	
	float2 wildRoughnessXY = float2(spec_roughX_Sample.a, albedo_roughY_Sample.a);
	
	if(materialParams.unlit == 1)
	{  
		diffuseOutput[threadID.xy] = float4(emissive, 0);
		return;
	}
		
	float3 ViewVector = normalize(g_CamPos - wpos);
		
	// IBL
	float NoV = calculateNoV( normal, ViewVector );
	float3 specularNormal = calculateAnisotropicNormal(wildRoughnessXY, normal, binormal, tangent, ViewVector);
		
	float Roughness = clamp( min(wildRoughnessXY.x, wildRoughnessXY.y), 0.0001f, 0.9999f);
	
	float3 envBrdf = envbrdfLUT.SampleLevel( samplerBilinearClamp, float2(NoV, Roughness), 0).xyz;
	float3 specularBrdf = spec * envBrdf.x + saturate(50.0 * spec.g) * envBrdf.y;
	float3 diffuseBrdf = albedo * envBrdf.z;
	
	// SPECULAR
	float3 specular = distantProbSpecular(samplerTrilinearWrap, envprobsDist, samplerBilinearWrap, envprobsDistDiff,
		specularNormal, ViewVector, NoV, Roughness, sqrt(Roughness), distMip, vertex_normal);

	float SO = computeSpecularOcclusion(NoV, AO, Roughness);
	specular *= specularBrdf * SO;
	
	float4 specularSecond = float4( (SSR.rgb * SO) * SSR.a, 1 - SSR.a );
	specularSecond.rgb *= specularBrdf;

	// DIFFUSE
	float3 diffuse = distantProbDiffuse(samplerBilinearWrap, envprobsDistDiff, normal, ViewVector, NoV, Roughness);
	diffuse *= diffuseBrdf * AO; 
	
	/*if(params.subscattering != 0)
	{
		res_diff.rgb += indirectSubScattering(subsurf.rgb, params, normal, ViewVector, ao, 0, envprobsDistDiff, 2, envprobsDist);
	}*/   
	// temp
	if(subsurf_thick_Sample.a == 0.111f)
		diffuse += subsurf.rgb * subsurf_thick_Sample.a;

	// OUTPUT
	diffuseOutput[threadID.xy] = float4( (emissive + diffuse) * indirDiff, specularSecond.r);
	specularFirstOutput[threadID.xy] = float4( specular * indirSpec, specularSecond.g);
	specularSecondOutput[threadID.xy] = specularSecond.ba; 
}
