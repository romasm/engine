TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "CalcHiZ";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

#include "light_constants.hlsl"

Texture2D depthTex : register(t0); 
//Texture2D visTex : register(t1); 

SamplerState samplerPointClamp : register(s0);

cbuffer materialBuffer : register(b1)
{
	float calcVis;
	float _padding0;
	float _padding1;
	float _padding2;
};

struct PO_HIZ
{
    float2 hiz : SV_TARGET0;
	//float vis : SV_TARGET1;
};

#define VIS_DIV 1.0f/4.0f

PO_HIZ CalcHiZ(PI_PosTex input)
{
	// hi z
	float4 depthMin = depthTex.GatherRed(samplerPointClamp, input.tex);
	float4 depthMax = depthTex.GatherGreen(samplerPointClamp, input.tex);
	
	PO_HIZ res;
	res.hiz.x = min( min(depthMin.x, depthMin.y), min(depthMin.z, depthMin.w) );
	res.hiz.y = max( max(depthMax.x, depthMax.y), max(depthMax.z, depthMax.w) );
	/*
	// vis
	float4 vis = 1;
	if(calcVis > 0)
		vis = visTex.GatherRed(samplerPointClamp, input.tex);
	
	res.vis = 0;
	float maxmindiff = 1.0f / (res.hiz.y - res.hiz.x);
	res.vis += ((depthMin.x - res.hiz.x) * maxmindiff) * vis.x;
	res.vis += ((depthMin.y - res.hiz.x) * maxmindiff) * vis.y;
	res.vis += ((depthMin.z - res.hiz.x) * maxmindiff) * vis.z;
	res.vis += ((depthMin.w - res.hiz.x) * maxmindiff) * vis.w;
	res.vis *= VIS_DIV;
	*/
	return res;
}
