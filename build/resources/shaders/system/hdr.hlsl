TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "HDRLDR";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

#include "../common/voxel_helpers.hlsl"

#define VOXEL_ALPHA 0.75

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
Texture3D <uint> voxelTex : register(t14);
Texture3D <uint> voxelColor0Tex : register(t15);
Texture3D <uint> voxelColor1Tex : register(t16);
Texture3D <uint> voxelNormalTex : register(t17);
Texture3D <float4> voxelEmittanceTex : register(t18);

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
	float _padding0;
	float _padding1;
	float _padding2;
};

cbuffer volumeBuffer : register(b2)
{
	VolumeData volumeData[VCT_CLIPMAP_COUNT_MAX];
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

float3 Combine(Texture2D opaque, Texture2D transperency, float2 coord)
{
	float3 color = opaque.Sample(samplerPointClamp, coord).rgb;
	float4 trsp = transperency.Sample(samplerPointClamp, coord);
	return lerp(color, trsp.rgb, trsp.a);
}

struct PO_LDR
{
    float4 srgb : SV_TARGET0;
	float4 lin : SV_TARGET1;
};

float2 GetVoxelOpacity(float2 uv, uint level)
{
	float3 collidePosWS = 0;
	int4 sampleCoords = GetVoxelOnRay(g_CamPos, GetCameraVector(uv), volumeData, level, voxelEmittanceTex, collidePosWS);
	if( sampleCoords.w < 0.0f )
		return 0;
	 
	float4 emittance = voxelEmittanceTex.Load(sampleCoords);
	float4 collidePosPS = mul(float4(collidePosWS, 1.0f), g_viewProj);

	return float2(emittance.w, collidePosPS.z / collidePosPS.w);
}
    
float4 GetVoxelEmittance(float2 uv, uint level)
{ 
	float3 collidePosWS = 0;
	int4 sampleCoords = GetVoxelOnRay(g_CamPos, GetCameraVector(uv), volumeData, level, voxelEmittanceTex, collidePosWS);
	if( sampleCoords.w < 0.0f )
		return 0;
	    
	float4 emittance = voxelEmittanceTex.Load(sampleCoords);
	float4 collidePosPS = mul(float4(collidePosWS, 1.0f), g_viewProj);
	   
	return float4(emittance.rgb, collidePosPS.z / collidePosPS.w);
}      
          
float4 GetVoxelColor(float2 uv, out float depth)
{
	depth = 0;
	float3 collidePosWS = 0;
	int4 sampleCoords = GetVoxelOnRay(g_CamPos, GetCameraVector(uv), volumeData, 0, voxelEmittanceTex, collidePosWS);	 
	if( sampleCoords.w < 0.0f )
		return 0;
	            
	uint count = DecodeVoxelOpacity(voxelTex.Load(sampleCoords));
	 
	uint color0 = voxelColor0Tex.Load(sampleCoords);
	uint color1 = voxelColor1Tex.Load(sampleCoords);
	float4 color = DecodeVoxelColor(color0, color1, count);
		  
	float4 collidePosPS = mul(float4(collidePosWS, 1.0f), g_viewProj);
	depth = collidePosPS.z / collidePosPS.w;
	
	return color;
}  
 
float4 GetVoxelNormal(float2 uv)
{
	float3 collidePosWS = 0;
	int4 sampleCoords = GetVoxelOnRay(g_CamPos, GetCameraVector(uv), volumeData, 0, voxelEmittanceTex, collidePosWS);
	if( sampleCoords.w < 0.0f )
		return 0;

	float3 normal = DecodeVoxelNormal(voxelNormalTex.Load(sampleCoords), voxelTex.Load(sampleCoords));
	float4 collidePosPS = mul(float4(collidePosWS, 1.0f), g_viewProj);

	return float4(normal, collidePosPS.z / collidePosPS.w);
}

PO_LDR HDRLDR(PI_PosTex input)
{
	PO_LDR res;
	
	float3 lin = Combine(opaqueTex, transparentTex, input.tex);
	
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
	else 
	{
		float sceneDepth = gb_depth.SampleLevel(samplerPointClamp, UVforSamplePow2(input.tex), 0).r;

		if(debugMode == 11)
		{
			float voxelDepth = 0;
			float4 voxelColor = GetVoxelColor(input.tex, voxelDepth);
			if(sceneDepth >= voxelDepth && voxelDepth != 0) 
				tonemapped = lerp(tonemapped, voxelColor.rgb, float(voxelColor.a == 0) * VOXEL_ALPHA);
		}  
		else if(debugMode == 12)
		{ 
			float voxelDepth = 0;
			float4 voxelColor = GetVoxelColor(input.tex, voxelDepth);
			if(sceneDepth >= voxelDepth && voxelDepth != 0) 
				tonemapped = lerp(tonemapped, voxelColor.rgb, float(voxelColor.a != 0) * VOXEL_ALPHA);
		} 
		else if(debugMode == 13) 
		{       
			float voxelDepth = 0;
			float4 voxelColor = GetVoxelColor(input.tex, voxelDepth);
			if(sceneDepth >= voxelDepth && voxelDepth != 0) 
				tonemapped = lerp(tonemapped, voxelColor.a / 100.0f, float(voxelColor.a != 0) * VOXEL_ALPHA);
		}  
		else if(debugMode == 14) 
		{
			float4 voxelNormal = GetVoxelNormal(input.tex);
			if(sceneDepth >= voxelNormal.a && voxelNormal.a != 0) 
				tonemapped = lerp(tonemapped, (voxelNormal.rgb + 1.0f) * 0.5f, VOXEL_ALPHA);
		} 
		else if(debugMode >= 15 && debugMode <= 20)
		{
			float2 voxelOpacity = GetVoxelOpacity(input.tex, debugMode - 15);
			if(sceneDepth >= voxelOpacity.g && voxelOpacity.g != 0) 
				tonemapped = lerp(tonemapped, float3(voxelOpacity.r, 0, 1 - voxelOpacity.r), VOXEL_ALPHA);
		}
		else if(debugMode >= 21 && debugMode <= 26)
		{
			float4 voxelEmittance = GetVoxelEmittance(input.tex, debugMode - 21);
			if(sceneDepth >= voxelEmittance.a && voxelEmittance.a != 0) 
				tonemapped = lerp(tonemapped, voxelEmittance.rgb, VOXEL_ALPHA);
		} 
	}           
	                 
	float4 hud = hudTex.Sample(samplerPointClamp, input.tex);
	tonemapped = lerp(tonemapped, SRGBToLinear(hud.rgb), hud.a);
	
	if(debugMode == 0 || debugMode == 1 || debugMode == 2 || (debugMode >= 10 && debugMode != 14))
		res.srgb.rgb = saturate(LinearToSRGB(tonemapped));
	else
		res.srgb.rgb = saturate(tonemapped);
	res.srgb.a = 1;
	
	res.lin.rgb = saturate(tonemapped);
	res.lin.a = 1;
	
	return res;
}
