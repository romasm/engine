#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"
#include "../common/light_structs.hlsl"
#include "../common/common_helpers.hlsl"

SamplerState samplerPointClamp : register(s0);
SamplerState samplerBilinearClamp : register(s1);
SamplerState samplerBilinearWrap : register(s2);
SamplerState samplerTrilinearWrap : register(s3);
SamplerState samplerBilinearVolumeClamp : register(s4);
SamplerState samplerAnisotropicWrap : register(s5);
SamplerState samplerTrilinearMirror : register(s6);//samplerTrilinearMirror

cbuffer matrixBuffer : register(b2)
{
	matrix worldMatrix;
	matrix normalMatrix;  
}; 

Texture2D g_envbrdfLUT : register(t0);
TextureCube g_envprobsDist : register(t1); 
TextureCube g_envprobsDistBlurred : register(t2); 

#include "../common/ibl_helpers.hlsl"


Texture2DArray <float> sys_shadows: register(t3); 

Texture3D <float4> sys_volumeEmittance : register(t4); 

Texture2D <float4> sys_sceneColor : register(t5); 
Texture2D <float2> sys_depth : register(t6);
Texture2D <float> sys_backDepth : register(t7);
Texture2D <float4> sys_backNormal : register(t8);

StructuredBuffer<SpotLightBuffer> g_spotLightBuffer : register(t9); 
StructuredBuffer<DiskLightBuffer> g_diskLightBuffer : register(t10); 
StructuredBuffer<RectLightBuffer> g_rectLightBuffer : register(t11); 

StructuredBuffer<SpotCasterBuffer> g_spotCasterBuffer : register(t12); 
StructuredBuffer<DiskCasterBuffer> g_diskCasterBuffer : register(t13); 
StructuredBuffer<RectCasterBuffer> g_rectCasterBuffer : register(t14); 

StructuredBuffer<PointLightBuffer> g_pointLightBuffer : register(t15); 
StructuredBuffer<SphereLightBuffer> g_sphereLightBuffer : register(t16); 
StructuredBuffer<TubeLightBuffer> g_tubeLightBuffer : register(t17); 

StructuredBuffer<PointCasterBuffer> g_pointCasterBuffer : register(t18); 
StructuredBuffer<SphereCasterBuffer> g_sphereCasterBuffer : register(t19); 
StructuredBuffer<TubeCasterBuffer> g_tubeCasterBuffer : register(t20); 

StructuredBuffer<DirLightBuffer> g_dirLightBuffer : register(t21); 

StructuredBuffer<int> g_lightIDs : register(t22);  // TODO!!!!!

#include "../system/direct_brdf.hlsl"
#define FORWARD_LIGHTING
#include "pixel_input.hlsl" 
 
cbuffer configBuffer : register(b3)
{ 
	ConfigParams configs;
};

cbuffer lightsCount : register(b4)
{  
	LightsCount g_lightCount; 
};
 
#include "../common/shadow_helpers.hlsl"
#define FULL_LIGHT
#include "../common/light_helpers.hlsl"
#include "../common/voxel_helpers.hlsl"

#include "../common/transparency_helpers.hlsl"

cbuffer volumeBuffer : register(b5)
{
	VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX];
};
   
            
float4 MediumPS(PI_Mesh input, bool front: SV_IsFrontFace) : SV_TARGET
{	 
	GBufferData gbuffer = (GBufferData)0;
	MediumData mediumData = (MediumData)0;
	
	gbuffer.albedo = AlbedoCalculate(samplerAnisotropicWrap, input.tex);
	gbuffer.normal = NormalCalculate(samplerAnisotropicWrap, input.tex, input.normal, input.tangent, input.binormal, normalMatrix);
	gbuffer.roughness = RoughnessCalculate(samplerAnisotropicWrap, input.tex);
	gbuffer.reflectivity = ReflectivityCalculate(samplerAnisotropicWrap, input.tex, gbuffer.albedo);
	gbuffer.emissive = EmissiveCalculate(samplerAnisotropicWrap, input.tex);
	gbuffer.ao = AOCalculate(samplerAnisotropicWrap, input.tex);

	gbuffer.subsurf = SubsurfaceCalculate(samplerAnisotropicWrap, input.tex);
	mediumData.absorption = AbsorptionCalculate(samplerAnisotropicWrap, input.tex);
	//mediumData.thickness = subsurface.a; // to shell
	
	mediumData.opacity = OpacityCalculate(samplerAnisotropicWrap, input.tex);
	mediumData.insideRoughness = InsideRoughnessCalculate(samplerAnisotropicWrap, input.tex);
	mediumData.attenuation = AttenuationCalculate();
	mediumData.invIOR = IORCalculate();
	mediumData.tirAmount = TIRCalculate();
	
	[branch]
	if(!front) 
		gbuffer.normal = -gbuffer.normal;
	gbuffer.tangent = normalize(cross(gbuffer.normal, cross(input.tangent, gbuffer.normal)));
	gbuffer.binormal = normalize(cross(gbuffer.tangent, gbuffer.normal));
	
	gbuffer.vertex_normal = normalize(cross(ddx(input.worldPos.xyz), ddy(input.worldPos.xyz)));
	 
	gbuffer.wpos = input.worldPos.xyz;
	gbuffer.depth = input.position.z;
	
	// THICKNESS CALCULATION
	float2 screenUV = input.position.xy * g_PixSize;

	mediumData.backNormal = sys_backNormal.SampleLevel(samplerPointClamp, screenUV, 0).xyz;

	mediumData.backDepth = DepthToLinear( sys_backDepth.SampleLevel(samplerPointClamp, screenUV, 0).r );
	mediumData.frontDepth = DepthToLinear( gbuffer.depth );
	mediumData.thickness = max(0, mediumData.backDepth - mediumData.frontDepth);
	
	gbuffer.thickness = 0.25;//mediumData.thickness;

	// LIGHT CALCULATION -----------------------------
	float3 ViewVector = g_CamPos - gbuffer.wpos;
	const float linDepth = length(ViewVector);
	ViewVector = ViewVector / linDepth; 
	
	DataForLightCompute mData = PrepareDataForLight(gbuffer, ViewVector); 

	float SO = computeSpecularOcclusion(mData.NoV, gbuffer.ao, mData.minR); 
	
	// DIRECT LIGHT
	LightComponents directLight = (LightComponents)0;
	[branch]
	if(configs.isLightweight == 0)  
	{
		const MaterialParams materialParams = (MaterialParams)0; 
		directLight = 
			ProcessLights(samplerPointClamp, sys_shadows, gbuffer, mData, materialParams, ViewVector, linDepth);
	} 

	// IBL
	float3 specularBrdf, diffuseBrdf;
	LightComponents indirectLight = CalcutaleDistantProbLight(samplerBilinearClamp, samplerTrilinearWrap, samplerBilinearWrap, 
		mData.NoV, mData.minR, ViewVector, gbuffer, SO, specularBrdf, diffuseBrdf);
	
	[branch] 
	if(configs.isLightweight == 0)
	{ 
		// VCTGI   
		LightComponentsWeight vctLight = CalculateVCTLight(samplerBilinearVolumeClamp, sys_volumeEmittance, volumeData, gbuffer, mData, 
			specularBrdf, diffuseBrdf, SO);

		indirectLight.diffuse = lerp(indirectLight.diffuse, vctLight.diffuse, vctLight.diffuseW);
		indirectLight.specular = lerp(indirectLight.specular, vctLight.specular, vctLight.specularW);
		indirectLight.scattering = lerp(indirectLight.scattering, vctLight.scattering, vctLight.scatteringW);
	}

	// TRANSMITTANCE
	float3 transmittance = CalcutaleMediumTransmittedLight(samplerPointClamp, sys_depth, 
		samplerTrilinearMirror, sys_sceneColor, screenUV, mediumData, mData, gbuffer, ViewVector);
	float transparency = 1 - mediumData.opacity;

	// OUTPUT
	float3 diffuse = indirectLight.diffuse + directLight.diffuse;
	float3 specular = indirectLight.specular * configs.indirSpec + directLight.specular * configs.dirSpec;
	float3 scattering = indirectLight.scattering * configs.indirDiff + directLight.scattering * configs.dirDiff;
	
	// TODO: corret scattering
	//scattering = scattering + transmittance;
	diffuse = lerp(diffuse + scattering, transmittance, transparency);

	float3 final = gbuffer.emissive + diffuse + specular;
	return float4(final, 1.0f);
}

float4 MediumPrepassPS(PI_Mesh input) : SV_TARGET
{
	//TODO: alphatest

	return float4(-input.normal.xyz, 0);
}