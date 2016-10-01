TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "SMAAEdge";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

#define COLOR_DETECT 0

Texture2D colorTex : register(t0);

SamplerState samplerBilinearClamp : register(s0);

cbuffer materialBuffer : register(b1)
{
	float thresholdVal;
	float _padding0;
	float _padding1;
	float _padding2;
};

#if COLOR_DETECT == 0

float4 SMAAEdge(PI_PosTex input) : SV_TARGET
{
	const float2 texcoord = input.tex;
	const float2 threshold = float2(thresholdVal, thresholdVal);
	
	float4 offset[3];
	offset[0] = texcoord.xyxy + g_PixSize.xyxy * float4(-1.0, 0.0, 0.0, -1.0);
    offset[1] = texcoord.xyxy + g_PixSize.xyxy * float4( 1.0, 0.0, 0.0,  1.0);
    offset[2] = texcoord.xyxy + g_PixSize.xyxy * float4(-2.0, 0.0, 0.0, -2.0);
	
    // Calculate lumas:
    const float3 weights = float3(0.2126, 0.7152, 0.0722);
    float L = dot(colorTex.Sample(samplerBilinearClamp, texcoord).rgb, weights);
    float Lleft = dot(colorTex.Sample(samplerBilinearClamp, offset[0].xy).rgb, weights);
    float Ltop  = dot(colorTex.Sample(samplerBilinearClamp, offset[0].zw).rgb, weights);

    // We do the usual threshold:
    float4 delta;
    delta.xy = abs(L - float2(Lleft, Ltop));
    float2 edges = step(threshold, delta.xy);

    // Then discard if there is no edge:
    if (dot(edges, float2(1.0, 1.0)) == 0.0)
        discard;

    // Calculate right and bottom deltas:
    float Lright = dot(colorTex.Sample(samplerBilinearClamp, offset[1].xy).rgb, weights);
    float Lbottom  = dot(colorTex.Sample(samplerBilinearClamp, offset[1].zw).rgb, weights);
    delta.zw = abs(L - float2(Lright, Lbottom));

    // Calculate the maximum delta in the direct neighborhood:
    float2 maxDelta = max(delta.xy, delta.zw);
    maxDelta = max(maxDelta.xx, maxDelta.yy);

    // Calculate left-left and top-top deltas:
    float Lleftleft = dot(colorTex.Sample(samplerBilinearClamp, offset[2].xy).rgb, weights);
    float Ltoptop = dot(colorTex.Sample(samplerBilinearClamp, offset[2].zw).rgb, weights);
    delta.zw = abs(float2(Lleft, Ltop) - float2(Lleftleft, Ltoptop));

    // Calculate the final maximum delta:
    maxDelta = max(maxDelta.xy, delta.zw);
	
    edges.xy *= step(0.5 * maxDelta, delta.xy);

    return float4(edges, 0.0, 0.0);
}

#else

float4 SMAA_Edge(PI_PosTex input) : SV_TARGET
{
    const float2 texcoord = input.tex;
	const float2 threshold = float2(thresholdVal, thresholdVal);

	float4 offset[3];
	offset[0] = texcoord.xyxy + g_PixSize.xyxy * float4(-1.0, 0.0, 0.0, -1.0);
    offset[1] = texcoord.xyxy + g_PixSize.xyxy * float4( 1.0, 0.0, 0.0,  1.0);
    offset[2] = texcoord.xyxy + g_PixSize.xyxy * float4(-2.0, 0.0, 0.0, -2.0);
	
    // Calculate color deltas:
    float4 delta;
    float3 C = colorTex.Sample(samplerBilinearClamp, texcoord).rgb;

    float3 Cleft = colorTex.Sample(samplerBilinearClamp, offset[0].xy).rgb;
    float3 t = abs(C - Cleft);
    delta.x = max(max(t.r, t.g), t.b);

    float3 Ctop  = colorTex.Sample(samplerBilinearClamp, offset[0].zw).rgb;
    t = abs(C - Ctop);
    delta.y = max(max(t.r, t.g), t.b);

    // We do the usual threshold:
    float2 edges = step(threshold, delta.xy);

    // Then discard if there is no edge:
    if (dot(edges, float2(1.0, 1.0)) == 0.0)
        discard;

    // Calculate right and bottom deltas:
    float3 Cright = colorTex.Sample(samplerBilinearClamp, offset[1].xy).rgb;
    t = abs(C - Cright);
    delta.z = max(max(t.r, t.g), t.b);

    float3 Cbottom  = colorTex.Sample(samplerBilinearClamp, offset[1].zw).rgb;
    t = abs(C - Cbottom);
    delta.w = max(max(t.r, t.g), t.b);

    // Calculate the maximum delta in the direct neighborhood:
    float maxDelta = max(max(max(delta.x, delta.y), delta.z), delta.w);

    // Calculate left-left and top-top deltas:
    float3 Cleftleft  = colorTex.Sample(samplerBilinearClamp, offset[2].xy).rgb;
    t = abs(C - Cleftleft);
    delta.z = max(max(t.r, t.g), t.b);

    float3 Ctoptop = colorTex.Sample(samplerBilinearClamp, offset[2].zw).rgb;
    t = abs(C - Ctoptop);
    delta.w = max(max(t.r, t.g), t.b);

    // Calculate the final maximum delta:
    maxDelta = max(max(maxDelta, delta.z), delta.w);

    // Local contrast adaptation in action:
    edges.xy *= step(0.5 * maxDelta, delta.xy);

    return float4(edges, 0.0, 0.0);
}

#endif