TECHNIQUE_DEFAULT
{
	Queue = SC_OPAQUE;

	VertexShader = "VS";
	PixelShader = "PS";
}

TECHNIQUE_SHADOW
{
	Queue = SC_OPAQUE;

	VertexShader = "VS";
	PixelShader = NULL;
}

#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"

PI_Pos VS(VI_Pos input)
{
	PI_Pos output;
	output.position = float4(input.position, 1);
	return output;
}

float4 PS(PI_Pos input) : SV_TARGET
{
	return 0;
}