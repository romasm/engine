TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "SMAABlend";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

Texture2D areaTex : register(t0);
Texture2D searchTex : register(t1);
Texture2D edgesTex : register(t2);

SamplerState samplerPointClamp : register(s0);
SamplerState samplerBilinearClamp : register(s1);

cbuffer materialBuffer : register(b1)
{
	float search_steps;
	float search_steps_diag;
	float corner_rounding;
	float subIdx0;

	float subIdx1;
	float subIdx2;
	float subIdx3;
	float _padding0;
};

#define SMAA_AREATEX_MAX_DISTANCE 16.0f
#define SMAA_AREATEX_MAX_DISTANCE_DIAG 20.0f
#define SMAA_AREATEX_PIXEL_SIZE (1.0 / float2(160.0, 560.0))
#define SMAA_AREATEX_SUBTEX_SIZE (1.0 / 7.0)

float SearchDiag1(Texture2D edgesTex, float2 texcoord, float2 dir, float c)
{
    texcoord += dir * g_PixSize;
    float2 e = float2(0.0, 0.0);
    float i;
    for (i = 0.0; i < search_steps_diag; i++) {
        e.rg = edgesTex.SampleLevel(samplerBilinearClamp, texcoord, 0).rg;
        
		if (dot(e, float2(1.0, 1.0)) < 1.9) break;
		
        texcoord += dir * g_PixSize;
    }
    return i + float(e.g > 0.9) * c;
}

float SearchDiag2(Texture2D edgesTex, float2 texcoord, float2 dir, float c)
{
    texcoord += dir * g_PixSize;
    float2 e = float2(0.0, 0.0);
    float i;
    for (i = 0.0; i < search_steps_diag; i++) {
        e.g = edgesTex.SampleLevel(samplerBilinearClamp, texcoord, 0).g;
        e.r = edgesTex.SampleLevel(samplerBilinearClamp, texcoord, 0, int2(1, 0)).r;
        
		if (dot(e, float2(1.0, 1.0)) < 1.9) break;
		
        texcoord += dir * g_PixSize;
    }
    return i + float(e.g > 0.9) * c;
}

float2 AreaDiag(Texture2D areaTex, float2 dist, float2 e, float offset)
{
    float2 texcoord = SMAA_AREATEX_MAX_DISTANCE_DIAG * e + dist;

    // We do a scale and bias for mapping to texel space:
    texcoord = SMAA_AREATEX_PIXEL_SIZE * texcoord + (0.5 * SMAA_AREATEX_PIXEL_SIZE);

    // Diagonal areas are on the second half of the texture:
    texcoord.x += 0.5;

    // Move to proper place, according to the subpixel offset:
    texcoord.y += SMAA_AREATEX_SUBTEX_SIZE * offset;

    // Do it!
    return areaTex.SampleLevel(samplerBilinearClamp, texcoord, 0).rg;
}

float2 CalculateDiagWeights(Texture2D edgesTex, Texture2D areaTex, float2 texcoord, float2 e, float4 subsampleIndices)
{
    float2 weights = float2(0.0, 0.0);

    float2 d;
    d.x = e.r > 0.0? SearchDiag1(edgesTex, texcoord, float2(-1.0,  1.0), 1.0) : 0.0;
    d.y = SearchDiag1(edgesTex, texcoord, float2(1.0, -1.0), 0.0);

    [branch]
    if (d.r + d.g > 2.0) { // d.r + d.g + 1 > 3
        float4 coords = mad(float4(-d.r, d.r, d.g, -d.g), g_PixSize.xyxy, texcoord.xyxy);

        float4 c;
        c.x = edgesTex.SampleLevel(samplerBilinearClamp, coords.xy, 0, int2(-1,  0)).g;
        c.y = edgesTex.SampleLevel(samplerBilinearClamp, coords.xy, 0, int2( 0,  0)).r;
        c.z = edgesTex.SampleLevel(samplerBilinearClamp, coords.zw, 0, int2( 1,  0)).g;
        c.w = edgesTex.SampleLevel(samplerBilinearClamp, coords.zw, 0, int2( 1, -1)).r;
        float2 e = 2.0 * c.xz + c.yw;
        float t = float(search_steps_diag) - 1.0;
        e *= step(d.rg, float2(t, t));

        weights += AreaDiag(areaTex, d, e, subsampleIndices.z);
    }

    d.x = SearchDiag2(edgesTex, texcoord, float2(-1.0, -1.0), 0.0);
    float right = edgesTex.SampleLevel(samplerBilinearClamp, texcoord, 0, int2(1, 0)).r;
    d.y = right > 0.0? SearchDiag2(edgesTex, texcoord, float2(1.0, 1.0), 1.0) : 0.0;

    [branch]
    if (d.r + d.g > 2.0) { // d.r + d.g + 1 > 3
        float4 coords = mad(float4(-d.r, -d.r, d.g, d.g), g_PixSize.xyxy, texcoord.xyxy);

        float4 c;
        c.x  = edgesTex.SampleLevel(samplerBilinearClamp, coords.xy, 0, uint2(-1,  0)).g;
        c.y  = edgesTex.SampleLevel(samplerBilinearClamp, coords.xy, 0, uint2( 0, -1)).r;
        c.zw = edgesTex.SampleLevel(samplerBilinearClamp, coords.zw, 0, uint2( 1,  0)).gr;
        float2 e = 2.0 * c.xz + c.yw;
        float t = float(search_steps_diag) - 1.0;
        e *= step(d.rg, float2(t, t));

        weights += AreaDiag(areaTex, d, e, subsampleIndices.w).gr;
    }

    return weights;
}

float SearchLength(Texture2D searchTex, float2 e, float bias, float scale)
{
    e.r = bias + e.r * scale;
    return 255.0 * searchTex.SampleLevel(samplerPointClamp, e, 0).r;
}

float SearchXLeft(Texture2D edgesTex, Texture2D searchTex, float2 texcoord, float end)
{
    /**
     * @PSEUDO_GATHER4
     * This texcoord has been offset by (-0.25, -0.125) in the vertex shader to
     * sample between edge, thus fetching four edges in a row.
     * Sampling with different offsets in each direction allows to disambiguate
     * which edges are active from the four fetched ones.
     */
    float2 e = float2(0.0, 1.0);
    while (texcoord.x > end && 
           e.g > 0.8281 && // Is there some edge not activated?
           e.r == 0.0) { // Or is there a crossing edge that breaks the line?
        e = edgesTex.SampleLevel(samplerBilinearClamp, texcoord, 0).rg;
        texcoord -= float2(2.0, 0.0) * g_PixSize;
    }

    // We correct the previous (-0.25, -0.125) offset we applied:
    texcoord.x += 0.25 * g_PixSize.x;

    // The searches are bias by 1, so adjust the coords accordingly:
    texcoord.x += g_PixSize.x;

    // Disambiguate the length added by the last step:
    texcoord.x += 2.0 * g_PixSize.x; // Undo last step
    texcoord.x -= g_PixSize.x * SearchLength(searchTex, e, 0.0, 0.5);

    return texcoord.x;
}

float SearchXRight(Texture2D edgesTex, Texture2D searchTex, float2 texcoord, float end)
{
    float2 e = float2(0.0, 1.0);
    while (texcoord.x < end && 
           e.g > 0.8281 && // Is there some edge not activated?
           e.r == 0.0) { // Or is there a crossing edge that breaks the line?
        e = edgesTex.SampleLevel(samplerBilinearClamp, texcoord, 0).rg;
        texcoord += float2(2.0, 0.0) * g_PixSize;
    }

    texcoord.x -= 0.25 * g_PixSize.x;
    texcoord.x -= g_PixSize.x;
    texcoord.x -= 2.0 * g_PixSize.x;
    texcoord.x += g_PixSize.x * SearchLength(searchTex, e, 0.5, 0.5);
    return texcoord.x;
}

float SearchYUp(Texture2D edgesTex, Texture2D searchTex, float2 texcoord, float end)
{
    float2 e = float2(1.0, 0.0);
    while (texcoord.y > end && 
           e.r > 0.8281 && // Is there some edge not activated?
           e.g == 0.0) { // Or is there a crossing edge that breaks the line?
        e = edgesTex.SampleLevel(samplerBilinearClamp, texcoord, 0).rg;
        texcoord -= float2(0.0, 2.0) * g_PixSize;
    }

    texcoord.y += 0.25 * g_PixSize.y;
    texcoord.y += g_PixSize.y;
    texcoord.y += 2.0 * g_PixSize.y;
    texcoord.y -= g_PixSize.y * SearchLength(searchTex, e.gr, 0.0, 0.5);
    return texcoord.y;
}

float SearchYDown(Texture2D edgesTex, Texture2D searchTex, float2 texcoord, float end)
{
    float2 e = float2(1.0, 0.0);
    while (texcoord.y < end && 
           e.r > 0.8281 && // Is there some edge not activated?
           e.g == 0.0) { // Or is there a crossing edge that breaks the line?
        e = edgesTex.SampleLevel(samplerBilinearClamp, texcoord, 0).rg;
        texcoord += float2(0.0, 2.0) * g_PixSize;
    }
    
    texcoord.y -= 0.25 * g_PixSize.y;
    texcoord.y -= g_PixSize.y;
    texcoord.y -= 2.0 * g_PixSize.y;
    texcoord.y += g_PixSize.y * SearchLength(searchTex, e.gr, 0.5, 0.5);
    return texcoord.y;
}

float2 Area(Texture2D areaTex, float2 dist, float e1, float e2, float offset)
{
    // Rounding prevents precision errors of bilinear filtering:
    float2 texcoord = SMAA_AREATEX_MAX_DISTANCE * round(4.0 * float2(e1, e2)) + dist;
    
    // We do a scale and bias for mapping to texel space:
    texcoord = SMAA_AREATEX_PIXEL_SIZE * texcoord + (0.5 * SMAA_AREATEX_PIXEL_SIZE);

    // Move to proper place, according to the subpixel offset:
    texcoord.y += SMAA_AREATEX_SUBTEX_SIZE * offset;

    // Do it!
    return areaTex.SampleLevel(samplerBilinearClamp, texcoord, 0).rg;
}

void DetectHorizontalCornerPattern(Texture2D edgesTex, inout float2 weights, float2 texcoord, float2 d)
{
    float4 coords = mad(float4(d.x, 0.0, d.y, 0.0), g_PixSize.xyxy, texcoord.xyxy);
    float2 e;
    e.r = edgesTex.SampleLevel(samplerBilinearClamp, coords.xy, 0, int2(0.0,  1.0)).r;
    bool left = abs(d.x) < abs(d.y);
    e.g = edgesTex.SampleLevel(samplerBilinearClamp, coords.xy, 0, int2(0.0, -2.0)).r;
    if (left) weights *= saturate(corner_rounding / 100.0 + 1.0 - e);

    e.r = edgesTex.SampleLevel(samplerBilinearClamp, coords.zw, 0, int2(1.0,  1.0)).r;
    e.g = edgesTex.SampleLevel(samplerBilinearClamp, coords.zw, 0, int2(1.0, -2.0)).r;
    if (!left) weights *= saturate(corner_rounding / 100.0 + 1.0 - e);
}

void DetectVerticalCornerPattern(Texture2D edgesTex, inout float2 weights, float2 texcoord, float2 d)
{
    float4 coords = mad(float4(0.0, d.x, 0.0, d.y), g_PixSize.xyxy, texcoord.xyxy);
    float2 e;
    e.r = edgesTex.SampleLevel(samplerBilinearClamp, coords.xy, 0, int2( 1.0, 0.0)).g;
    bool left = abs(d.x) < abs(d.y);
    e.g = edgesTex.SampleLevel(samplerBilinearClamp, coords.xy, 0, int2(-2.0, 0.0)).g;
    if (left) weights *= saturate(corner_rounding / 100.0 + 1.0 - e);

    e.r = edgesTex.SampleLevel(samplerBilinearClamp, coords.zw, 0, int2( 1.0, 1.0)).g;
    e.g = edgesTex.SampleLevel(samplerBilinearClamp, coords.zw, 0, int2(-2.0, 1.0)).g;
    if (!left) weights *= saturate(corner_rounding / 100.0 + 1.0 - e);
}

float4 SMAABlend(PI_PosTex input) : SV_TARGET
{
	const float2 pixcoord = input.pos.xy;
	const float2 texcoord = input.tex;

	const float4 subsampleIndices = float4(subIdx0, subIdx1, subIdx2, subIdx3);
	
	float4 offset[3];
    // We will use these offsets for the searches later on (see @PSEUDO_GATHER4):
    offset[0] = texcoord.xyxy + g_PixSize.xyxy * float4(-0.25, -0.125,  1.25, -0.125);
    offset[1] = texcoord.xyxy + g_PixSize.xyxy * float4(-0.125, -0.25, -0.125,  1.25);

    // And these for the searches, they indicate the ends of the loops:
    offset[2] = float4(offset[0].xz, offset[1].yw) + float4(-2.0, 2.0, -2.0, 2.0) * g_PixSize.xxyy * search_steps;
	
    float4 weights = float4(0.0, 0.0, 0.0, 0.0);
	
    float2 e = edgesTex.Sample(samplerBilinearClamp, texcoord).rg;
	
    [branch]
    if (e.g > 0.0) 
	{ 	// Edge at north
		
        // Diagonals have both north and west edges, so searching for them in
        // one of the boundaries is enough.
        weights.rg = CalculateDiagWeights(edgesTex, areaTex, texcoord, e, subsampleIndices);

        // We give priority to diagonals, so if we find a diagonal we skip 
        // horizontal/vertical processing.
        [branch]
        if (dot(weights.rg, float2(1.0, 1.0)) == 0.0)
		{
			float2 d;

			// Find the distance to the left:
			float2 coords;
			coords.x = SearchXLeft(edgesTex, searchTex, offset[0].xy, offset[2].x);
			coords.y = offset[1].y; // offset[1].y = texcoord.y - 0.25 * SMAA_PIXEL_SIZE.y (@CROSSING_OFFSET)
			d.x = coords.x;

			// Now fetch the left crossing edges, two at a time using bilinear
			// filtering. Sampling at -0.25 (see @CROSSING_OFFSET) enables to
			// discern what value each edge has:
			float e1 = edgesTex.SampleLevel(samplerBilinearClamp, coords, 0).r;

			// Find the distance to the right:
			coords.x = SearchXRight(edgesTex, searchTex, offset[0].zw, offset[2].y);
			d.y = coords.x;

			// We want the distances to be in pixel units (doing this here allow to
			// better interleave arithmetic and memory accesses):
			d = d / g_PixSize.x - pixcoord.x;

			// SMAAArea below needs a sqrt, as the areas texture is compressed 
			// quadratically:
			float2 sqrt_d = sqrt(abs(d));

			// Fetch the right crossing edges:
			float e2 = edgesTex.SampleLevel(samplerBilinearClamp, coords, 0, uint2(1, 0)).r;

			// Ok, we know how this pattern looks like, now it is time for getting
			// the actual area:
			weights.rg = Area(areaTex, sqrt_d, e1, e2, subsampleIndices.y);

			// Fix corners:
			DetectHorizontalCornerPattern(edgesTex, weights.rg, texcoord, d);
		}
		else
            e.r = 0.0; // Skip vertical processing.
    }

    [branch]
    if (e.r > 0.0) { // Edge at west
        float2 d;

        // Find the distance to the top:
        float2 coords;
        coords.y = SearchYUp(edgesTex, searchTex, offset[1].xy, offset[2].z);
        coords.x = offset[0].x; // offset[1].x = texcoord.x - 0.25 * SMAA_PIXEL_SIZE.x;
        d.x = coords.y;

        // Fetch the top crossing edges:
        float e1 = edgesTex.SampleLevel(samplerBilinearClamp, coords, 0).g;

        // Find the distance to the bottom:
        coords.y = SearchYDown(edgesTex, searchTex, offset[1].zw, offset[2].w);
        d.y = coords.y;

        // We want the distances to be in pixel units:
        d = d / g_PixSize.y - pixcoord.y;

        // SMAAArea below needs a sqrt, as the areas texture is compressed 
        // quadratically:
        float2 sqrt_d = sqrt(abs(d));

        // Fetch the bottom crossing edges:
        float e2 = edgesTex.SampleLevel(samplerBilinearClamp, coords, 0, int2(0, 1)).g;

        // Get the area for this direction:
        weights.ba = Area(areaTex, sqrt_d, e1, e2, subsampleIndices.x);

        // Fix corners:
        DetectVerticalCornerPattern(edgesTex, weights.ba, texcoord, d);
    }

    return weights;
}