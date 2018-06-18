#define SG_COUNT 9

struct SGAmpl
{
	float3 A[SG_COUNT];
};

float3 IntegrateSampleToSingleSG(float3 color, float3 dir, float4 sgAxisSharpness)
{
	float AoD = dot(sgAxisSharpness.xyz, dir);
	if (AoD <= 0.0)
		return 0;

	float weight = exp((AoD - 1.0) * sgAxisSharpness.w);
	return color * weight;
}

void IntegrateSampleToSGs(float3 color, float3 dir, float4 sgAxisSharpness[SG_COUNT], float monteCarlo, inout SGAmpl SG)
{
	[unroll]
	for (int i = 0; i < SG_COUNT; i++)
		SG.A[i] += IntegrateSampleToSingleSG(color, dir, sgAxisSharpness[i]) * monteCarlo;
}

float3 SGFitted(float3 sgA, float4 sgAxisSharpness, float3 dir)
{
	const float muDotN = dot(sgAxisSharpness.xyz, dir);

	// on CPU
	const float lambda = lightingLobe.Sharpness;

	const float c0 = 0.36f;
	const float c1 = 1.0f / (4.0f * c0);

	float eml = exp(-lambda);
	float em2l = eml * eml;
	float rl = rcp(lambda);

	float scale = 1.0f + 2.0f * em2l - rl;
	float bias = (eml - em2l) * rl - em2l;

	float x = sqrt(1.0f - scale);
	//

	float x0 = c0 * muDotN;
	float x1 = c1 * x;

	float n = x0 + x1;

	float y = (abs(x0) <= x1) ? n * n / x : saturate(muDotN);

	float normalizedIrradiance = scale * y + bias;

	return normalizedIrradiance * PI_MUL2 * (sgA / sgAxisSharpness.w);
}

void ComputeSGLighting(out float3 diffuseIrradiance, out float3 specularIrradiance, float3 dirDiffuse, float3 dirSpecular, 
	SGAmpl sg, float4 sgAxisSharpness[SG_COUNT])
{
	diffuseIrradiance = 0;
	specularIrradiance = 0;
	[unroll]
	for (int i = 0; i < SG_COUNT; ++i)
	{
		diffuseIrradiance += SGFitted(sg.A[i], sgAxisSharpness[i], dirDiffuse);
		specularIrradiance += ;
	}
}