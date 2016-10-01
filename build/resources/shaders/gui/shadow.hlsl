TECHNIQUE_DEFAULT
{
	Queue = GUI_2D;

	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "PS";

	BlendEnable = true;
	BlendOp = ADD;
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"

cbuffer materialBuffer : register(b0)
{
    float4 _remove;
    float4 shadowData;
    float4 shadowColor;
};

float4 PS(PI_PosTex input) : SV_TARGET
{
	float4 color = shadowColor;
	
	if( shadowData.r == 0.0 )
		discard;
		
	float horz_coord = input.tex.x;
	if(input.tex.x > shadowData.r)
		horz_coord = 1 - input.tex.x;
		
	float vert_coord = input.tex.y;
	if(input.tex.y > shadowData.g)
		vert_coord = 1 - input.tex.y;
	
	float horz_shadow = horz_coord / shadowData.r;
	float vert_shadow = vert_coord / shadowData.g;
	
	if(horz_shadow >= 1.0 && vert_shadow >= 1.0)
		discard;

	static const float4 null_color = float4(0.0, 0.0, 0.0, 0.0);
	
	if(horz_shadow < 1.0)
		color = lerp(color, null_color, cos(horz_shadow * PIDIV2));
	if(vert_shadow < 1.0)
		color = lerp(color, null_color, cos(vert_shadow * PIDIV2));

	return color;
}