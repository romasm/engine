TECHNIQUE_DEFAULT
{
	Queue = GUI_2D_FONT;

	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "Viewport";

	BlendEnable = true;
	BlendOp = ADD;
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"

Texture2DArray shaderTexture : register(t0);
SamplerState samplerPointWrap : register(s0);

cbuffer materialBuffer : register(b0)
{
    float4 clip;

	float srgb;
	float mulColor;
	float isMulAlpha;
	float mulAlpha;

	float outAlpha;
	float powVal;
	float _padding0;
	float _padding1;
};

float4 Viewport(PI_PosTex input) : SV_TARGET
{
	if( input.pos.x < clip.r ||
		input.pos.y < clip.g ||
		input.pos.x > clip.b ||
		input.pos.y > clip.a )
		discard;
		
	float tex_w;
	float tex_h;
	float tex_e;
	shaderTexture.GetDimensions(tex_w, tex_h, tex_e);
		
	float3 texCoord;
	float sqrt_elements = sqrt(tex_e);
	uint2 dims;
	dims.x = uint(ceil(sqrt_elements));
	dims.y = uint(round(sqrt_elements));
	float2 rcpDims = 1.0f / dims;
	
	float2 cellPos = trunc(input.tex / rcpDims);
	texCoord.z = float(uint(cellPos.y * dims.x + cellPos.x));
	texCoord.z = min(texCoord.z, tex_e);
	texCoord.xy = (input.tex - cellPos * rcpDims) * dims;
	
	float4 color = shaderTexture.Sample(samplerPointWrap, texCoord);
	if(outAlpha > 0)
		color.rgb = color.a;

	color.rgb *= mulColor;

	if(powVal != 1.0)
		color.rgb = PowAbs(color.rgb, powVal);

	if(isMulAlpha > 0)
		color.a *= mulAlpha;
	else
		color.a = mulAlpha;
		
	if(srgb > 0)
		color.rgb = LinearToSRGB(color.rgb);
		
	return saturate(color);
}
