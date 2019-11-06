TECHNIQUE_DEFAULT
{
	Queue = SC_FORWARD;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS_EQUAL;

	BlendEnable = true;

	BlendOp = ADD;
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;

	BlendOpAlpha = ADD;
	SrcBlendAlpha = ZERO;
	DestBlendAlpha = ZERO;

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
static const float lineAlpha = 1.0;
static const float lineContrast = 1.5;
static const float lineBrightness[2] = {0.1, 1.0};
static const float lineHardness = 1000.0;

static const float lineThickness = 0.01;
static const float lineNearClip = 0.1;
static const float lineNearFade = 0.1;

static const float tile = 10.0;

struct PI_DrawPlane
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float4 worldPos : POSITION0;
	float4 localPos : POSITION1;
};

float4 GridCascade(float2 wpos, int i)
{
	float2 wposNear = floor(wpos + 0.5);
	float2 edge = 1 - saturate(abs(wposNear - wpos) - lineThickness);
	edge = pow(edge, lineHardness);
	
	float alpha = max(edge.x, edge.y);

	float3 color = float3(edge.y, 0.0, edge.x);
	color = lerp(float3(1,1,1), color, lineContrast) * lineBrightness[i];

	return float4(color, alpha);
}

float4 PlanePS(PI_DrawPlane input) : SV_TARGET
{	
	float4 cascade0 = colorTex.Sample(samplerAnisotropicWrap, input.localPos.xz * 10 + 0.5).r;
	cascade0.a *= lineAlpha;
	if( cascade0.a < 0.1 )
		discard;
	
	cascade0.rgb = lineBrightness[0];

	float3 V = g_CamPos - input.worldPos.xyz;
	float NoV = abs(dot(normalize(V), input.normal));
	NoV = saturate(pow(NoV * 5, 1.0));
	
	float distToCam = length(V);

	float4 cascade1 = GridCascade(input.localPos.xz, 1);

	cascade0.rgb = lerp(cascade0.rgb, cascade1.rgb, cascade1.a);
	
	float distFadeNear = saturate( ( distToCam - lineNearClip) / lineNearFade);
	cascade0.a *= distFadeNear;

	cascade0 = saturate(cascade0);

	return float4(cascade0.rgb, cascade0.a);
}

// vetrex
PI_DrawPlane PlaneVS(VI_Mesh input)
{
	PI_DrawPlane output;
	
    output.position = mul(float4(input.position, 1), worldMatrix);

	output.worldPos = output.position;
	output.localPos = float4(input.position * tile, 1);

    output.position = mul(output.position, g_viewProj);
	
    output.normal = mul(input.normal, (float3x3)normalMatrix);
    output.normal = normalize(output.normal);

	output.tex = input.tex;

    return output;
}