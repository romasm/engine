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
//SamplerState samplerBilinearWrap : register(s2);
SamplerState samplerTrilinearWrap : register(s2);
SamplerState samplerBilinearVolumeClamp : register(s3);

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

Texture3D g_giVolume : register(t13);

Texture2DArray <float> shadows: register(t14); 

StructuredBuffer<SpotLightBuffer> g_spotLightBuffer : register(t15); 
StructuredBuffer<DiskLightBuffer> g_diskLightBuffer : register(t16); 
StructuredBuffer<RectLightBuffer> g_rectLightBuffer : register(t17); 

StructuredBuffer<SpotCasterBuffer> g_spotCasterBuffer : register(t18); 
StructuredBuffer<DiskCasterBuffer> g_diskCasterBuffer : register(t19); 
StructuredBuffer<RectCasterBuffer> g_rectCasterBuffer : register(t20); 

StructuredBuffer<PointLightBuffer> g_pointLightBuffer : register(t21); 
StructuredBuffer<SphereLightBuffer> g_sphereLightBuffer : register(t22); 
StructuredBuffer<TubeLightBuffer> g_tubeLightBuffer : register(t23); 

StructuredBuffer<PointCasterBuffer> g_pointCasterBuffer : register(t24); 
StructuredBuffer<SphereCasterBuffer> g_sphereCasterBuffer : register(t25); 
StructuredBuffer<TubeCasterBuffer> g_tubeCasterBuffer : register(t26); 

StructuredBuffer<DirLightBuffer> g_dirLightBuffer : register(t27);     

StructuredBuffer<int> g_lightIDs : register(t28); 

TextureCubeArray <float4> g_hqEnvProbsArray: register(t29); 
TextureCubeArray <float4> g_sqEnvProbsArray: register(t30); 
TextureCubeArray <float4> g_lqEnvProbsArray: register(t31); 

StructuredBuffer <EnvProbRenderData> g_hqEnvProbsData: register(t32); 
StructuredBuffer <EnvProbRenderData> g_sqEnvProbsData: register(t33); 
StructuredBuffer <EnvProbRenderData> g_lqEnvProbsData: register(t34); 

cbuffer configBuffer : register(b1)
{
	ConfigParams configs;     
};

cbuffer lightsCount : register(b2)
{
	LightsCount g_lightCount;
};

cbuffer giData : register(b3)
{
	GISampleData g_giSampleData;
};   

#include "../common/ibl_helpers.hlsl"         
#include "../common/sg_helpers.hlsl"         
  
// TEMP       
//#define TEMP_FAST_COMPILE     
 
#include "../common/shadow_helpers.hlsl"
#include "../system/direct_brdf.hlsl"   
#define FULL_LIGHT
#include "../common/light_helpers.hlsl"
 
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
	    
	gbuffer.subsurfTint = lerp(1.0, gbuffer.subsurf, materialParams.ssTint);  
	      
	const float4 SSR = SSRTexture.SampleLevel(samplerPointClamp, coords, 0);
	const float SceneAO = DynamicAO.SampleLevel(samplerPointClamp, coords, 0).r;
	gbuffer.ao = min( SceneAO, gbuffer.ao );
	 
	float3 ViewVector = g_CamPos - gbuffer.wpos;     
	const float linDepth = length(ViewVector);
	ViewVector = ViewVector / linDepth;   
	   
	DataForLightCompute mData = PrepareDataForLight(gbuffer, ViewVector);  
	   
	float SO = computeSpecularOcclusion(mData.NoV, gbuffer.ao, mData.minR);    
	           
	// DIRECT LIGHT                 
	LightComponents directLight = ProcessLights(samplerPointClamp, shadows, gbuffer, mData, materialParams, ViewVector, linDepth);
	             
	// IBL     	
	float3 specularBrdf = 0;
	float3 diffuseBrdf = 0;
	float4 envProbSpecular = 0;
	float4 envProbDiffuse = 0;
	EvaluateEnvProbSpecular(samplerTrilinearWrap, mData.NoV, mData.minR, ViewVector, gbuffer, SO, specularBrdf, diffuseBrdf, envProbSpecular, envProbDiffuse);

	// SG
	float4 sgGI = EvaluateSGIndirect(gbuffer);

	// SSR 
	float4 specularSecond = float4( ( SSR.rgb * specularBrdf ) * SSR.a, 1 - SSR.a );
	   
	// OUTPUT  
	// temp, move somewhere
	float scatteringBlendFactor = saturate(luminance(gbuffer.albedo) + float(materialParams.ior == 0.0));

	//indirectLight.diffuse = lerp(indirectLight.scattering, indirectLight.diffuse, scatteringBlendFactor);
	directLight.diffuse = lerp(directLight.scattering, directLight.diffuse, scatteringBlendFactor);

	float3 diffuse = (sgGI.rgb + envProbDiffuse.rgb) * configs.indirDiff + directLight.diffuse * configs.dirDiff;
	float3 specular = envProbSpecular.rgb * configs.indirSpec + directLight.specular * configs.dirSpec;

	//diffuse = lerp(diffuse, g_lightCount.envProbsCountHQ, 0.999);

	diffuseOutput[threadID.xy] = float4( gbuffer.emissive + diffuse, specularSecond.r);
	specularFirstOutput[threadID.xy] = float4( specular, specularSecond.g);
	specularSecondOutput[threadID.xy] = specularSecond.ba; 
}                            
                                                               