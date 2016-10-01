TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "PS";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"

cbuffer materialBuffer : register(b0)
{
    float4 clip;
};

#define T_START 1000
#define T_END 15000

#define RCP255 1.0f / 255.0f

float3 TtoRGB(float Temperature)
{
    float t = Temperature / 100.0f;
    float r, g, b;

    if(t <= 66)
	{
        r = 255;

        g = t;
        g = 99.4708025861 * log(g) - 161.1195681661;
		g = clamp(g, 0, 255);

		if(t <= 19)
		{
			b = 0;
		}
		else
		{
			b = t - 10;
			b = 138.5177312231 * log(b) - 305.0447927307;
			b = clamp(b, 0, 255);
		}
	}
    else
	{
        r = t - 60;
        r = 329.698727466 * pow(abs(r), -0.1332047592);
        r = clamp(r, 0, 255);

        g = t - 60;
		g = 288.1221695283 * pow(abs(g), -0.0755148492);
		g = clamp(g, 0, 255);

		b = 255;
	}

    return float3(r * RCP255, g * RCP255, b * RCP255);
}

float4 PS(PI_PosTex input) : SV_TARGET
{
	if( input.pos.x < clip.r ||
		input.pos.y < clip.g ||
		input.pos.x > clip.b ||
		input.pos.y > clip.a )
		discard;

	float3 color = TtoRGB(input.tex.x * (T_END - T_START) + T_START);	
	return float4(color, 1);
}