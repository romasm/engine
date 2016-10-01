TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "Combine";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

#include "light_constants.hlsl"

Texture2D diffuse : register(t0); 
Texture2D specular : register(t1); 
Texture2D gb_matID_objID : register(t2); 
Texture2D gb_depth : register(t3); 

SamplerState samplerPointClamp : register(s0);

struct PO_opaque
{
	float4 opaque : SV_TARGET0;
	float4 diffuse : SV_TARGET1;
};

PO_opaque Combine(PI_PosTex input)
{
	PO_opaque res;
	
	float4 light_diff = diffuse.Sample(samplerPointClamp, input.tex);
	float3 light_spec = 0;
	light_spec.gb = specular.Sample(samplerPointClamp, input.tex).rg;
	light_spec.r = light_diff.a;
	
	float d = gb_depth.Sample(samplerPointClamp, input.tex).r;
	
	// ss blur: TODO
	res.diffuse.rgb = light_diff.rgb;
	res.diffuse.a = d;

	res.opaque.rgb = res.diffuse.rgb + light_spec;
	res.opaque.a = d;
	
	return res;
}
