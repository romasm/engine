#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"
#include "light_constants.hlsl"

#define GROUP_THREAD_COUNT 2

RWTexture2D <float4> diffuseOutput : register(u0);  
RWTexture2D <float4> specularFirstOutput : register(u1);  
RWTexture2D <float2> specularSecondOutput : register(u2);  

SamplerState samplerPointClamp : register(s0);
SamplerState samplerBilinearClamp : register(s1);
SamplerState samplerBilinearWrap : register(s2);
SamplerState samplerTrilinearWrap : register(s3);

// GBUFFER
#define GBUFFER_READ

StructuredBuffer<MaterialParams> gb_MaterialParamsBuffer : register(t0);

Texture2D <float4> gb_AlbedoRoughnesY : register(t1); 
Texture2D <float4> gb_TBN : register(t2); 
Texture2D <float2> gb_VertexNormalXY : register(t3); 
Texture2D <float4> gb_ReflectivityRoughnessX : register(t4); 
Texture2D <float4> gb_EmissiveVertexNormalZ : register(t5); 
Texture2D <uint> gb_MaterialObjectID : register(t6); 
Texture2D <float4> gb_SubsurfaceThickness : register(t7); 
Texture2D <float> gb_AmbientOcclusion : register(t8); 
Texture2D <float2> gb_Depth : register(t9);

#include "../common/common_helpers.hlsl"


Texture2D <float> DynamicAO : register(t10); 
Texture2D <float4> SSRTexture : register(t11); 


Texture2D g_envbrdfLUT : register(t12);
TextureCube g_envprobsDist : register(t13); 
TextureCube g_envprobsDistBlurred : register(t14); 

#include "../common/ibl_helpers.hlsl"

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
	const float2 coords = PixelCoordsFromThreadID(threadID.xy);
	GBufferData gbuffer = ReadGBuffer(samplerPointClamp, coords);
	const MaterialParams materialParams = ReadMaterialParams(threadID.xy);

	if(materialParams.unlit == 1)
	{  
		diffuseOutput[threadID.xy] = float4(gbuffer.emissive, 0);
		return;
	}

	const float4 SSR = SSRTexture.SampleLevel(samplerPointClamp, coords, 0);
	const float SceneAO = DynamicAO.SampleLevel(samplerPointClamp, coords, 0).r;
	gbuffer.ao = min( SceneAO, gbuffer.ao );
	
	// IBL
	float3 ViewVector = normalize(g_CamPos - gbuffer.wpos);
	float NoV = calculateNoV( gbuffer.normal, ViewVector );
	float Roughness = clamp( min(gbuffer.roughness.x, gbuffer.roughness.y), 0.0001f, 0.9999f);

	float3 specular, diffuse;
	float3 specularBrdf = CalcutaleDistantProbLight(samplerBilinearClamp, samplerTrilinearWrap, samplerBilinearWrap, 
		NoV, Roughness, ViewVector, gbuffer, distMip, specular, diffuse);
	
	// SSR
	float4 specularSecond = float4( SSR.rgb * SSR.a, 1 - SSR.a );
	specularSecond.rgb *= specularBrdf;
	
	// OUTPUT
	diffuseOutput[threadID.xy] = float4( (gbuffer.emissive + diffuse) * indirDiff, specularSecond.r);
	specularFirstOutput[threadID.xy] = float4( specular * indirSpec, specularSecond.g);
	specularSecondOutput[threadID.xy] = specularSecond.ba; 
}
