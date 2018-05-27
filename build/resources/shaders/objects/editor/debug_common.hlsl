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

VI_PosTex ProbVS(VI_PosTex input)
{
	VI_PosTex output;
	output.pos = input.pos;
	output.tex = input.tex;
	return output;
}

#define PROB_SIZE 0.02

struct PI_PosTexTBNProb
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
	float3 normal			: NORMAL;
	float3 tangent			: TANGENT;
	float3 binormal			: BINORMAL;
	float2 debugData		: TEXCOORD1;
};

[maxvertexcount(4)]
void ProbGS(point VI_PosTex input[1], inout TriangleStream<PI_PosTexTBNProb> outputStream)
{
	PI_PosTexTBNProb output = (PI_PosTexTBNProb)0;
	
	float3 normal = normalize(g_CamPos - input[0].pos);
	float3 tangent = normalize(cross(normal, g_CamBinormal));
	float3 binormal = normalize(cross(normal, tangent));

	float3 tangentSize = tangent * PROB_SIZE;
	float3 binormalSize = binormal * PROB_SIZE;
		
	output.debugData = input[0].tex;
	output.normal = normal;
	output.tangent = tangent;
	output.binormal = binormal;

	float3 wpos = input[0].pos + tangentSize - binormalSize;
	output.pos = mul(float4(wpos, 1), g_viewProj);
	output.tex = float2(1, -1);
	outputStream.Append(output);

	wpos = input[0].pos + tangentSize + binormalSize;
	output.pos = mul(float4(wpos, 1), g_viewProj);
	output.tex = float2(1, 1);
	outputStream.Append(output);

	wpos = input[0].pos - tangentSize - binormalSize;
	output.pos = mul(float4(wpos, 1), g_viewProj);
	output.tex = float2(-1, -1);
	outputStream.Append(output);
	
	wpos = input[0].pos - tangentSize + binormalSize;
	output.pos = mul(float4(wpos, 1), g_viewProj);
	output.tex = float2(-1, 1);
	outputStream.Append(output);

	outputStream.RestartStrip();
}

PO_Gbuffer ProbPS(PI_PosTexTBNProb input)
{
	float cosNsq = dot(input.tex, input.tex);
	if (cosNsq > 1)
		discard;

	float3 normal = normalize(input.tex.x * input.tangent + input.tex.y * input.binormal + input.normal * sqrt(1 - cosNsq));
	float3 nTangent = normalize(cross(normal, input.binormal));

	const float3 albedo = 1.0;
	const float3 albedoInterpolated = float3(0.0, 0.0, 1.0);
	const float3 albedoError = float3(1.0, 0.0, 0.0);
	const float roughness = 0.8;

	float3 finalAlbedo = lerp(albedo, albedoInterpolated, input.debugData.x);
	finalAlbedo = lerp(finalAlbedo, albedoError, input.debugData.y);

	PO_Gbuffer res = (PO_Gbuffer)0;
	res.albedo_roughY = float4(finalAlbedo, roughness);
	res.tbn = EncodeTBNasFloat4(normal, nTangent);
	res.vnormXY = input.normal.xy;
	res.spec_roughX = float4(0.04, 0.04, 0.04, roughness);
	res.emiss_vnormZ = float4(0, 0, 0, input.normal.z);
	res.ao = 1.0;

	return res;
}