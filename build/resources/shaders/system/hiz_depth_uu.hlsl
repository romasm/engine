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

Texture2D depth : register(t0); 
Texture2D vis : register(t1); 

SamplerState samplerPointClamp : register(s0);

cbuffer materialBuffer : register(b1)
{
	float coordData0;
	float coordData1;
	float calcVis;
	float _padding0;
};

struct PO_HIZ
{
    float2 hiz : SV_TARGET0;
	float vis : SV_TARGET1;
};

#define VIS_DIV 1.0f/9.0f

PO_HIZ CalcHiZ(PI_PosTex input)
{
	// hi z
	float2 coords1 = input.tex - float2(coordData0, coordData1) * 0.49f;
	float2 coords2 = coords1 + float2(0, coordData1 * 0.49f);
	float2 coords3 = coords1 + float2(coordData0 * 0.49f, 0);
	float2 coords4 = coords1 + float2(coordData0, coordData1) * 0.49f;
	float2 coords5 = coords1 + float2(0, coordData1 * 0.99f);
	float2 coords6 = coords1 + float2(coordData0 * 0.99f, 0);
	float2 coords7 = coords1 + float2(coordData0 * 0.99f, coordData1 * 0.49f);
	float2 coords8 = coords1 + float2(coordData0 * 0.49f, coordData1 * 0.99f);
	float2 coords9 = coords1 + float2(coordData0, coordData1) * 0.99f;

	float2 depth1, depth2, depth3, depth4, depth5, depth6, depth7, depth8, depth9;

	depth1 = depth.SampleLevel(samplerPointClamp, coords1, 0).rg;
	depth2 = depth.SampleLevel(samplerPointClamp, coords2, 0).rg;
	depth3 = depth.SampleLevel(samplerPointClamp, coords3, 0).rg;
	depth4 = depth.SampleLevel(samplerPointClamp, coords4, 0).rg;
	depth5 = depth.SampleLevel(samplerPointClamp, coords5, 0).rg;
	depth6 = depth.SampleLevel(samplerPointClamp, coords6, 0).rg;
	depth7 = depth.SampleLevel(samplerPointClamp, coords7, 0).rg;
	depth8 = depth.SampleLevel(samplerPointClamp, coords8, 0).rg;
	depth9 = depth.SampleLevel(samplerPointClamp, coords9, 0).rg;
	
	PO_HIZ res;
	res.hiz.x = min(min(min( min(depth1.x, depth2.x), min(depth3.x, depth4.x) ), min( min(depth5.x, depth6.x), min(depth7.x, depth8.x) )), depth9.x);
	res.hiz.y = max(max(max( max(depth1.y, depth2.y), max(depth3.y, depth4.y) ), max( max(depth5.y, depth6.y), max(depth7.y, depth8.y) )), depth9.y);

	// vis
	float vis1 = 1;
	float vis2 = 1;
	float vis3 = 1;
	float vis4 = 1;
	float vis5 = 1;
	float vis6 = 1;
	float vis7 = 1;
	float vis8 = 1;
	float vis9 = 1;
	if(calcVis > 0)
	{
		vis1 = vis.SampleLevel(samplerPointClamp, coords1, 0).r;
		vis2 = vis.SampleLevel(samplerPointClamp, coords2, 0).r;
		vis3 = vis.SampleLevel(samplerPointClamp, coords3, 0).r;
		vis4 = vis.SampleLevel(samplerPointClamp, coords4, 0).r;
		vis5 = vis.SampleLevel(samplerPointClamp, coords5, 0).r;
		vis6 = vis.SampleLevel(samplerPointClamp, coords6, 0).r;
		vis7 = vis.SampleLevel(samplerPointClamp, coords7, 0).r;
		vis8 = vis.SampleLevel(samplerPointClamp, coords8, 0).r;
		vis9 = vis.SampleLevel(samplerPointClamp, coords9, 0).r;
	}
	res.vis = 0;
	float maxmindiff = 1.0f / (res.hiz.y - res.hiz.x);
	res.vis += ((depth1.x - res.hiz.x) * maxmindiff) * vis1;
	res.vis += ((depth2.x - res.hiz.x) * maxmindiff) * vis2;
	res.vis += ((depth3.x - res.hiz.x) * maxmindiff) * vis3;
	res.vis += ((depth4.x - res.hiz.x) * maxmindiff) * vis4;
	res.vis += ((depth5.x - res.hiz.x) * maxmindiff) * vis5;
	res.vis += ((depth6.x - res.hiz.x) * maxmindiff) * vis6;
	res.vis += ((depth7.x - res.hiz.x) * maxmindiff) * vis7;
	res.vis += ((depth8.x - res.hiz.x) * maxmindiff) * vis8;
	res.vis += ((depth9.x - res.hiz.x) * maxmindiff) * vis9;
	res.vis *= VIS_DIV;

	return res;
}
