#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"
#include "light_constants.hlsl"
#include "../common/voxel_helpers.hlsl"
#include "direct_brdf.hlsl"

#define GROUP_THREAD_COUNT 2

// DEBUG
#define DEBUG_CASCADE_LIGHTS 0

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


#define normalShadowOffsetSpot 2
#define shadowBiasSpotArea 0.0008 
#define shadowBiasSpot shadowBiasSpotArea * SHADOW_NEARCLIP

#define normalShadowOffsetPoint 2
#define shadowBiasPoint shadowBiasSpotArea * SHADOW_NEARCLIP

// todo: configurate
#define normalShadowOffsetDir0 0.5 * 0.05
#define normalShadowOffsetDir1 3 * 0.05
#define normalShadowOffsetDir2 10 * 0.05
#define normalShadowOffsetDir3 65 * 0.05
#define shadowBiasDir0 0.000005
//#define shadowBiasDir1 0.00001
//#define shadowBiasDir2 0.00003
//#define shadowBiasDir3 0.00015

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

cbuffer Lights : register(b3) 
{
	LightsIDs g_lightIDs;
};

cbuffer volumeBuffer : register(b4)
{
	VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX];
};

#include "light_calc.hlsl"
//#include "ssr.hlsl"

[numthreads( GROUP_THREAD_COUNT, GROUP_THREAD_COUNT, 1 )]
void DefferedLighting(uint3 threadID : SV_DispatchThreadID)
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
	float3 ViewVector = g_CamPos - gbuffer.wpos;
	const float linDepth = length(ViewVector);
	ViewVector = ViewVector / linDepth;
	
	// move to forward stage
	// normal fix for gazaring angles TODO
	//float NoV = dot(normal, VtoWP);
	//float nFix = (-clamp(NoV, -1.0f, NORMAL_CLAMP) * NORMAL_CLAMP_MUL) + 0.5f;    
	//normal = normalize(normal + VtoWP * NORMAL_CLAMP_DOUBLE * nFix);
	//tangent = normalize(cross(cross(normal, tangent), normal));
	//binormal = normalize(cross(cross(normal, binormal), binormal));

	// ----------------- DIRECT -------------------------
	DataForLightCompute mData;
	mData.R = gbuffer.roughness;
	mData.R.x = clamp(mData.R.x, 0.001f, 0.9999f);
	mData.R.y = clamp(mData.R.y, 0.001f, 0.9999f);
	mData.avgR = (mData.R.x + mData.R.y) * 0.5;
	mData.aGGX = max(mData.R.x * mData.R.y, 0.1);
	mData.sqr_aGGX = Square( mData.aGGX );
	mData.NoV = calculateNoV( gbuffer.normal, ViewVector );
	mData.reflect = 2 * gbuffer.normal * mData.NoV - ViewVector; 
	
	// SHADOW DEPTH FIX // TODO: remove?
#define NORMAL_OFFSET_MAX 10
	float3 shadowDepthFix;
	shadowDepthFix.x = clamp(0.15 * linDepth, 1, 1 + NORMAL_OFFSET_MAX);
	shadowDepthFix.y = saturate((shadowDepthFix.x - 1) * 1.0 / NORMAL_OFFSET_MAX);
	shadowDepthFix.x = 1;//pow(shadowDepthFix.x, 0.75);
	shadowDepthFix.z = max(0.5 * linDepth, 2);
	float dirDepthFix = max(0.1 * linDepth, 1);
	
	LightComponents directLight = 0;

	[loop] // spot
	for(int i_spt=0; i_spt < int(spot_count); i_spt++)
	{
		SpotLightBuffer lightData = g_spotLightBuffer[ g_lightIDs.SpotLightsIDs[i_spt] ];
		LightPrepared prepared = PrepareSpotLight(lightData, gbuffer);

		LightComponents lightResult;
		[branch] if( !CalculateSpotLight( lightData, prepared, gbuffer, mData, materialParams, ViewVector, lightResult ) )
			continue;
		
		directLight.Append(lightResult);
	}
	
	[loop] // caster spot
	for(int ic_spt=0; ic_spt < int(caster_spot_count); ic_spt++)
	{
		SpotCasterBuffer lightData = g_spotCasterBuffer[ g_lightIDs.SpotCastersIDs[ic_spt] ];
		SpotLightBuffer lightDataShort = 0;
		lightDataShort.Construct(lightData);

		LightPrepared prepared = PrepareSpotLight(lightDataShort, gbuffer);
		
		float lightAmount = SpotlightShadow(prepared, lightData, gbuffer, shadowDepthFix);
		[branch] if( lightAmount == 0 )
			continue;

		LightComponents lightResult;
		[branch] if( !CalculateSpotLight( lightDataShort, prepared, gbuffer, mData, materialParams, ViewVector, lightResult ) )
			continue;
		
		directLight.AppendShadowed(lightResult, lightAmount);
	}
	
	[loop] // disk
	for(int i_disk=0; i_disk < int(disk_count); i_disk++)
	{
		DiskLightBuffer lightData = g_diskLightBuffer[ g_lightIDs.DiskLightsIDs[i_disk] ];
		LightPrepared prepared = PrepareDiskLight(lightData, gbuffer);

		LightComponents lightResult;
		[branch] if( !CalculateDiskLight( lightData, prepared, gbuffer, mData, materialParams, ViewVector, lightResult ) )
			continue;
		
		directLight.Append(lightResult);
	}
	
	[loop] // caster disk
	for(int ic_disk=0; ic_disk < int(caster_disk_count); ic_disk++)
	{
		DiskCasterBuffer lightData = g_diskCasterBuffer[ g_lightIDs.DiskCasterIDs[ic_disk] ];
		DiskLightBuffer lightDataShort = 0;
		lightDataShort.Construct(lightData);

		LightPrepared prepared = PrepareDiskLight(lightDataShort, gbuffer);
		
		float lightAmount = AreaSpotlightShadow(prepared, gbuffer, shadowDepthFix, 
			lightData.DirConeY.xyz, lightData.ShadowmapParams, lightData.ShadowmapAdress, lightData.matViewProj);
		[branch] if( lightAmount == 0 )
			continue;

		LightComponents lightResult;
		[branch] if( !CalculateDiskLight( lightDataShort, prepared, gbuffer, mData, materialParams, ViewVector, lightResult ) )
			continue;
		
		directLight.AppendShadowed(lightResult, lightAmount);
	}
	
	[loop] // rect
	for(int i_rect=0; i_rect < int(rect_count); i_rect++)
	{
		RectLightBuffer lightData = g_rectLightBuffer[ g_lightIDs.RectLightsIDs[i_rect] ];
		LightPrepared prepared = PrepareRectLight(lightData, gbuffer);

		LightComponents lightResult;
		[branch] if( !CalculateRectLight( lightData, prepared, gbuffer, mData, materialParams, ViewVector, lightResult ) )
			continue;
		
		directLight.Append(lightResult);
	}
	
	[loop] // caster rect
	for(int ic_rect=0; ic_rect < int(caster_rect_count); ic_rect++)
	{
		RectCasterBuffer lightData = g_rectCasterBuffer[ g_lightIDs.RectCasterIDs[ic_rect] ];
		RectLightBuffer lightDataShort = 0;
		lightDataShort.Construct(lightData);

		LightPrepared prepared = PrepareRectLight(lightDataShort, gbuffer);
		
		float lightAmount = AreaSpotlightShadow(prepared, gbuffer, shadowDepthFix,
			lightData.DirConeY.xyz, lightData.ShadowmapParams, lightData.ShadowmapAdress, lightData.matViewProj);
		[branch] if( lightAmount == 0 )
			continue;

		LightComponents lightResult;
		[branch] if( !CalculateRectLight( lightDataShort, prepared, gbuffer, mData, materialParams, ViewVector, lightResult ) )
			continue;
		
		directLight.AppendShadowed(lightResult, lightAmount);
	}

	[loop] // point
	for(int i_p=0; i_p < int(point_count); i_p++)
	{
		PointLightBuffer lightData = g_pointLightBuffer[ g_lightIDs.PointLightsIDs[i_p] ];
		LightPrepared prepared = PreparePointLight(lightData, gbuffer);

		LightComponents lightResult;
		[branch] if( !CalculatePointLight( lightData, prepared, gbuffer, mData, materialParams, ViewVector, lightResult ) )
			continue;
		
		directLight.Append(lightResult);
	}
	
	[loop] // caster point
	for(int ic_p=0; ic_p < int(caster_point_count); ic_p++)
	{
		PointCasterBuffer lightData = g_point�asterBuffer[ g_lightIDs.Point�asterIDs[ic_p] ];
		PointLightBuffer lightDataShort = 0;
		lightDataShort.Construct(lightData);

		LightPrepared prepared = PreparePointLight(lightDataShort, gbuffer);
		
		float lightAmount = PointlightShadow(prepared, lightData, gbuffer, shadowDepthFix);
		[branch] if( lightAmount == 0 )
			continue;

		LightComponents lightResult;
		[branch] if( !CalculatePointLight( lightDataShort, prepared, gbuffer, mData, materialParams, ViewVector, lightResult ) )
			continue;
		
		directLight.AppendShadowed(lightResult, lightAmount);
	}

	[loop] // sphere
	for(int i_sph=0; i_sph < int(sphere_count); i_sph++)
	{
		SphereLightBuffer lightData = g_sphereLightBuffer[ g_lightIDs.SphereLightsIDs[i_sph] ];
		LightPrepared prepared = PrepareSphereLight(lightData, gbuffer);

		LightComponents lightResult;
		[branch] if( !CalculateSphereLight( lightData, prepared, gbuffer, mData, materialParams, ViewVector, lightResult ) )
			continue;
		
		directLight.Append(lightResult);
	}
	
	[loop] // caster sphere
	for(int ic_sph=0; ic_sph < int(caster_sphere_count); ic_sph++)
	{
		SphereCasterBuffer lightData = g_sphere�asterBuffer[ g_lightIDs.Sphere�asterIDs[ic_sph] ];
		SphereLightBuffer lightDataShort = 0;
		lightDataShort.Construct(lightData);

		LightPrepared prepared = PrepareSphereLight(lightDataShort, gbuffer);
		
		float lightAmount = PointlightShadow(prepared, lightData, gbuffer, shadowDepthFix);
		[branch] if( lightAmount == 0 )
			continue;

		LightComponents lightResult;
		[branch] if( !CalculateSphereLight( lightDataShort, prepared, gbuffer, mData, materialParams, ViewVector, lightResult ) )
			continue;
		
		directLight.AppendShadowed(lightResult, lightAmount);
	}

	[loop] // tube
	for(int i_tube=0; i_tube < int(tube_count); i_tube++)
	{
		SphereLightBuffer lightData = g_sphereLightBuffer[ g_lightIDs.SphereLightsIDs[i_tube] ];
		LightPrepared prepared = PrepareSphereLight(lightData, gbuffer);

		LightComponents lightResult;
		[branch] if( !CalculateSphereLight( lightData, prepared, gbuffer, mData, materialParams, ViewVector, lightResult ) )
			continue;
		
		directLight.Append(lightResult);
	}
	
	[loop] // caster tube
	for(int ic_tube=0; ic_tube < int(caster_tube_count); ic_tube++)
	{
		TubeCasterBuffer lightData = g_tube�asterBuffer[ g_lightIDs.Tube�asterIDs[ic_tube] ];
		TubeLightBuffer lightDataShort = 0;
		lightDataShort.Construct(lightData);

		LightPrepared prepared = PrepareTubeLight(lightDataShort, gbuffer);
		
		float lightAmount = TubelightShadow(prepared, lightData, gbuffer, shadowDepthFix);
		[branch] if( lightAmount == 0 )
			continue;

		LightComponents lightResult;
		[branch] if( !CalculateTubeLight( lightDataShort, prepared, gbuffer, mData, materialParams, ViewVector, lightResult ) )
			continue;
		
		directLight.AppendShadowed(lightResult, lightAmount);
	}




	
	// point
	[loop]
	for(uint i_p=0; i_p < uint(point_count); i_p++)
	{
		const float4 pos_range = Point_Pos_Range[i_p];
		const float3 color = Point_Color[i_p].rgb;
	
		const float3 unnormL = pos_range.xyz - wpos;
		const float3 L = normalize(unnormL);
		
		const float illuminance = getDistanceAtt( unnormL, pos_range.w );
		if(illuminance == 0)
			continue;
			
		const float3 colorIlluminance = illuminance * color;
			
		const float NoL = saturate(dot(normal, L));
		if(NoL == 0.0f)
		{
			//if(params.subscattering != 0)
				Light.diffuse += colorIlluminance * directSubScattering(subsurf, params, L, normal, VtoWP);
			continue;
		}
		
		const float3 H = normalize(VtoWP + L);
		const float VoH = saturate(dot(VtoWP, H));
		const float NoH = saturate( dot(normal, H) + 0.00001f );
		
		const float3 colorIlluminanceDir = colorIlluminance * NoL;
		
		Light.specular += colorIlluminanceDir * directSpecularBRDF(spec, roughnessXY, NoH, NoV, NoL, VoH, H, tangent, binormal, avgR);	
		Light.diffuse += colorIlluminanceDir * directDiffuseBRDF(albedo, avgR, NoV, NoL, VoH);
		
		//if(params.subscattering != 0)
			Light.diffuse += colorIlluminance * directSubScattering(subsurf, params, L, normal, VtoWP);
	}
	
	// sphere
	[loop]
	for(uint i_sph=0; i_sph < uint(sphere_count); i_sph++)
	{
		const float4 pos_range = Sphere_Pos_Range[i_sph];
		const float3 color = Sphere_Color_Empty[i_sph].rgb;
		const float2 rad_sqrrad = Sphere_AreaInfo_Empty[i_sph].rg;
	
		const float3 unnormL = pos_range.xyz - wpos;
		const float3 L = normalize(unnormL);
			
		const float sqrDist = dot( unnormL, unnormL );
			
		const float smoothFalloff = smoothDistanceAtt(sqrDist, pos_range.w);
		if(smoothFalloff == 0)
			continue;
		
		// const float3 newRefl = getSpecularDominantDirArea(N, Refl, avgR); 
				
		// specular
		const float SphereAngle = clamp( rad_sqrrad.x / max(sqrt( sqrDist ), rad_sqrrad.x), 0, 0.5 );
		const float specEnergy = Square( aGGX / saturate( aGGX + SphereAngle ) );
		
		const float3 centerToRay = dot(unnormL, Refl) * Refl - unnormL;
		const float3 specL = normalize(unnormL + centerToRay * saturate(rad_sqrrad.x / sqrt(dot(centerToRay, centerToRay))));
			
		//diffuse
		const float3 diffL = normalize(unnormL + normal * rad_sqrrad.x * (1 - saturate(dot(normal, L))));	
				
		// Sphere evaluation
		const float cosTheta = clamp( dot(normal, L), -0.999, 0.999);
		const float sinSigmaSqr = min( rad_sqrrad.y / sqrDist, 0.9999f );
		float noDirIlluminance;
		float illuminance = illuminanceSphereOrDisk( cosTheta, sinSigmaSqr, noDirIlluminance );
		
		noDirIlluminance *= smoothFalloff;
		illuminance *= smoothFalloff;
			
		if(illuminance == 0)
		{
			//if(params.subscattering != 0)
				Light.diffuse += noDirIlluminance * color * directSubScattering(subsurf, params, L, normal, VtoWP);
			continue;
		}
			
		const float diffNoL = saturate(dot(normal, diffL));	
		const float3 diffH = normalize(VtoWP + diffL);
		const float diffVoH = saturate(dot(VtoWP, diffH));

		const float specNoL = saturate( dot(normal, specL) );
		const float3 specH = normalize(VtoWP + specL);
		const float specNoH = saturate( dot(normal, specH) + 0.00001f );
		const float specVoH = saturate( dot(VtoWP, specH) );
		
		const float3 colorIlluminance = illuminance * color;
		
		Light.specular += colorIlluminance * specEnergy * directSpecularBRDF(spec, roughnessXY, specNoH, NoV, specNoL, specVoH, specH, tangent, binormal, avgR);	
		Light.diffuse += colorIlluminance * directDiffuseBRDF(albedo, avgR, NoV, diffNoL, diffVoH);
		
		//if(params.subscattering != 0)
			Light.diffuse += noDirIlluminance * color * directSubScattering(subsurf, params, L, normal, VtoWP);
	}
	
	// tube
	[loop]
	for(uint i_t=0; i_t < uint(tube_count); i_t++)
	{
		const float4 pos_range = Tube_Pos_Range[i_t];
		const float3 color = Tube_Color_Empty[i_t].rgb;
		const float4 rad_len_sqrrad_sqrlen = Tube_AreaInfo[i_t];
		const float4 dir_areaa = Tube_Dir_AreaA[i_t];
	
		const float3 unnormL = pos_range.xyz - wpos;
		const float3 L = normalize(unnormL);
			
		const float sqrDist = dot( unnormL, unnormL );
			
		const float smoothFalloff = smoothDistanceAtt(sqrDist, pos_range.w);
		if(smoothFalloff == 0)
			continue;
		
		// const float3 newRefl = getSpecularDominantDirArea(N, Refl, avgR); 
				
		// specular
		const float LineAngle = saturate( rad_len_sqrrad_sqrlen.y / max(sqrt( sqrDist ), rad_len_sqrrad_sqrlen.x) );
		float specEnergy = aGGX / saturate( aGGX + 0.5 * LineAngle );
				
		// Closest point on line segment to ray
		const float3 Ld = dir_areaa.xyz * rad_len_sqrrad_sqrlen.y;
		const float3 halfLd = 0.5 * Ld;
		const float3 L0 = unnormL - halfLd;

		// Shortest distance
		const float b = dot( Refl, Ld );
		const float t = saturate( dot( L0, b * Refl - Ld ) / (rad_len_sqrrad_sqrlen.w - b * b) );
		float3 specL = L0 + t * Ld;
		
		// sphere
		const float SphereAngle = clamp( rad_len_sqrrad_sqrlen.x / max(sqrt( dot( specL, specL ) ), rad_len_sqrrad_sqrlen.x), 0, 0.5 );
		specEnergy *= Square( aGGX / saturate( aGGX + SphereAngle ) );
		
		const float3 centerToRay = dot(specL, Refl) * Refl - specL;
		specL = normalize(specL + centerToRay * saturate(rad_len_sqrrad_sqrlen.x / sqrt(dot(centerToRay, centerToRay))));
			
		// diffuse
		const float3 diffL = normalize( unnormL + normal * dir_areaa.w * (1 - saturate(dot(normal, L))) );
		
		float noDirIlluminance;
		float illuminance = illuminanceTube( pos_range.xyz, wpos, normal, rad_len_sqrrad_sqrlen.x, rad_len_sqrrad_sqrlen.z, L, L0, Ld, rad_len_sqrrad_sqrlen.w, noDirIlluminance );
			
		noDirIlluminance *= smoothFalloff;
		illuminance *= smoothFalloff;
		
		if(illuminance == 0)
		{
			//if(params.subscattering != 0)
				Light.diffuse += noDirIlluminance * color * directSubScattering(subsurf, params, L, normal, VtoWP);
			continue;
		}
			
		const float diffNoL = saturate(dot(normal, diffL));	
		const float3 diffH = normalize(VtoWP + diffL);
		const float diffVoH = saturate(dot(VtoWP, diffH));

		const float specNoL = saturate( dot(normal, specL) );
		const float3 specH = normalize(VtoWP + specL);
		const float specNoH = saturate( dot(normal, specH) + 0.00001f );
		const float specVoH = saturate( dot(VtoWP, specH) );
			
		const float3 colorIlluminance = illuminance * color;
		
		Light.specular += colorIlluminance * specEnergy * directSpecularBRDF(spec, roughnessXY, specNoH, NoV, specNoL, specVoH, specH, tangent, binormal, avgR);	
		Light.diffuse += colorIlluminance * directDiffuseBRDF(albedo, avgR, NoV, diffNoL, diffVoH);
		
		//if(params.subscattering != 0)
			Light.diffuse += noDirIlluminance * color * directSubScattering(subsurf, params, L, normal, VtoWP);
	}

	// dir
	[loop]
	for(uint i_dir=0; i_dir < uint(dir_count); i_dir++)
	{
		const float4 color_areax = Dir_Color_AreaX[i_dir];
		const float4 dir_areay = Dir_Dir_AreaY[i_dir];
		
		const float4 pos_res_0 = Dir_Pos0[i_dir];
		const float4 pos_res_1 = Dir_Pos1[i_dir];
		const float4 pos_res_2 = Dir_Pos2[i_dir];
		const float4 pos_res_3 = Dir_Pos3[i_dir];
		matrix vp_mat_0 = Dir_ViewProj0[i_dir];
		matrix vp_mat_1 = Dir_ViewProj1[i_dir];
		matrix vp_mat_2 = Dir_ViewProj2[i_dir];
		matrix vp_mat_3 = Dir_ViewProj3[i_dir];
		const float4 adress_0 = Dir_ShadowmapAdress0[i_dir];
		const float4 adress_1 = Dir_ShadowmapAdress1[i_dir];
		const float4 adress_2 = Dir_ShadowmapAdress2[i_dir];
		const float4 adress_3 = Dir_ShadowmapAdress3[i_dir];
			
		const float3 L = -dir_areay.xyz;
		
		const float LoR = saturate(dot(L, Refl));
		const float3 projR = Refl - LoR * L;
		const float3 specL = LoR < dir_areay.w ? normalize(dir_areay.w * L + normalize(projR) * color_areax.w) : Refl;
			
		const float3 H = normalize(VtoWP + L);
		const float VoH = saturate(dot(VtoWP, H));
		const float NoL = saturate(dot(normal, L));
		
		const float3 specH = normalize(VtoWP + specL);
		const float specNoH = saturate( dot(normal, specH) + 0.00001f );
		const float specVoH = saturate( dot(VtoWP, specH) );
		const float specNoL = saturate( dot(normal, specL) );

		float3 colorLight = color_areax.rgb;

		if(specNoL == 0.0f)
		{
			//if(params.subscattering != 0)
				Light.diffuse += colorLight * directSubScattering(subsurf, params, L, normal, VtoWP);
			continue;
		}

#if DEBUG_CASCADE_LIGHTS == 0
		float light_blocked = DirlightShadow(wpos, dir_areay.xyz, pos_res_0.xyz, pos_res_1.xyz, pos_res_2.xyz, pos_res_3.xyz, vertex_normal, dot(vertex_normal, L), 
			vp_mat_0, vp_mat_1, vp_mat_2, vp_mat_3, adress_0, adress_1, adress_2, adress_3,  float3(shadowDepthFix.xy, dirDepthFix), 
			float4(pos_res_0.w, pos_res_1.w, pos_res_2.w, pos_res_3.w));
		if(light_blocked == 0)
			continue;
		colorLight *= light_blocked;
#else
		Light.diffuse += DirlightShadow(wpos, dir_areay.xyz, pos_res_0.xyz, pos_res_1.xyz, pos_res_2.xyz, pos_res_3.xyz, vertex_normal, dot(vertex_normal, L), 
			vp_mat_0, vp_mat_1, vp_mat_2, vp_mat_3, adress_0, adress_1, adress_2, adress_3, float3(shadowDepthFix.xy, dirDepthFix), 
			float4(pos_res_0.w, pos_res_1.w, pos_res_2.w, pos_res_3.w));
		continue;
#endif
		
		Light.specular += colorLight * specNoL * directSpecularBRDF(spec, roughnessXY, specNoH, NoV, specNoL, specVoH, specH, tangent, binormal, avgR);	
		Light.diffuse += colorLight * NoL * directDiffuseBRDF(albedo, avgR, NoV, NoL, VoH);
		
		//if(params.subscattering != 0)
			Light.diffuse += colorLight * directSubScattering(subsurf, params, L, normal, VtoWP);
	}
	
	// caster point
	[loop]
	for(uint ic_p=0; ic_p < uint(caster_point_count); ic_p++)
	{
		const float4 pos_range = CastPoint_Pos_Range[ic_p];
		const float4 color_texelProj = CastPoint_Color_ShParams[ic_p];
		
		const float4 adress0 = CastPoint_ShadowmapAdress0[ic_p];
		const float4 adress1 = CastPoint_ShadowmapAdress1[ic_p];
		const float4 adress2 = CastPoint_ShadowmapAdress2[ic_p];
		const float4 adress3 = CastPoint_ShadowmapAdress3[ic_p];
		const float4 adress4 = CastPoint_ShadowmapAdress4[ic_p];
		const float4 adress5 = CastPoint_ShadowmapAdress5[ic_p];
		const float4 texelSize0 = CastPoint_ShadowmapParams0[ic_p];
		const float2 texelSize1 = CastPoint_ShadowmapParams1[ic_p].xy;
		matrix proj_mat = CastPoint_Proj[ic_p];
	
		const float3 unnormL = pos_range.xyz - wpos;
		
		const float illuminance = getDistanceAtt( unnormL, pos_range.w );
		if(illuminance == 0)
			continue;
		
		const float3 L = normalize(unnormL);
		const float NoL = saturate(dot(normal, L));
	
		float3 colorIlluminance = illuminance * color_texelProj.rgb;
			
		if(NoL == 0.0f)
		{
			//if(params.subscattering != 0)
				Light.diffuse += colorIlluminance * directSubScattering(subsurf, params, L, normal, VtoWP);
			continue;
		}
	
		float light_blocked = PointlightShadow(wpos, -unnormL, pos_range.xyz, vertex_normal, NoL, proj_mat, 
			adress0, adress1, adress2, adress3, adress4, adress5, color_texelProj.w, texelSize0, texelSize1, shadowDepthFix);
		if(light_blocked == 0)
			continue;
				
		colorIlluminance *= light_blocked;
			
		
		const float3 H = normalize(VtoWP + L);
		const float VoH = saturate(dot(VtoWP, H));
		const float NoH = saturate( dot(normal, H) + 0.00001f );
		
		const float3 colorIlluminanceDir = colorIlluminance * NoL;
		
		Light.specular += colorIlluminanceDir * directSpecularBRDF(spec, roughnessXY, NoH, NoV, NoL, VoH, H, tangent, binormal, avgR);	
		Light.diffuse += colorIlluminanceDir * directDiffuseBRDF(albedo, avgR, NoV, NoL, VoH);
		
		//if(params.subscattering != 0)
			Light.diffuse += colorIlluminance * directSubScattering(subsurf, params, L, normal, VtoWP);
	}
	
	// caster sphere
	[loop]
	for(uint ic_sph=0; ic_sph < uint(caster_sphere_count); ic_sph++)
	{
		const float4 pos_range = CastSphere_Pos_Range[ic_sph];
		const float4 color_texelProj = CastSphere_Color_ShParams[ic_sph];
		const float4 rad_sqrrad_texetSize = CastSphere_AreaInfo_ShParams[ic_sph];
	
		const float4 adress0 = CastSphere_ShadowmapAdress0[ic_sph];
		const float4 adress1 = CastSphere_ShadowmapAdress1[ic_sph];
		const float4 adress2 = CastSphere_ShadowmapAdress2[ic_sph];
		const float4 adress3 = CastSphere_ShadowmapAdress3[ic_sph];
		const float4 adress4 = CastSphere_ShadowmapAdress4[ic_sph];
		const float4 adress5 = CastSphere_ShadowmapAdress5[ic_sph];
		const float4 texelSize = CastSphere_ShadowmapParams[ic_sph];
		matrix proj_mat = CastSphere_Proj[ic_sph];
	
		const float3 unnormL = pos_range.xyz - wpos;
			
		const float sqrDist = dot( unnormL, unnormL );
			
		float smoothFalloff = smoothDistanceAtt(sqrDist, pos_range.w);
		if(smoothFalloff == 0)
			continue;
			
		const float3 L = normalize(unnormL);
		const float NoL = dot(normal, L);
		
		float light_blocked = PointlightShadow(wpos, -unnormL, pos_range.xyz, vertex_normal, NoL, proj_mat, 
			adress0, adress1, adress2, adress3, adress4, adress5, color_texelProj.w, texelSize, rad_sqrrad_texetSize.zw, shadowDepthFix);
		if(light_blocked == 0)
			continue;
				
		smoothFalloff *= light_blocked;
		
		// const float3 newRefl = getSpecularDominantDirArea(N, Refl, avgR); 
				
		// specular
		const float SphereAngle = clamp( rad_sqrrad_texetSize.x / max(sqrt( sqrDist ), rad_sqrrad_texetSize.x), 0, 0.5 );
		const float specEnergy = Square( aGGX / saturate( aGGX + SphereAngle ) );
		
		const float3 centerToRay = dot(unnormL, Refl) * Refl - unnormL;
		const float3 specL = normalize(unnormL + centerToRay * saturate(rad_sqrrad_texetSize.x / sqrt(dot(centerToRay, centerToRay))));
			
		//diffuse
		const float3 diffL = normalize(unnormL + normal * rad_sqrrad_texetSize.x * (1 - saturate(dot(normal, L))));	
				
		// Sphere evaluation
		const float cosTheta = clamp( dot(normal, L), -0.999, 0.999);
		const float sinSigmaSqr = min( rad_sqrrad_texetSize.y / sqrDist, 0.9999f );
		float noDirIlluminance;
		float illuminance = illuminanceSphereOrDisk( cosTheta, sinSigmaSqr, noDirIlluminance );
		
		noDirIlluminance *= smoothFalloff;
		illuminance *= smoothFalloff;
			
		if(illuminance == 0)
		{
			//if(params.subscattering != 0)
				Light.diffuse += noDirIlluminance * color_texelProj.rgb * directSubScattering(subsurf, params, L, normal, VtoWP);
			continue;
		}
			
		const float diffNoL = saturate(dot(normal, diffL));	
		const float3 diffH = normalize(VtoWP + diffL);
		const float diffVoH = saturate(dot(VtoWP, diffH));

		const float specNoL = saturate( dot(normal, specL) );
		const float3 specH = normalize(VtoWP + specL);
		const float specNoH = saturate( dot(normal, specH) + 0.00001f );
		const float specVoH = saturate( dot(VtoWP, specH) );
			
		const float3 colorIlluminance = illuminance * color_texelProj.rgb;
		
		Light.specular += colorIlluminance * specEnergy * directSpecularBRDF(spec, roughnessXY, specNoH, NoV, specNoL, specVoH, specH, tangent, binormal, avgR);	
		Light.diffuse += colorIlluminance * directDiffuseBRDF(albedo, avgR, NoV, diffNoL, diffVoH);
		
		//if(params.subscattering != 0)
			Light.diffuse += noDirIlluminance * color_texelProj.rgb * directSubScattering(subsurf, params, L, normal, VtoWP);
	}
	
	// caster tube
	[loop]
	for(uint ic_t=0; ic_t < uint(caster_tube_count); ic_t++)
	{
		const float4 pos_range = CastTube_Pos_Range[ic_t];
		const float4 color_texelProj = CastTube_Color_ShParams[ic_t];
		const float4 rad_len_sqrrad_sqrlen = CastTube_AreaInfo[ic_t];
		const float4 dir_areaa = CastTube_Dir_AreaA[ic_t];
		
		const float4 texelSize0 = CastTube_ShadowmapParams0[ic_t];
		const float2 texelSize1 = CastTube_ShadowmapParams1[ic_t].xy;
		const float4 adress0 = CastTube_ShadowmapAdress0[ic_t];
		const float4 adress1 = CastTube_ShadowmapAdress1[ic_t];
		const float4 adress2 = CastTube_ShadowmapAdress2[ic_t];
		const float4 adress3 = CastTube_ShadowmapAdress3[ic_t];
		const float4 adress4 = CastTube_ShadowmapAdress4[ic_t];
		const float4 adress5 = CastTube_ShadowmapAdress5[ic_t];
		matrix proj_mat = CastTube_Proj[ic_t];
		matrix view_mat = CastTube_View[ic_t];
	
		const float3 unnormL = pos_range.xyz - wpos;
		
		const float sqrDist = dot( unnormL, unnormL );
			
		float smoothFalloff = smoothDistanceAtt(sqrDist, pos_range.w);
		if(smoothFalloff == 0)
			continue;
		
		const float3 L = normalize(unnormL);
		const float NoL = dot(normal, L);
		
		float light_blocked = TubelightShadow(wpos, vertex_normal, NoL, proj_mat, view_mat,
			adress0, adress1, adress2, adress3, adress4, adress5, color_texelProj.w, texelSize0, texelSize1, shadowDepthFix);
		if(light_blocked == 0)
			continue;
				
		smoothFalloff *= light_blocked;
		
		// const float3 newRefl = getSpecularDominantDirArea(N, Refl, avgR); 
				
		// specular
		const float LineAngle = saturate( rad_len_sqrrad_sqrlen.y / max(sqrt( sqrDist ), rad_len_sqrrad_sqrlen.x) );
		float specEnergy = aGGX / saturate( aGGX + 0.5 * LineAngle );
				
		// Closest point on line segment to ray
		const float3 Ld = dir_areaa.xyz * rad_len_sqrrad_sqrlen.y;
		const float3 halfLd = 0.5 * Ld;
		const float3 L0 = unnormL - halfLd;

		// Shortest distance
		const float b = dot( Refl, Ld );
		const float t = saturate( dot( L0, b * Refl - Ld ) / (rad_len_sqrrad_sqrlen.w - b * b) );
		float3 specL = L0 + t * Ld;
		
		// sphere
		const float SphereAngle = clamp( rad_len_sqrrad_sqrlen.x / max(sqrt( dot( specL, specL ) ), rad_len_sqrrad_sqrlen.x), 0, 0.5 );
		specEnergy *= Square( aGGX / saturate( aGGX + SphereAngle ) );
		
		const float3 centerToRay = dot(specL, Refl) * Refl - specL;
		specL = normalize(specL + centerToRay * saturate(rad_len_sqrrad_sqrlen.x / sqrt(dot(centerToRay, centerToRay))));
			
		// diffuse
		const float3 diffL = normalize( unnormL + normal * dir_areaa.w * (1 - saturate(dot(normal, L))) );
		
		float noDirIlluminance;
		float illuminance = illuminanceTube( pos_range.xyz, wpos, normal, rad_len_sqrrad_sqrlen.x, rad_len_sqrrad_sqrlen.z, L, L0, Ld, rad_len_sqrrad_sqrlen.w, noDirIlluminance );
			
		noDirIlluminance *= smoothFalloff;
		illuminance *= smoothFalloff;
		
		if(illuminance == 0)
		{
			//if(params.subscattering != 0)
				Light.diffuse += noDirIlluminance * color_texelProj.rgb * directSubScattering(subsurf, params, L, normal, VtoWP);
			continue;
		}
			
		const float diffNoL = saturate(dot(normal, diffL));	
		const float3 diffH = normalize(VtoWP + diffL);
		const float diffVoH = saturate(dot(VtoWP, diffH));

		const float specNoL = saturate( dot(normal, specL) );
		const float3 specH = normalize(VtoWP + specL);
		const float specNoH = saturate( dot(normal, specH) + 0.00001f );
		const float specVoH = saturate( dot(VtoWP, specH) );
			
		const float3 colorIlluminance = illuminance * color_texelProj.rgb;
		
		Light.specular += colorIlluminance * specEnergy * directSpecularBRDF(spec, roughnessXY, specNoH, NoV, specNoL, specVoH, specH, tangent, binormal, avgR);	
		Light.diffuse += colorIlluminance * directDiffuseBRDF(albedo, avgR, NoV, diffNoL, diffVoH);
		
		//if(params.subscattering != 0)
			Light.diffuse += noDirIlluminance * color_texelProj.rgb * directSubScattering(subsurf, params, L, normal, VtoWP);
	}
	
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
	
	float4 specSecond;
	 
	float SO = computeSpecularOcclusion(indirNoV, ao, indirR);
	
	specSecond.rgb = (ssr.rgb * SO) * ssr.a;
	specSecond.a = 1 - ssr.a;
	specSecond.rgb *= specBrdf;

	Indir.diffuse = skyEnvProbDiff(normal, VtoWP, indirNoV, indirR, envprobsDistDiff);
		   
	/*if(params.subscattering != 0)
	{
		res_diff.rgb += indirectSubScattering(subsurf.rgb, params, normal, VtoWP, ao, 0, envprobsDistDiff, 2, envprobsDist);
	}*/ 
	 
	if(Indir.specular.r >= 0)
	{
		// Voxel Cone Tracing
		float4 diffuseVCT = 0;
		const float apertureDiffuse = 0.57735f;
		for(int diffuseCones = 0; diffuseCones < 4; diffuseCones++)
		{
			float3 coneDirection = normal;
			coneDirection += diffuseConeDirectionsCheap[diffuseCones].x * tangent + diffuseConeDirectionsCheap[diffuseCones].z * binormal;
			coneDirection = normalize(coneDirection);
        
			float4 VCTdiffuse = VoxelConeTrace(wpos, coneDirection, apertureDiffuse, normal, volumeData, volumeEmittance, samplerBilinearVolumeClamp);
			diffuseVCT += VCTdiffuse * diffuseConeWeightsCheap[diffuseCones];
		} 
	   
		//if(Indir.specular.r != 0) 
		//	Indir.diffuse = 0;

		Indir.diffuse = lerp( Indir.diffuse, diffuseVCT.rgb, diffuseVCT.a);
			                     
		float3 coneReflDirection = normalize(Refl);

		float apertureSpecular = tan( clamp( PIDIV2 * avgR, 0.0174533f, PI) );
		float4 specularVCT = VoxelConeTrace(wpos, coneReflDirection, apertureSpecular, normal, volumeData, volumeEmittance, samplerBilinearVolumeClamp);
	 
		Indir.specular = lerp( Indir.specular, specularVCT.rgb, specularVCT.a);
	} 

	Indir.diffuse *= diffBrdf * ao; 
	Indir.specular *= specBrdf * SO;
	  
	// ----------------- FINAL -------------------------
	res.diffuse.rgb = Light.diffuse * dirDiff + (emissive + Indir.diffuse) * indirDiff;
	res.specular.rgb = Indir.specular * indirSpec;

	specSecond.rgb += Light.specular * dirSpec;

	res.diffuse.a = specSecond.r;
	res.specular.a = specSecond.g;
	res.specularMore.rg = specSecond.ba;

	return res; 
}
