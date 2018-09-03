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

TextureCubeArray <float4> g_hqEnvProbsArray: register(t13);
TextureCubeArray <float4> g_sqEnvProbsArray: register(t14);
TextureCubeArray <float4> g_lqEnvProbsArray: register(t15);

StructuredBuffer <EnvProbRenderData> g_hqEnvProbsData: register(t16);
StructuredBuffer <EnvProbRenderData> g_sqEnvProbsData: register(t17);
StructuredBuffer <EnvProbRenderData> g_lqEnvProbsData: register(t18);


cbuffer configBuffer : register(b1)
{
	ConfigParams configs;
};

cbuffer lightsCount : register(b2)
{
	LightsCount g_lightCount;
};

#include "../common/ibl_helpers.hlsl" 

[numthreads( GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, 1 )]
void DefferedLightingIBL(uint3 threadID : SV_DispatchThreadID)
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
	
	float3 ViewVector = normalize(g_CamPos - gbuffer.wpos);
	DataForLightCompute mData = PrepareDataForLight(gbuffer, ViewVector);

	float SO = computeSpecularOcclusion(mData.NoV, gbuffer.ao, mData.minR);

	// IBL     	
	float3 specularBrdf = 0;
	float3 diffuseBrdf = 0;
	float4 envProbSpecular = 0;
	float4 envProbDiffuse = 0;
	EvaluateEnvProbSpecular(samplerTrilinearWrap, mData, ViewVector, gbuffer, SO, specularBrdf, diffuseBrdf, envProbSpecular, envProbDiffuse);

	// SSR
	float4 specularSecond = float4( SSR.rgb * SO * SSR.a, 1 - SSR.a );
	specularSecond.rgb *= specularBrdf;

	// OUTPUT
	float3 diffuse = envProbDiffuse.rgb * configs.indirDiff;
	float3 specular = envProbSpecular.rgb * configs.indirSpec;

	diffuseOutput[threadID.xy] = float4( gbuffer.emissive + diffuse, specularSecond.r);
	specularFirstOutput[threadID.xy] = float4(specular, specularSecond.g);
	specularSecondOutput[threadID.xy] = specularSecond.ba; 
}
