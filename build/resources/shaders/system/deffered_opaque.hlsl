#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"
#include "../common/light_structs.hlsl"

#define GROUP_THREAD_COUNT 8

RWTexture2D <float4> diffuseOutput : register(u0);  
RWTexture2D <float4> specularFirstOutput : register(u1);  
RWTexture2D <float2> specularSecondOutput : register(u2);  

SamplerState samplerPointClamp : register(s0);
SamplerState samplerBilinearClamp : register(s1);
SamplerState samplerBilinearWrap : register(s2);
SamplerState samplerTrilinearWrap : register(s3);
SamplerState samplerBilinearVolumeClamp : register(s4);

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


Texture2DArray <float> shadows: register(t15); 

Texture3D <float4> volumeEmittance : register(t16); 

StructuredBuffer<SpotLightBuffer> g_spotLightBuffer : register(t17); 
StructuredBuffer<DiskLightBuffer> g_diskLightBuffer : register(t18); 
StructuredBuffer<RectLightBuffer> g_rectLightBuffer : register(t19); 

StructuredBuffer<SpotCasterBuffer> g_spotCasterBuffer : register(t20); 
StructuredBuffer<DiskCasterBuffer> g_diskCasterBuffer : register(t21); 
StructuredBuffer<RectCasterBuffer> g_rectCasterBuffer : register(t22); 

StructuredBuffer<PointLightBuffer> g_pointLightBuffer : register(t23); 
StructuredBuffer<SphereLightBuffer> g_sphereLightBuffer : register(t24); 
StructuredBuffer<TubeLightBuffer> g_tubeLightBuffer : register(t25); 

StructuredBuffer<PointCasterBuffer> g_pointCasterBuffer : register(t26); 
StructuredBuffer<SphereCasterBuffer> g_sphereCasterBuffer : register(t27); 
StructuredBuffer<TubeCasterBuffer> g_tubeCasterBuffer : register(t28); 

StructuredBuffer<DirLightBuffer> g_dirLightBuffer : register(t29); 

cbuffer camMove : register(b1)
{
	float4x4 viewProjInv_ViewProjPrev;
};

cbuffer configBuffer : register(b2)
{
	float g_spot_count;
	float g_disk_count;
	float g_rect_count;
	float g_point_count;

	float g_sphere_count;
	float g_tube_count;
	float g_dir_count;
	float g_caster_spot_count;

	float g_caster_disk_count;
	float g_caster_rect_count;
	float g_caster_point_count;
	float g_caster_sphere_count;

	float g_caster_tube_count;
	float distMip;
	float dirDiff;
	float dirSpec;

	float indirDiff;
	float indirSpec;
	float _padding0; 
	float _padding1; 
};

cbuffer Lights : register(b3) 
{
	LightsIDs g_lightIDs;
};

#include "../common/shadow_helpers.hlsl"
#include "../system/direct_brdf.hlsl"
#define FULL_LIGHT
#include "../common/light_helpers.hlsl"
#include "../common/voxel_helpers.hlsl"

cbuffer volumeBuffer : register(b4)
{
	VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX];
};

[numthreads( GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, 1 )]
void DefferedLighting(uint3 threadID : SV_DispatchThreadID)
{
	const float2 coords = PixelCoordsFromThreadID(threadID.xy);
	[branch]
	if(coords.x > 1.0f || coords.y > 1.0f)
		return;

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
	
	float3 ViewVector = g_CamPos - gbuffer.wpos;
	const float linDepth = length(ViewVector);
	ViewVector = ViewVector / linDepth;
	
	/*// move to forward stage
	// normal fix for gazaring angles TODO
	float NoV = dot(normal, VtoWP);
	float nFix = (-clamp(NoV, -1.0f, NORMAL_CLAMP) * NORMAL_CLAMP_MUL) + 0.5f;    
	normal = normalize(normal + VtoWP * NORMAL_CLAMP_DOUBLE * nFix);
	tangent = normalize(cross(cross(normal, tangent), normal));
	binormal = normalize(cross(cross(normal, binormal), binormal));*/

	DataForLightCompute mData;
	mData.R = gbuffer.roughness;
	mData.R.x = clamp(mData.R.x, 0.001f, 0.9999f);
	mData.R.y = clamp(mData.R.y, 0.001f, 0.9999f);
	mData.avgR = (mData.R.x + mData.R.y) * 0.5;
	mData.minR = min(mData.R.x, mData.R.y);
	mData.aGGX = max(mData.R.x * mData.R.y, 0.1);
	mData.sqr_aGGX = Square( mData.aGGX );
	mData.NoV = calculateNoV( gbuffer.normal, ViewVector );
	mData.reflect = 2 * gbuffer.normal * mData.NoV - ViewVector; 

	float SO = computeSpecularOcclusion(mData.NoV, gbuffer.ao, mData.minR);
	   
	// DIRECT LIGHT
	LightComponents directLight = 
		ProcessLights(samplerPointClamp, shadows, gbuffer, mData, materialParams, ViewVector, linDepth);
	
	// IBL
	float3 specularBrdf, diffuseBrdf;
	LightComponents indirectLight = CalcutaleDistantProbLight(samplerBilinearClamp, samplerTrilinearWrap, samplerBilinearWrap, 
		mData.NoV, mData.minR, ViewVector, gbuffer, distMip, SO, specularBrdf, diffuseBrdf);
	      
	// VCTGI   
	LightComponentsWeight vctLight = CalculateVCTLight(samplerBilinearVolumeClamp, volumeEmittance, volumeData, gbuffer, mData, 
		specularBrdf, diffuseBrdf, SO);

	indirectLight.diffuse = lerp(indirectLight.diffuse, vctLight.diffuse, vctLight.diffuseW);
	indirectLight.specular = lerp(indirectLight.specular, vctLight.specular, vctLight.specularW);
	indirectLight.scattering = lerp(indirectLight.scattering, vctLight.scattering, vctLight.scatteringW);
	
	// SSR
	float4 specularSecond = float4( ( SSR.rgb * specularBrdf * SO ) * SSR.a, 1 - SSR.a );
	
	// OUTPUT
	float3 diffuse = (indirectLight.diffuse + indirectLight.scattering) * indirDiff + 
		(directLight.diffuse + directLight.scattering) * dirDiff;
	float3 specular = indirectLight.specular * indirSpec + directLight.specular * dirSpec;

	diffuseOutput[threadID.xy] = float4( gbuffer.emissive + diffuse, specularSecond.r);
	specularFirstOutput[threadID.xy] = float4( specular, specularSecond.g);
	specularSecondOutput[threadID.xy] = specularSecond.ba;
}
