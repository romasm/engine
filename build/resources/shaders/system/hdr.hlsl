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
Texture3D <uint> voxelTex : register(t14);

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

static float3 axis_colors[6] = {
	float3(0.6,0.0,0.0),
	float3(0.8,0.0,0.0),
	float3(0.4,0.0,0.0),
	float3(0.0,0.0,0.6),
	float3(0.0,0.0,0.8),
	float3(0.0,0.0,0.4)
};

#define VOXEL_ALPHA 1.0

float2 GetVoxel(float2 uv)
{
	const float3 boxExtend = float3(VOXEL_VOLUME_SIZE, VOXEL_VOLUME_SIZE, VOXEL_VOLUME_SIZE) * 0.5f;
	const float3 voxelSize = float3(VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE);
	const float distance = 25.0f;

	float3 ray = GetCameraVector(uv) * distance;
	float3 camInBox = g_CamPos - boxExtend;

	float2 intersections = RayBoxIntersect( camInBox, ray, -boxExtend, boxExtend );

	if ( intersections.y <= intersections.x )
		return 0;
	
	float3 epcilon = 0.00001f;
	float3 step;
	step.x = ray.x >= 0 ? 1 : -1;
	step.y = ray.y >= 0 ? 1 : -1;
	step.z = ray.z >= 0 ? 1 : -1;

	epcilon *= step;

	step = saturate(step);
	step *= voxelSize;
	
	float3 samplePoint = g_CamPos + ray * intersections.x + epcilon;
	float3 voxelSnap = 0;
	float3 prevVoxel = 0;
	
	float value = 0;
	float depth = 0;
	float3 color = 0;
	int i = 0;
	[loop]
	while( i < 512 )
	{
		voxelSnap = floor(samplePoint / VOXEL_VOLUME_SIZE * VOXEL_VOLUME_RES);

		value = float( voxelTex.Load(int4(voxelSnap, 0)) ) * 0.0625f;
		
		voxelSnap *= VOXEL_VOLUME_SIZE / VOXEL_VOLUME_RES;
		prevVoxel = voxelSnap;

		[branch]
		if(value > 0.0f)
			break;

		voxelSnap += step;

		float3 delta = (voxelSnap - g_CamPos) / ray;
		float d = min(delta.x, min(delta.y, delta.z));

		[branch]
		if ( intersections.y <= d )
			return 0;

		samplePoint = g_CamPos + ray * d + epcilon;
		i++;
	}

	float3 voxelExtend = voxelSize * 0.5f;
	prevVoxel += voxelExtend;
	float3 camInVoxel = g_CamPos - prevVoxel;
	float2 voxelIntersections = RayBoxIntersect( camInVoxel, ray, -voxelExtend, voxelExtend );
	float3 collidePosWS = g_CamPos + ray * voxelIntersections.x;

	float4 collidePosPS = mul(float4(collidePosWS, 1.0f), g_viewProj);

	return float2(value, collidePosPS.z / collidePosPS.w);
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
		float2 voxel = GetVoxel(input.tex);
		float sceneDepth = gb_depth.SampleLevel(samplerPointClamp, UVforSamplePow2(input.tex), 0).r;
		if(sceneDepth >= voxel.g) 
			tonemapped = lerp(tonemapped, float3(voxel.r, 0, 1 - voxel.r), float(voxel.r > 0) * VOXEL_ALPHA);
	}

	float4 hud = hudTex.Sample(samplerPointClamp, input.tex);
	tonemapped = lerp(tonemapped, SRGBToLinear(hud.rgb), hud.a);
	
	if(debugMode == 0 || debugMode == 1 || debugMode == 2 || debugMode == 10 || debugMode == 11)
		res.srgb.rgb = saturate(LinearToSRGB(tonemapped));
	else
		res.srgb.rgb = saturate(tonemapped);
	res.srgb.a = 1;
	
	res.lin.rgb = saturate(tonemapped);
	res.lin.a = 1;
	
	return res;
}
