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
	float _padding0;
	float _padding1;
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

	float2 inUV = SnapToScreenTexel(input.tex);

	float4 TBN = gb_tbn.Sample(samplerPointClamp, inUV);
	float3 wpos = GetWPos(inUV, gb_depth.SampleLevel(samplerPointClamp, UVforSamplePow2(inUV), 0).r);

	float3 normal;
	float3 tangent;
	float3 binormal;
	DecodeTBNfromFloat4(tangent, binormal, normal, TBN);
		
	// Get the random factors and construct the row vectors for the 2D matrix from cos(a) and -sin(a) to rotate the sample directions
	float2 randomFactors = ditherTexture.SampleLevel(samplerPointWrap, input.tex * ditherResRcp * uint2(g_screenW, g_screenH), 0).gb;
	float2 rotationX = normalize(randomFactors - 0.5);
	float2 rotationY = rotationX.yx * float2(-1.0f, 1.0f);
	
	// do not take more steps than there are pixels		
	float mipMul = maxStepsPerRayRcp * g_hizMipCount * mipScaler;

	float totalOcclusion = 0.0;
	
	[unroll]
	for (uint i = 0; i < numRays; ++i)
	{
		float2 sampleDir = Rotate(sampleDirections[i].xy, rotationX, rotationY);
		sampleDir *= halfSampleRadius;

		// calculate uv increments per marching step, snapped to texel centres to avoid depth discontinuity artefacts
		float2 stepUV = SnapToScreenTexel(sampleDir * maxStepsPerRayRcp);
		stepUV.x = stepUV.x > 0 ? max(stepUV.x, g_maxScreenCoords.z) : min(stepUV.x, -g_maxScreenCoords.z);
		stepUV.y = stepUV.y > 0 ? max(stepUV.y, g_maxScreenCoords.w) : min(stepUV.y, -g_maxScreenCoords.w);

		float2 uv = inUV;
		
		// top occlusion keeps track of the occlusion contribution of the last found occluder.
		// set to bias value to avoid near-occluders
		float topOcclusion = depthBias;

		for(uint step = 0; step < maxStepsPerRay; ++step)
		{
			uv += stepUV;
			float3 sampleWpos = GetWPos(uv, gb_depth.SampleLevel( samplerPointClamp, UVforSamplePow2(uv), step * mipMul ).r);
			
			// get occlusion factor based on candidate horizon elevation
			float3 horizonVector = sampleWpos - wpos;
			float horizonVectorLength = length(horizonVector);
	
			float occlusion = dot(normal, horizonVector) / horizonVectorLength;
			
			// this adds occlusion only if angle of the horizon vector is higher than the previous highest one without branching
			float diff = max(occlusion - topOcclusion, 0.0);
			topOcclusion = max(occlusion, topOcclusion);
						
			// attenuate occlusion contribution using distance function 1 - (d/f)^2
			float distanceFactor = saturate(horizonVectorLength * fallOff);
			distanceFactor = 1.0 - distanceFactor * distanceFactor;

			totalOcclusion += diff * distanceFactor;
		}
	}

	float finalAO = 1.0 - saturate(strengthPerRay * totalOcclusion);
	finalAO *= finalAO * finalAO;
	return finalAO;
}
