#include "../common/math.hlsl"
#include "../common/structs.hlsl"

cbuffer matrixBuffer : register(b0)
{
	matrix WVP;
};

PI_PosTex Main(VI_PosTex input)
{
    PI_PosTex output;

    output.pos = mul(float4(input.pos, 1), WVP);    
	output.tex = input.tex;
    
    return output;
}