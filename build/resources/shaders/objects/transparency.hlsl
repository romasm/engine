#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"

SamplerState samplerPointClamp : register(s0);
SamplerState samplerBilinearClamp : register(s1);
SamplerState samplerBilinearWrap : register(s2);
SamplerState samplerTrilinearWrap : register(s3);
SamplerState samplerBilinearVolumeClamp : register(s4);
SamplerState samplerAnisotropicWrap : register(s5);

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

StructuredBuffer<SpotLightBuffer> g_spotLightBuffer : register(t5); 
StructuredBuffer<DiskLightBuffer> g_diskLightBuffer : register(t6); 
StructuredBuffer<RectLightBuffer> g_rectLightBuffer : register(t7); 

StructuredBuffer<SpotCasterBuffer> g_spotCasterBuffer : register(t8); 
StructuredBuffer<DiskCasterBuffer> g_diskCasterBuffer : register(t9); 
StructuredBuffer<RectCasterBuffer> g_rectCasterBuffer : register(t10); 

StructuredBuffer<PointLightBuffer> g_pointLightBuffer : register(t11); 
StructuredBuffer<SphereLightBuffer> g_sphereLightBuffer : register(t12); 
StructuredBuffer<TubeLightBuffer> g_tubeLightBuffer : register(t13); 

StructuredBuffer<PointCasterBuffer> g_pointCasterBuffer : register(t14); 
StructuredBuffer<SphereCasterBuffer> g_sphereCasterBuffer : register(t15); 
StructuredBuffer<TubeCasterBuffer> g_tubeCasterBuffer : register(t16); 

StructuredBuffer<DirLightBuffer> g_dirLightBuffer : register(t17); 

Texture2D <float4> sys_sceneColor : register(t18); 
Texture2D <float2> sys_depth : register(t19);

#define FORWARD_LIGHTING
#include "pixel_input.hlsl"

cbuffer configBuffer : register(b3)
{
	ConfigParams configs;
};

cbuffer Lights : register(b4) 
{
	LightsIDs g_lightIDs;
};

#include "../common/shadow_helpers.hlsl"
#include "../system/direct_brdf.hlsl"
#define FULL_LIGHT
#include "../common/light_helpers.hlsl"
#include "../common/voxel_helpers.hlsl"

cbuffer volumeBuffer : register(b5)
{
	VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX];
};
   
            
float4 MediumPS(PI_Mesh input, bool front: SV_IsFrontFace)
{	 
	GBufferData gbuffer = (GBufferData)0;
	MediumData mediumData = (MediumData)0;

	gbuffer.albedo = AlbedoCalculate(samplerAnisotropicWrap, input.tex);
	gbuffer.normal = NormalCalculate(samplerAnisotropicWrap, input.tex, input.normal, input.tangent, input.binormal, normalMatrix);
	gbuffer.roughness = RoughnessCalculate(samplerAnisotropicWrap, input.tex);
	gbuffer.reflectivity = ReflectivityCalculate(samplerAnisotropicWrap, input.tex, albedo);
	gbuffer.emissive = EmissiveCalculate(samplerAnisotropicWrap, input.tex);
	gbuffer.ao = AOCalculate(samplerAnisotropicWrap, input.tex);

	float4 subsurface = SSSCalculate(samplerAnisotropicWrap, input.tex);
	mediumData.insideColor = subsurface.rgb;
	mediumData.thickness = subsurface.a;

	mediumData.opacity = OpacityCalculate(samplerAnisotropicWrap, input.tex);
	mediumData.insideRoughness = InsideRoughnessCalculate(samplerAnisotropicWrap, input.tex);
	mediumData.absorption = AbsorptionCalculate(samplerAnisotropicWrap, input.tex);
	
	if(!front)
		gbuffer.normal = -gbuffer.normal;
	gbuffer.tangent = normalize(cross(gbuffer.normal, cross(input.tangent, gbuffer.normal)));
	gbuffer.binormal = normalize(cross(gbuffer.tangent, gbuffer.normal));
	
	gbuffer.vertex_normal = normalize(cross(ddx(input.worldPos.xyz), ddy(input.worldPos.xyz)));
	
	gbuffer.wpos = input.worldPos.xyz;
	gbuffer.depth = input.position.z / input.position.w;
	
	// LIGHT CALCULATION -----------------------------
	
	float3 ViewVector = g_CamPos - gbuffer.wpos;
	const float linDepth = length(ViewVector);
	ViewVector = ViewVector / linDepth;

	DataForLightCompute mData = PrepareDataForLight(gbuffer, ViewVector);

	float SO = computeSpecularOcclusion(mData.NoV, gbuffer.ao, mData.minR);

	// TRANSMITTANCE
	float4 transmittance = CalcutaleMediumTransmittanceLight(samplerPointClamp, sys_depth, samplerTrilinearWrap, sys_sceneColor, mediumData);
	
	// DIRECT LIGHT
	const MaterialParams materialParams = (MaterialParams)0;
	LightComponents directLight = 
		ProcessLights(samplerPointClamp, sys_shadows, gbuffer, mData, materialParams, ViewVector, linDepth);
	
	// IBL
	float3 specularBrdf, diffuseBrdf;
	LightComponents indirectLight = CalcutaleDistantProbLight(samplerBilinearClamp, samplerTrilinearWrap, samplerBilinearWrap, 
		mData.NoV, mData.minR, ViewVector, gbuffer, SO, specularBrdf, diffuseBrdf);
	
	// VCTGI   
	LightComponentsWeight vctLight = CalculateVCTLight(samplerBilinearVolumeClamp, sys_volumeEmittance, volumeData, gbuffer, mData, 
		specularBrdf, diffuseBrdf, SO);

	indirectLight.diffuse = lerp(indirectLight.diffuse, vctLight.diffuse, vctLight.diffuseW);
	indirectLight.specular = lerp(indirectLight.specular, vctLight.specular, vctLight.specularW);
	indirectLight.scattering = lerp(indirectLight.scattering, vctLight.scattering, vctLight.scatteringW);
	
	// OUTPUT
	float3 diffuse = (indirectLight.diffuse + indirectLight.scattering) * configs.indirDiff + 
		(directLight.diffuse + directLight.scattering) * configs.dirDiff;
	float3 specular = indirectLight.specular * configs.indirSpec + directLight.specular * configs.dirSpec;

	float3 final = gbuffer.emissive + diffuse + specular;
	return float4(final, gbuffer.depth);
}