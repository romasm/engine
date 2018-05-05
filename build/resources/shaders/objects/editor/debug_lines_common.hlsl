#include "../../common/math.hlsl"
#include "../../common/structs.hlsl"
#include "../../common/shared.hlsl"

PI_PosColor VS(VI_PosColor input)
{
	PI_PosColor output;
	
	output.pos = mul(float4(input.pos, 1), g_viewProj);
	output.color = input.color;
	
	return output;
}

float4 PS(PI_PosColor input) : SV_TARGET
{
	return float4(input.color, 1);
}