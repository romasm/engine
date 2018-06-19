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

SGAmpl IntegrateSampleToSGs(float3 color, float3 dir, float4 sgAxisSharpness[SG_COUNT])
{
	SGAmpl sg = (SGAmpl)0;
	[unroll]
	for (int i = 0; i < SG_COUNT; i++)
		sg.A[i] += IntegrateSampleToSingleSG(color, dir, sgAxisSharpness[i]);
	return sg;
}

float3 EvaluateSG(float3 sgA, float3 dir, float4 sgAxisSharpness)
{
	return sgA * exp(sgAxisSharpness.w * (dot(dir, sgAxisSharpness.xyz) - 1.0f));
}

float3 SGFitted(float3 sgA, float3 dir, float4 sgAxisSharpness, float4 sgHelpers0, float4 sgHelpers1)
{
	const float ADotN = dot(sgAxisSharpness.xyz, dir);

	const float c0 = 0.36f;
	/* on CPU
	const float lambda = sgAxisSharpness.w;

	const float c1 = 1.0f / (4.0f * c0);

	float eml = exp(-lambda);
	float em2l = eml * eml;
	float rl = rcp(lambda);

	float scale = 1.0f + 2.0f * em2l - rl;
	float bias = (eml - em2l) * rl - em2l;

	float x = sqrt(1.0f - scale);
	float x1 = c1 * x;
	*/
	// sgHelpers0: x - x1, y - scale, z - bias, w - 2 Pi / sharpness
	// sgHelpers1: x - x rcp

	const float x0 = c0 * ADotN;
	float x1 = sgHelpers0.x;
	float n = x0 + x1;

	const float xRcp = sgHelpers1.x;
	float y = (abs(x0) <= x1) ? n * n * xRcp : saturate(ADotN);

	const float scale = sgHelpers0.y;
	const float bias = sgHelpers0.z;
	float normalizedIrradiance = scale * y + bias;

	return normalizedIrradiance * sgA * sgHelpers0.w;
}

void ComputeSGLighting(out float3 diffuseIrradiance/*, out float3 specularIrradiance*/, float3 dirDiffuse/*, float3 dirSpecular*/, 
	SGAmpl sg, float4 sgBasis[SG_COUNT], float4 sgHelpers0[SG_COUNT], float4 sgHelpers1[SG_COUNT])
{
	diffuseIrradiance = 0;
	//specularIrradiance = 0;
	[unroll]
	for (int i = 0; i < SG_COUNT; ++i)
	{
		diffuseIrradiance += SGFitted(sg.A[i], dirDiffuse, sgBasis[i], sgHelpers0[i], sgHelpers1[i]);
		//diffuseIrradiance += EvaluateSG(sg.A[i], dirDiffuse, sgBasis[i]);
		//specularIrradiance += ;
	}
}