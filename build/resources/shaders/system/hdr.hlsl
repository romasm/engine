TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "HDRLDR";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

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
Texture3D <float> voxelTex : register(t14);

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

#define VOXEL_VOLUME_RES 256
#define VOXEL_VOLUME_SIZE 10.0f
#define VOXEL_SIZE VOXEL_VOLUME_SIZE / VOXEL_VOLUME_RES

float4 GetVoxel(float2 uv)
{
	float3 ray = GetCameraVector(uv);

	float step = 0.25 * VOXEL_VOLUME_SIZE / VOXEL_VOLUME_RES;
	
	float3 colorFin = 0;

	float dist = step;
	float value = 0;
	[loop]
	while(value < 1.0f && dist < VOXEL_VOLUME_SIZE * 2.0f)
	{
		float3 samplePos = g_CamPos + ray * dist;
		samplePos /= VOXEL_VOLUME_SIZE;

		[branch]
		if( samplePos.x > 1.0f || samplePos.x < 0.0f ||
			samplePos.y > 1.0f || samplePos.y < 0.0f ||
			samplePos.z > 1.0f || samplePos.z < 0.0f)
		{
			dist += step;
			continue;
		}

		float voxel = voxelTex.SampleLevel(samplerPointClamp, samplePos, 0).r;
	
		float3 voxelSnap = floor(samplePos * VOXEL_VOLUME_RES) / VOXEL_VOLUME_RES;
		float d = mul(float4(voxelSnap * VOXEL_VOLUME_SIZE, 1.0f), g_viewProj).z;

		colorFin += d * voxel;
		value += voxel;
		dist += step;
	}

	return float4(colorFin, value);
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
	else if(debugMode == 11)
	{
		float4 voxel = GetVoxel(input.tex);
		tonemapped = lerp(tonemapped, voxel.rgb, voxel.a * 0.75f);
	}

	float4 hud = hudTex.Sample(samplerPointClamp, input.tex);
	tonemapped = lerp(tonemapped, SRGBToLinear(hud.rgb), hud.a);
	
	if(debugMode == 0 || debugMode == 1 || debugMode == 2 || debugMode == 10)
		res.srgb.rgb = saturate(LinearToSRGB(tonemapped));
	else
		res.srgb.rgb = saturate(tonemapped);
	res.srgb.a = 1;
	
	res.lin.rgb = saturate(tonemapped);
	res.lin.a = 1;
	
	return res;
}
