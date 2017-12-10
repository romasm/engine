TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "HDRLDR";
}

#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

#include "../common/voxel_helpers.hlsl"

#define VOXEL_ALPHA 1.0

Texture2D opaqueTex : register(t0); 
Texture2D transparentTex : register(t1); 
Texture2D hudTex : register(t2); 
Texture2D lumTex : register(t3); 
Texture2D bloomTex : register(t4); 

Texture2D <float4> gb_albedo_roughY : register(t5); 
Texture2D <float4> gb_tbn : register(t6); 
Texture2D <float4> gb_spec_roughX : register(t7); 
Texture2D <float4> gb_emiss_vnZ : register(t8); 
Texture2D <float4> gb_subs_thick : register(t9); 
Texture2D <float> gb_ao : register(t10); 
Texture2D <float> dynamicAO : register(t11); 
Texture2D <float2> gb_depth : register(t12);
// debug
Texture2D <float4> ssrTex : register(t13); 
Texture3D <float4> voxelLightTex : register(t14); 
Texture3D <float4> voxelEmittanceTex : register(t15); 

SamplerState samplerPointClamp : register(s0);
SamplerState samplerBilinearClamp : register(s1);

cbuffer materialBuffer : register(b1)
{
	float tm_A;
	float tm_B;
	float tm_C;
	float tm_D;

	float tm_E;
	float tm_F;
	float tm_W;
	float exposure;

	float applyToLum;
	float middleGray;
	float bloomAmount;
	float bloomMul;

	float debugMode;
	float voxelVis;
	float voxelCascade;
	float _padding2;
};

cbuffer volumeBuffer0 : register(b2)
{
	VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX];
};

cbuffer volumeBuffer1 : register(b3)
{ 
	VolumeTraceData volumeTraceData;
};   

#include "tonemapping.hlsl"

/*
float3 Abberation(Texture2D opaque, Texture2D transperency, float2 coord)
{
	float2 from_center = coord - float2(0.5f,0.5f);
	float dist = length(from_center);
	float aber = saturate(-7.4641016151*(cos(dist * 0.5235987756f)-1));
	
	float2 pix = float2(1.0f/float(g_screenW), 1.0f/float(g_screenH));
	
	const float str = 0.0f;
	aber = clamp(aber, 0.25f, 1.0f) * str;
	
	float2 offset_r = normalize(from_center);
	offset_r = pix * offset_r * aber;
	float2 offset_b = -offset_r;
	
	float color_r = opaque.Sample(samplerClamp, coord+offset_r).r;
	float color_g = opaque.Sample(samplerClamp, coord).g;
	float color_b = opaque.Sample(samplerClamp, coord+offset_b).b;
	float2 trsp_r = transperency.Sample(samplerClamp, coord+offset_r).ra;
	float2 trsp_g = transperency.Sample(samplerClamp, coord).ga;
	float2 trsp_b = transperency.Sample(samplerClamp, coord+offset_b).ba;

	float3 fin = 0;
	fin.r = lerp(color_r, trsp_r.r, trsp_r.g);
	fin.g = lerp(color_g, trsp_g.r, trsp_g.g);
	fin.b = lerp(color_b, trsp_b.r, trsp_b.g);
	
	return fin;
}
*/

float4 Combine(Texture2D opaque, Texture2D transperency, float2 coord)
{
	float4 color = opaque.Sample(samplerPointClamp, coord);
	float4 trsp = transperency.Sample(samplerPointClamp, coord);
	color.rgb = lerp(color.rgb, trsp.rgb, trsp.a);
	
	color.a = color.a < 1.0 ? 1 : 0;
	color.a = saturate(color.a + trsp.a);
	return color;
}

struct PO_LDR
{
    float4 srgb : SV_TARGET0;
	float4 lin : SV_TARGET1;
};

float2 GetVoxelOpacity(float2 uv, uint level)
{
	float3 collidePosWS = 0;
	int4 sampleCoords = GetVoxelOnRay(g_CamPos, GetCameraVector(uv), volumeData, volumeTraceData, level, voxelEmittanceTex, collidePosWS);
	if( sampleCoords.w < 0.0f )
		return 0;
	  
	float4 emittance = voxelEmittanceTex.Load(sampleCoords);
	float4 collidePosPS = mul(float4(collidePosWS, 1.0f), g_viewProj);

	return float2(saturate(emittance.w), collidePosPS.z / collidePosPS.w);
}
     
float4 GetVoxelEmittance(float2 uv, uint level)
{ 
	float3 collidePosWS = 0;
	int4 sampleCoords = GetVoxelOnRay(g_CamPos, GetCameraVector(uv), volumeData, volumeTraceData, level, voxelEmittanceTex, collidePosWS);
	if( sampleCoords.w < 0.0f )
		return 0;
	    
	float4 emittance = voxelEmittanceTex.Load(sampleCoords);

	[flatten]
	if(emittance.w > 0)
		emittance.rgb /= emittance.w;

	float4 collidePosPS = mul(float4(collidePosWS, 1.0f), g_viewProj);
	   
	return float4(emittance.rgb, collidePosPS.z / collidePosPS.w);
}       

PO_LDR HDRLDR(PI_PosTex input)
{
	PO_LDR res;
	
	float4 combinedScene = Combine(opaqueTex, transparentTex, input.tex);
	float3 lin = combinedScene.rgb;
	
	const float2 lum_coords = float2(0.5f, 0.5f);
	float averageFrameLum = lumTex.Sample(samplerPointClamp, lum_coords).r;
	
	float lum = luminance(lin);
	
	/*static const float bloomMipsWiegth[5] = 
	{
		0.332452, 0.234927, 0.082898, 0.014607, 0.001285
	};*/
	
	float3 bloom = 0;
	[unroll]
	for(uint i=0; i<5; i++)
		bloom += bloomTex.SampleLevel(samplerBilinearClamp, input.tex, i).rgb;// * bloomMipsWiegth[i];
	
	float bloomPower = bloomAmount * averageFrameLum;
	if(bloomPower < 0)
		lin += bloom * bloomMul;
	else
		lin += bloom * bloomMul * saturate( (bloomPower / (bloomPower + 1) ) - (lum / (lum + 1)) );
	
	float3 tonemapped = 0;
	if(applyToLum > 0)
	{
		lum = luminance(lin);
		tonemapped = lin;
		tonemapped *= FilmicTonemapping(lum, averageFrameLum)/lum;
	}
	else
	{
		tonemapped = FilmicTonemapping(lin, averageFrameLum);
	}
	
	// debug
	if(debugMode == 0)
		;
	else if(debugMode == 1)
		tonemapped = gb_albedo_roughY.Sample(samplerPointClamp, input.tex).rgb;
	else if(debugMode == 2)
		tonemapped = gb_spec_roughX.Sample(samplerPointClamp, input.tex).rgb;
	else if(debugMode == 3)
	{
		tonemapped.r = gb_spec_roughX.Sample(samplerPointClamp, input.tex).a;
		tonemapped.g = gb_albedo_roughY.Sample(samplerPointClamp, input.tex).a;
		tonemapped.b = tonemapped.r == tonemapped.g ? tonemapped.r : 0;
	}
	else if(debugMode == 4)
	{
		const float4 TBN = gb_tbn.Sample(samplerPointClamp, input.tex);
		float3 normal;
		float3 tangent;
		float3 binormal;
		DecodeTBNfromFloat4(tangent, binormal, normal, TBN);
		tonemapped = normal * 0.5 + 0.5;
	}
	else if(debugMode == 5)
		tonemapped = gb_emiss_vnZ.Sample(samplerPointClamp, input.tex).rgb;
	else if(debugMode == 6)
		tonemapped = gb_subs_thick.Sample(samplerPointClamp, input.tex).rgb;
	else if(debugMode == 7)
		tonemapped = gb_subs_thick.Sample(samplerPointClamp, input.tex).a;
	else if(debugMode == 8)
		tonemapped = min(gb_ao.Sample(samplerPointClamp, input.tex).r, dynamicAO.Sample(samplerPointClamp, input.tex).r);
	else if(debugMode == 9)
		tonemapped = PowAbs( gb_depth.SampleLevel(samplerPointClamp, UVforSamplePow2(input.tex), 0).r, 30 );
	else if(debugMode == 10)
	{
		float4 ssr = ssrTex.Sample(samplerPointClamp, input.tex);
		tonemapped = ssr.rgb * ssr.a;
	}
	
	if(voxelVis > 0)       
	{ 
		float sceneDepth = gb_depth.SampleLevel(samplerPointClamp, UVforSamplePow2(input.tex), 0).r;
		float viewLength = length(GetWPos(input.tex, sceneDepth) - g_CamPos);
		
		if(voxelVis == 1)               
		{      
			float4 light = GetVoxelLightOnRay(g_CamPos, GetCameraVector(input.tex), viewLength, volumeData, 
				volumeTraceData, voxelCascade, voxelLightTex, 0.003);
			tonemapped = lerp(0, light.rgb, light.a * VOXEL_ALPHA); 
		}  
		else if(voxelVis == 2)         
		{  
			float4 light = GetVoxelLightOnRay(g_CamPos, GetCameraVector(input.tex), viewLength, volumeData, 
				volumeTraceData, voxelCascade, voxelLightTex, 0.01);
			tonemapped = lerp(0, 1, light.a * VOXEL_ALPHA); 
		}
		else if(voxelVis == 3) 
		{ 
			float2 voxelOpacity = GetVoxelOpacity(input.tex, voxelCascade);      
			if(sceneDepth >= voxelOpacity.g && voxelOpacity.g != 0) 
				tonemapped = lerp(tonemapped, float3(voxelOpacity.r, 0, 1 - voxelOpacity.r), VOXEL_ALPHA);
		}
		else if(voxelVis == 4) 
		{
			float4 voxelEmittance = GetVoxelEmittance(input.tex, voxelCascade);
			if(sceneDepth >= voxelEmittance.a && voxelEmittance.a != 0) 
				tonemapped = lerp(tonemapped, voxelEmittance.rgb, VOXEL_ALPHA);
		}  
	}            

	float4 hud = hudTex.Sample(samplerPointClamp, input.tex);
	tonemapped = lerp(tonemapped, SRGBToLinear(hud.rgb), hud.a);
	
	if(debugMode == 0 || debugMode == 1 || debugMode == 2 || (debugMode >= 10))
		res.srgb.rgb = saturate(LinearToSRGB(tonemapped));
	else
		res.srgb.rgb = saturate(tonemapped);
	res.srgb.a = combinedScene.a;
	
	res.lin.rgb = saturate(tonemapped);
	res.lin.a = 1;
	
	return res; 
}
