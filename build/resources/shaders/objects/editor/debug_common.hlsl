#include "../../common/math.hlsl"
#include "../../common/structs.hlsl"
#include "../../common/shared.hlsl"

PI_PosColor LineColorVS(VI_PosColor input)
{
	PI_PosColor output;
	
	output.pos = mul(float4(input.pos, 1), g_viewProj);
	output.color = input.color;
	
	return output;
}

float4 LineColorPS(PI_PosColor input) : SV_TARGET
{
	return float4(input.color, 1);
}

VI_Pos ProbVS(VI_Pos input)
{
	VI_Pos output;
	output.position = input.position;
	return output;
}

#define PROB_SIZE 0.02

[maxvertexcount(4)]
void ProbGS(point VI_Pos input[1], inout TriangleStream<PI_PosTexNorm> outputStream)
{
	PI_PosTexNorm output = (PI_PosTexNorm)0;
	
	float3 normal = normalize(g_CamPos - input[0].position);
	float3 tangent = normalize(cross(normal, g_CamBinormal));
	float3 binormal = normalize(cross(normal, tangent));

	tangent *= PROB_SIZE;
	binormal *= PROB_SIZE;
		
	float3 wpos = input[0].position + tangent - binormal;
	output.pos = mul(float4(wpos, 1), g_viewProj);
	output.normal = normal;
	output.tex = float2(1, -1);
	outputStream.Append(output);

	wpos = input[0].position + tangent + binormal;
	output.pos = mul(float4(wpos, 1), g_viewProj);
	output.normal = normal;
	output.tex = float2(1, 1);
	outputStream.Append(output);

	wpos = input[0].position - tangent - binormal;
	output.pos = mul(float4(wpos, 1), g_viewProj);
	output.normal = normal;
	output.tex = float2(-1, -1);
	outputStream.Append(output);
	
	wpos = input[0].position - tangent + binormal;
	output.pos = mul(float4(wpos, 1), g_viewProj);
	output.normal = normal;
	output.tex = float2(-1, 1);
	outputStream.Append(output);

	outputStream.RestartStrip();
}

float4 ProbPS(PI_PosTexNorm input) : SV_TARGET
{
	if (dot(input.tex, input.tex) > 1)
		discard;

	return float4(1, 1, 1, 1);
}