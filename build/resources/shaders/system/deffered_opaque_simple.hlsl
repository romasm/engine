TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "DefferedLighting";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"
#include "light_constants.hlsl"

// from c++
StructuredBuffer<MaterialParamsStructBuffer> MAT_PARAMS : register(t0);

// from material configs
//Texture2D noiseTex : register(t1);  
//#define noiseResInv 1.0/512
Texture2D envbrdfLUT : register(t1);
#define DFG_TEXTURE_SIZE 256
#define NOV_MIN 0.5f/DFG_TEXTURE_SIZE
#define NOV_MAX 1.0f - NOV_MIN
 
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
SamplerState samplerBilinearVolumeClamp : register(s4);

#include "indirect_brdf.hlsl"

cbuffer camMove : register(b1)
{
	float4x4 viewProjInv_ViewProjPrev;
};

cbuffer materialBuffer : register(b2)
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

struct PO_final
{
    float4 diffuse : SV_TARGET0;
	float4 specular : SV_TARGET1;
	float2 specularMore : SV_TARGET2;
};

PO_final DefferedLighting(PI_PosTex input)
{
	PO_final res;
	res.diffuse = 0;
	res.specular = 0;
	res.specularMore = 0;
	
	int2 pixCoords = 0;
	pixCoords.x = int(g_screenW * input.tex.x);
	pixCoords.y = int(g_screenH * input.tex.y);
	
	const float4 albedo_roughY = gb_albedo_roughY.Sample(samplerPointClamp, input.tex);
	const float4 TBN = gb_tbn.Sample(samplerPointClamp, input.tex);
	
	float3 vertex_normal;
	vertex_normal.xy = gb_vnXY.Sample(samplerPointClamp, input.tex).xy;
	
	const float4 spec_roughX = gb_spec_roughX.Sample(samplerPointClamp, input.tex);
	const float4 emiss_vnZ = gb_emiss_vnZ.Sample(samplerPointClamp, input.tex);
	vertex_normal.z = emiss_vnZ.a;
	
	const uint matID_objID = gb_mat_obj.Load(int3(pixCoords, 0));
	const float4 subsurf_thick = gb_subs_thick.Sample(samplerPointClamp, input.tex);
	
	float ao = min(gb_ao.Sample(samplerPointClamp, input.tex).r, dynamicAO.Sample(samplerPointClamp, input.tex).r);
	//ao *= ao;

	const float depth = gb_depth.Sample(samplerPointClamp, UVforSamplePow2(input.tex)).r;
	
	const uint matID = GetMatID(matID_objID);
	const uint objID = GetObjID(matID_objID);
	
	const float3 wpos = GetWPos(input.tex, depth);
	
	MaterialParamsStructBuffer params = MAT_PARAMS[matID];
	 
	float3 normal;
	float3 tangent;   
	float3 binormal;
	DecodeTBNfromFloat4(tangent, binormal, normal, TBN);
			
	float3 albedo = albedo_roughY.rgb;
	float3 spec = spec_roughX.rgb;
	float3 emissive = emiss_vnZ.rgb;
	
	float2 wildRoughnessXY;
	wildRoughnessXY.x = spec_roughX.a;
	wildRoughnessXY.y = albedo_roughY.a;
	//wildRoughnessXY = PowAbs(wildRoughnessXY, ROUGHNESS_REMAP);
	
	float3 subsurf = subsurf_thick.rgb;
		
	if(params.unlit == 1)
	{  
		res.diffuse.rgb = emissive;
		return res;
	}
		
	float3 VtoWP = g_CamPos - wpos;
	VtoWP = normalize(VtoWP);	
	
	// ----------------- INDIRECT -------------------------
	LightCalcOutput Indir;
	Indir.diffuse = 0;
	Indir.specular = 0;
	
	float indirR = clamp( min(wildRoughnessXY.x, wildRoughnessXY.y) ,0.0001f,0.9999f);
	float sqrtR = sqrt(indirR);
		
	float indirNoV = saturate( dot(normal, VtoWP) + 0.00001f );
	indirNoV = clamp(indirNoV, NOV_MIN, NOV_MAX);
	
	float3 specNormal = normal;
	if(wildRoughnessXY.x != wildRoughnessXY.y)
	{
		float anisotropy = wildRoughnessXY.x - wildRoughnessXY.y;
		
		float3 anisotropicBinormal;
		if(anisotropy>0) 
			anisotropicBinormal = binormal;
		else 
			anisotropicBinormal = tangent;
			
		anisotropy = clamp(PowAbs(anisotropy, ANISOTROPY_REFL_REMAP), 0, 0.9);
		float3 anisotropicTangent = cross(-VtoWP, anisotropicBinormal);
		float3 anisotropicNormal = normalize(cross(anisotropicTangent, anisotropicBinormal));
		specNormal = normalize(lerp(specNormal, anisotropicNormal, anisotropy));
	}
	
	float3 envBrdf = envbrdfLUT.SampleLevel( samplerBilinearClamp, float2(indirNoV, indirR), 0).xyz;
	float3 specBrdf = spec * envBrdf.x + saturate(50.0 * spec.g) * envBrdf.y;
	float3 diffBrdf = albedo * envBrdf.z;
	
	float4 ssr = ssr_buf.Sample(samplerPointClamp, input.tex);

	Indir.specular = skyEnvProbSpec(specNormal, VtoWP, indirNoV, indirR, sqrtR, distMip, envprobsDist, envprobsDistDiff, vertex_normal);

	float SO = computeSpecularOcclusion(indirNoV, ao, indirR);
	Indir.specular *= specBrdf * SO;
	
	float4 specSecond;
	specSecond.rgb = (ssr.rgb * SO) * ssr.a;
	specSecond.a = 1 - ssr.a;
	specSecond.rgb *= specBrdf;

	Indir.diffuse = skyEnvProbDiff(normal, VtoWP, indirNoV, indirR, envprobsDistDiff);
	Indir.diffuse *= diffBrdf * ao; 
		   
	/*if(params.subscattering != 0)
	{
		res_diff.rgb += indirectSubScattering(subsurf.rgb, params, normal, VtoWP, ao, 0, envprobsDistDiff, 2, envprobsDist);
	}*/   
	

	res.diffuse.rgb = (emissive + Indir.diffuse);// * indirDiff;
	res.specular.rgb = Indir.specular;// * indirSpec;
	
	res.diffuse.rgb += subsurf.rgb;

	res.diffuse.a = specSecond.r;
	res.specular.a = specSecond.g;
	res.specularMore.rg = specSecond.ba;

	return res; 
}
