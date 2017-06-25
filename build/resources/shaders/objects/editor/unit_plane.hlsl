TECHNIQUE_DEFAULT
{
	Queue = GUI_3D;

	DepthEnable = true;
	DepthWrite = false;
	DepthFunc = LESS_EQUAL;

	BlendEnable = true;
	BlendOp = ADD;
	BlendOpAlpha = ADD;
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
	SrcBlendAlpha = SRC_ALPHA;
	DestBlendAlpha = INV_SRC_ALPHA;

	CullMode = NONE;

	VertexShader = "PlaneVS";
	PixelShader = "PlanePS";
}

#include "../../common/math.hlsl"
#include "../../common/shared.hlsl"
#include "../../common/structs.hlsl"

Texture2D colorTex : register(t0);
SamplerState samplerAnisotropicWrap : register(s0);

cbuffer matrixBuffer : register(b2)
{
	matrix worldMatrix;
	matrix normalMatrix;
};

// config
static const float lineAlpha = 0.65;
static const float lineContrast = 0.8;
static const float lineBrightness[2] = {0.7, 1.0};
static const float lineHardness = 1000.0;

static const float lineThickness = 0.01;
static const float lineNearClip = 0.1;
static const float lineNearFade = 0.1;

float4 GridCascade(float2 wpos, float distToCam, int i)
{
	float2 wposNear = floor(wpos + 0.5);
	float2 edge = 1 - saturate(abs(wposNear - wpos) - lineThickness);
	edge = pow(edge, lineHardness);
	
	float alpha = max(edge.x, edge.y);

	float3 color = float3(edge.y, 0.0, edge.x);
	color = lerp(float3(1,1,1), color, lineContrast) * lineBrightness[i];

	return float4(color, alpha);
}

float4 PlanePS(PI_ToolMesh input) : SV_TARGET
{	
	float4 cascade0 = colorTex.Sample(samplerAnisotropicWrap, input.worldPos.xz * 10 + 0.5).r;
	cascade0.a *= lineAlpha;
	if( cascade0.a < 0.1 )
		discard;
	
	cascade0.rgb = lineBrightness[0];

	float3 V = g_CamPos - input.worldPos.xyz;
	float NoV = abs(dot(normalize(V), input.normal));
	NoV = saturate(pow(NoV * 5, 1.0));
	
	float distToCam = length(V);

	float4 cascade1 = GridCascade(input.worldPos.xz, distToCam, 1);

	cascade0.rgb = lerp(cascade0.rgb, cascade1.rgb, cascade1.a);
	
	float distFadeNear = saturate( ( distToCam - lineNearClip) / lineNearFade);
	cascade0.a *= distFadeNear;

	return saturate(cascade0);
}

// vetrex
PI_ToolMesh PlaneVS(VI_Mesh input)
{
    PI_ToolMesh output;

    output.position = mul(float4(input.position, 1), worldMatrix);
	output.position.xz += g_CamPos.xz; 
	output.worldPos = output.position;
    output.position = mul(output.position, g_viewProj);
	
    output.normal = mul(input.normal, (float3x3)normalMatrix);
    output.normal = normalize(output.normal);

	output.tex = input.tex;

    return output;
}