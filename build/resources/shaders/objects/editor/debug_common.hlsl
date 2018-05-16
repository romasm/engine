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
void ProbGS(point VI_Pos input[1], inout TriangleStream<PI_PosTexTBN> outputStream)
{
	PI_PosTexTBN output = (PI_PosTexTBN)0;
	
	float3 normal = normalize(g_CamPos - input[0].position);
	float3 tangent = normalize(cross(normal, g_CamBinormal));
	float3 binormal = normalize(cross(normal, tangent));

	float3 tangentSize = tangent * PROB_SIZE;
	float3 binormalSize = binormal * PROB_SIZE;
		
	float3 wpos = input[0].position + tangentSize - binormalSize;
	output.pos = mul(float4(wpos, 1), g_viewProj);
	output.normal = normal;
	output.tangent = tangent;
	output.binormal = binormal;
	output.tex = float2(1, -1);
	outputStream.Append(output);

	wpos = input[0].position + tangentSize + binormalSize;
	output.pos = mul(float4(wpos, 1), g_viewProj);
	output.normal = normal;
	output.tangent = tangent;
	output.binormal = binormal;
	output.tex = float2(1, 1);
	outputStream.Append(output);

	wpos = input[0].position - tangentSize - binormalSize;
	output.pos = mul(float4(wpos, 1), g_viewProj);
	output.normal = normal;
	output.tangent = tangent;
	output.binormal = binormal;
	output.tex = float2(-1, -1);
	outputStream.Append(output);
	
	wpos = input[0].position - tangentSize + binormalSize;
	output.pos = mul(float4(wpos, 1), g_viewProj);
	output.normal = normal;
	output.tangent = tangent;
	output.binormal = binormal;
	output.tex = float2(-1, 1);
	outputStream.Append(output);

	outputStream.RestartStrip();
}

PO_Gbuffer ProbPS(PI_PosTexTBN input)
{
	float cosNsq = dot(input.tex, input.tex);
	if (cosNsq > 1)
		discard;

	float3 normal = normalize(input.tex.x * input.tangent + input.tex.y * input.binormal + input.normal * sqrt(1 - cosNsq));
	float3 nTangent = normalize(cross(normal, input.binormal));

	const float3 albedo = 0.8;
	const float roughness = 0.8;

	PO_Gbuffer res = (PO_Gbuffer)0;
	res.albedo_roughY = float4(albedo, roughness);
	res.tbn = EncodeTBNasFloat4(normal, nTangent);
	res.vnormXY = input.normal.xy;
	res.spec_roughX = float4(0.04, 0.04, 0.04, roughness);
	res.emiss_vnormZ = float4(0, 0, 0, input.normal.z);
	res.ao = 1.0;

	return res;
}