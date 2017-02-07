TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "Combine";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

#include "../common/light_structs.hlsl"

Texture2D diffuse : register(t0); 
Texture2D specular : register(t1); 
Texture2D specularMore : register(t2); 
//Texture2D gb_matID_objID : register(t3); 
Texture2D gb_depth : register(t3); 

SamplerState samplerPointClamp : register(s0);

struct PO_opaque
{
	float4 opaque : SV_TARGET0;
	float4 forNextFrame : SV_TARGET1;
};

PO_opaque Combine(PI_PosTex input)
{
	PO_opaque res;
	
	float4 diffSample = diffuse.Sample(samplerPointClamp, input.tex);
	float4 specSample = specular.Sample(samplerPointClamp, input.tex);
	float2 specSecondSample = specularMore.Sample(samplerPointClamp, input.tex).rg;

	float d = gb_depth.Sample(samplerPointClamp, UVforSamplePow2(input.tex)).r;
	
	// ss blur: TODO
	res.forNextFrame.rgb = diffSample.rgb + specSample.rgb;
	res.forNextFrame.a = d;

	res.opaque.rgb = diffSample.rgb + specSample.rgb * specSecondSample.g + float3(diffSample.a, specSample.a, specSecondSample.r);
	res.opaque.a = d < 1.0f ? 1.0f : 0.0f;
	
	return res;
}
