#include "../common/shared.hlsl"
#include "../common/structs.hlsl"

PI_PosTex Main(VI_PosTex input)
{
    PI_PosTex output;
    output.pos = float4(input.pos, 1);    
	output.tex = input.tex;
    return output;
}