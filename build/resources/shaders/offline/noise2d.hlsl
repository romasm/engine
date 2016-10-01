TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "Noise2D";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"

#define RES 512

float4 Noise2D(PI_PosTex input) : SV_TARGET
{
	float2 rnd_vect = 0;		
	uint2 screen_pos = uint2(RES * input.tex.x, RES * input.tex.y);
	uint2 Random = ScrambleTEA( screen_pos, 10);
	rnd_vect.x = ( (Random.x & 0xffff ) / (float)(0xffff) ); 
	rnd_vect.y = ( (Random.y & 0xffff ) / (float)(0xffff) );

	return float4(rnd_vect.x, rnd_vect.y, 0, 0);
}
