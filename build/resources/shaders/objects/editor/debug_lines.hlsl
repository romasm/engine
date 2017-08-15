TECHNIQUE_DEFAULT
{
	Queue = GUI_3D;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;

	FillMode = WIREFRAME;
	CullMode = NONE;

	VertexShader = "VS";
	PixelShader = "PS";
}

#include "../../common/math.hlsl"
#include "../../common/structs.hlsl"
#include "../../common/shared.hlsl"

PI_PosColor VS(VI_PosColor input)
{
	PI_PosColor output;
	
	output.position = mul(float4(input.position, 1), world);
	output.position = mul(output.position, g_viewProj);

	output.color = input.color;
	
	return output;
}

float4 PS(PI_PosColor input) : SV_TARGET
{
	return input.color;
}