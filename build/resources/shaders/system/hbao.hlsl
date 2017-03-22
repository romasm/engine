TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "CalcAO";
}

#include "../common/math.hlsl"
#include "../common/structs.hlsl"
#include "../common/shared.hlsl"

#define numRays 12
#define strengthPerRay 1.0 / numRays
#define maxStepsPerRay 3
#define maxStepsPerRayRcp 1.0 / (maxStepsPerRay - 1)

const static float2 sampleDirections[12] =
{
	float2(0.000, 0.200), 
	float2(0.325, 0.101), 
	float2(0.272, -0.396), 
	float2(-0.385, -0.488), 
	float2(-0.711, 0.274), 
	float2(0.060, 0.900),
	float2(0.000, -0.200), 
	float2(-0.325, -0.101), 
	float2(-0.272, 0.396), 
	float2(0.385, 0.488), 
	float2(0.711, -0.274), 
	float2(-0.060, -0.900)
};

#define ditherResRcp 1.0/256.0
Texture2D ditherTexture : register(t0); 

Texture2D gb_tbn : register(t1); 
Texture2D gb_depth : register(t2); 

SamplerState samplerPointClamp : register(s0);
SamplerState samplerPointWrap : register(s1);

cbuffer materialBuffer : register(b1)
{
	float halfSampleRadius;
	float fallOff;
	float depthBias;
	float mipScaler;

	float maxDistSqr;
	float projParam;
	float hizMip;
	float screenCoordMaxW;

	float screenCoordMaxH;
	float screenCoordMaxWRcp;
	float screenCoordMaxHRcp;
	float _padding0;
};

// rotates a sample direction according to the row-vectors of the rotation matrix
float2 Rotate(float2 vec, float2 rotationX, float2 rotationY)
{
	float2 rotated;
	// just so we can use dot product
	float3 expanded = float3(vec, 0.0);
	rotated.x = dot(expanded.xyz, rotationX.xyy);
	rotated.y = dot(expanded.xyz, rotationY.xyy);
	return rotated;
}

float4 CalcAO(PI_PosTex input) : SV_TARGET
{
	float4 res = 0;
	
	/*const float2 screenCoordMax = float2(g_screenW, g_screenH) - float2(1,1); // cpu
	const float2 screenCoordMaxRcp = rcp(screenCoordMax);  // cpu
	const float projParam = (g_proj[1][1] +  g_proj[2][2]) * 0.5; // cpu
	const float maxDist = pow( (halfSampleRadius * projParam * g_screenW - g_proj[3][3]) * rcp(g_proj[2][3]), 2); // cpu*/

	const float2 screenCoordMax = float2(screenCoordMaxW, screenCoordMaxH);
	const float2 screenCoordMaxRcp = float2(screenCoordMaxWRcp, screenCoordMaxHRcp);

	// snaps a uv coord to the nearest texel centre
#define SnapToTexel(screen_uv) round((screen_uv) * screenCoordMax) * screenCoordMaxRcp

	float2 inUV = SnapToTexel(input.tex);

	float4 TBN = gb_tbn.Sample(samplerPointClamp, inUV);
	float3 wpos = GetWPos(inUV, gb_depth.SampleLevel(samplerPointClamp, inUV, 0).r);

	float3 normal;
	float3 tangent;
	float3 binormal;
	DecodeTBNfromFloat4(tangent, binormal, normal, TBN);
	
	float bias = depthBias;

	const float3 V = g_CamPos - wpos;
	float biasScale = saturate( dot(V, V) / maxDistSqr );

	float depthFalloff = 1 - biasScale * biasScale;
	if(!depthFalloff)
		return 1.0;

	// reconstruct view-space position from depth buffer
	float3 viewPos = mul(float4(wpos, 1.0f), g_view).rgb;
	
	// Get the random factors and construct the row vectors for the 2D matrix from cos(a) and -sin(a) to rotate the sample directions
	float2 randomFactors = ditherTexture.SampleLevel(samplerPointWrap, input.tex * ditherResRcp * int2(g_screenW, g_screenH), 0).gb;
	float2 rotationX = normalize(randomFactors - 0.5);
	float2 rotationY = rotationX.yx * float2(-1.0f, 1.0f);
	
	float w = viewPos.z * g_proj[2][3] + g_proj[3][3];
	float worldRadiusRcp = rcp(halfSampleRadius / projParam * w);
	worldRadiusRcp *= fallOff;

	bias *= (1 + biasScale * 10);

	float mipMul = maxStepsPerRayRcp * hizMip * mipScaler;
	float totalOcclusion = 0.0;
	
	[unroll]
	for (uint i = 0; i < numRays; ++i)
	{
		float2 sampleDir = Rotate(sampleDirections[i].xy, rotationX, rotationY);
		sampleDir *= halfSampleRadius;

		// calculate uv increments per marching step, snapped to texel centres to avoid depth discontinuity artefacts
		float2 stepUV = SnapToTexel(sampleDir * maxStepsPerRayRcp);
	
		float2 uv = inUV;
		
		// top occlusion keeps track of the occlusion contribution of the last found occluder.
		// set to bias value to avoid near-occluders
		float topOcclusion = bias;
		[unroll]
		for(uint step = 0; step < maxStepsPerRay; ++step)
		{
			/*[branch]
			if(uv.x > 1 || uv.y > 1 || uv.x < 0 || uv.y < 0)
				break;*/
			uv += stepUV;
			float3 sampleWpos = GetWPos(uv, gb_depth.SampleLevel( samplerPointClamp, uv, step * mipMul ).r);
			
			// get occlusion factor based on candidate horizon elevation
			float3 horizonVector = sampleWpos - wpos;
			float horizonVectorLength = length(horizonVector);
	
			float occlusion = dot(normal, horizonVector) / horizonVectorLength;
			
			// this adds occlusion only if angle of the horizon vector is higher than the previous highest one without branching
			float diff = max(occlusion - topOcclusion, 0.0);
			topOcclusion = max(occlusion, topOcclusion);
						
			// attenuate occlusion contribution using distance function 1 - (d/f)^2
			float distanceFactor = saturate(horizonVectorLength * worldRadiusRcp);
			distanceFactor = 1.0 - distanceFactor * distanceFactor;

			totalOcclusion += diff * distanceFactor;
		}
	}

	return 1.0 - depthFalloff * saturate(strengthPerRay * totalOcclusion); // pow 2 late
}
