TECHNIQUE_DEFAULT
{
	Queue = GUI_3D;

	DepthEnable = true;
	DepthWrite = true;
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

// config
static const float lineAlpha[3] = {0.5, 0.7, 0.5};
static const float lineContrast[3] = {0.0, 0.5, 0.0};
static const float lineBrightness[3] = {0.4, 0.8, 0.5};
static const float lineHardness = 1000.0;

static const float lineThickness = 0.003;
static const float lineThicknessDist = 0.05;
static const float lineNearClip = 0.15;
static const float lineNearFade = 0.2;
static const float lineFarClip[3] = {3.0, 15, 150};
static const float lineFarFade[3] = {3.0, 15, 150};

float4 GridCascade(float2 wpos, float distToCam, int i)
{
	float distFadeFar = 1 - saturate( ( distToCam - lineFarClip[i]) / lineFarFade[i]);
	float distFadeNear = saturate( ( distToCam - lineNearClip) / lineNearFade);

	float2 wposNear = floor(wpos + 0.5);
	float targetThickness = lerp(lineThickness, lineThicknessDist, saturate(distToCam / lineFarClip[i]));
	float2 edge = 1 - saturate(abs(wposNear - wpos) - targetThickness);
	edge = pow(edge, lineHardness);
	
	float alpha = lineAlpha[i] * max(edge.x, edge.y) * distFadeFar * distFadeNear;

	float3 color = float3(edge.y, 0.0, edge.x);
	color = lerp(float3(1,1,1), color, lineContrast[i]) * lineBrightness[i];

	return float4(color, alpha);
}

float4 PlanePS(PI_ToolMesh input) : SV_TARGET
{	
	float3 V = g_CamPos - input.worldPos.xyz;
	float NoV = abs(dot(normalize(V), input.normal));
	NoV = saturate(pow(NoV * 5, 1.0));
	
	float distToCam = length(V);

	float4 cascade0 = GridCascade(input.worldPos.xz * 10, distToCam, 0);
	float4 cascade1 = GridCascade(input.worldPos.xz, distToCam, 1);

	cascade0 *= 1 - (cascade1.a > 0.1 ? 1 : 0);
	cascade0 += cascade1;
	cascade0.a *= NoV;
	return saturate(cascade0);
}

// vetrex
cbuffer matrixBuffer : register(b2)
{
	matrix worldMatrix;
	matrix normalMatrix;
};

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