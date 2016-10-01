TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "SMAAFinal";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

Texture2D colorTex : register(t0);
Texture2D blendTex : register(t1);

SamplerState samplerBilinearClamp : register(s0);

float4 SMAAFinal(PI_PosTex input) : SV_TARGET
{
	float2 texcoord = input.tex;

	float4 offset[2];
	offset[0] = texcoord.xyxy + g_PixSize.xyxy * float4(-1.0, 0.0, 0.0, -1.0);
    offset[1] = texcoord.xyxy + g_PixSize.xyxy * float4( 1.0, 0.0, 0.0,  1.0);
	
	// Fetch the blending weights for current pixel:
    float4 a;
    a.xz = blendTex.Sample(samplerBilinearClamp, texcoord).xz;
    a.y = blendTex.Sample(samplerBilinearClamp, offset[1].zw).g;
    a.w = blendTex.Sample(samplerBilinearClamp, offset[1].xy).a;

    // Is there any blending weight with a value greater than 0.0?
    [branch]
    if (dot(a, float4(1.0, 1.0, 1.0, 1.0)) < 1e-5)
        return float4(colorTex.SampleLevel(samplerBilinearClamp, texcoord, 0).rgb, 1);
    else 
	{
        float4 color = float4(0.0, 0.0, 0.0, 0.0);

        // Up to 4 lines can be crossing a pixel (one through each edge). We
        // favor blending by choosing the line with the maximum weight for each
        // direction:
        float2 offset;
        offset.x = a.a > a.b ? a.a : -a.b; // left vs. right 
        offset.y = a.g > a.r ? a.g : -a.r; // top vs. bottom

        // Then we go in the direction that has the maximum weight:
        if (abs(offset.x) > abs(offset.y)) // horizontal vs. vertical
            offset.y = 0.0;
        else
            offset.x = 0.0;
		
		
        // Fetch the opposite color and lerp by hand:
        float4 C = colorTex.SampleLevel(samplerBilinearClamp, texcoord, 0);
        texcoord += sign(offset) * g_PixSize;
        float4 Cop = colorTex.SampleLevel(samplerBilinearClamp, texcoord, 0);
        float s = abs(offset.x) > abs(offset.y)? abs(offset.x) : abs(offset.y);
		
        float4 res = lerp(C, Cop, s);
		
		
		// We exploit bilinear filtering to mix current pixel with the chosen
        // neighbor:
        //texcoord += offset * g_PixSize;
        //float3 res = colorTex.SampleLevel(samplerBilinearClamp, texcoord, 0).rgb;
		
		return float4(res.rgb, 1);
    }
}