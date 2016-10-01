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

#define VIS_DIV 1.0f/4.0f

PO_HIZ CalcHiZ(PI_PosTex input)
{
	// hi z
	float2 coords1 = input.tex - float2(coordData0, coordData1) * 0.49f;
	float2 coords2 = coords1 + float2(0, coordData1 * 0.99f);
	float2 coords3 = coords1 + float2(coordData0 * 0.99f, 0);
	float2 coords4 = coords1 + float2(coordData0, coordData1) * 0.99f;
	
	float2 depth1, depth2, depth3, depth4;

	depth1 = depth.SampleLevel(samplerPointClamp, coords1, 0).rg;
	depth2 = depth.SampleLevel(samplerPointClamp, coords2, 0).rg;
	depth3 = depth.SampleLevel(samplerPointClamp, coords3, 0).rg;
	depth4 = depth.SampleLevel(samplerPointClamp, coords4, 0).rg;
	
	PO_HIZ res;
	res.hiz.x = min( min(depth1.x, depth2.x), min(depth3.x, depth4.x) );
	res.hiz.y = max( max(depth1.y, depth2.y), max(depth3.y, depth4.y) );

	// vis
	float vis1 = 1;
	float vis2 = 1;
	float vis3 = 1;
	float vis4 = 1;
	if(calcVis > 0)
	{
		vis1 = vis.SampleLevel(samplerPointClamp, coords1, 0).r;
		vis2 = vis.SampleLevel(samplerPointClamp, coords2, 0).r;
		vis3 = vis.SampleLevel(samplerPointClamp, coords3, 0).r;
		vis4 = vis.SampleLevel(samplerPointClamp, coords4, 0).r;
	}
	res.vis = 0;
	float maxmindiff = 1.0f / (res.hiz.y - res.hiz.x);
	res.vis += ((depth1.x - res.hiz.x) * maxmindiff) * vis1;
	res.vis += ((depth2.x - res.hiz.x) * maxmindiff) * vis2;
	res.vis += ((depth3.x - res.hiz.x) * maxmindiff) * vis3;
	res.vis += ((depth4.x - res.hiz.x) * maxmindiff) * vis4;
	res.vis *= VIS_DIV;

	return res;
}
