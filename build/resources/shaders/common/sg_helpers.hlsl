#define SG_COUNT 9

struct SGAmpl
{
	float3 A[SG_COUNT];
};

// SphericalGaussian(dir) = Amplitude * exp(Sharpness * (dot(Axis, dir) - 1.0f))
struct SG
{
	float3 Amplitude;
	float3 Axis;
	float Sharpness;
};

// AnisotropicSphericalGaussian(dir) = Amplitude * exp(-SharpnessX * dot(BasisX, dir)^2 - SharpnessY * dot(BasisY, dir)^2)
struct ASG
{
	float3 Amplitude;
	float3 BasisZ;              // Direction the ASG points
	float3 BasisX;
	float3 BasisY;
	float SharpnessX;           // Scale of the X axis
	float SharpnessY;           // Scale of the Y axis
};

// naive integration
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

// diffuse sg calculation
float3 SGFittedHill(in SG irradiance, in float3 dir, in float4 sgHelpers0, in float4 sgHelpers1)
{
	float ADotN = dot(irradiance.Axis, dir);

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
	float y = lerp(saturate(ADotN), n * n * xRcp, float(abs(x0) <= x1));

	const float scale = sgHelpers0.y;
	const float bias = sgHelpers0.z;
	float normalizedIrradiance = scale * y + bias;

	return normalizedIrradiance * irradiance.Amplitude * sgHelpers0.w;
}

float3 EvaluateASG(in ASG asg, in float3 dir)
{
	float smoothTerm = saturate(dot(asg.BasisZ, dir)); 
	float lambdaTerm = asg.SharpnessX * dot(dir, asg.BasisX) * dot(dir, asg.BasisX);
	float muTerm = asg.SharpnessY * dot(dir, asg.BasisY) * dot(dir, asg.BasisY);
	return asg.Amplitude * smoothTerm * exp(-lambdaTerm - muTerm);
}

float3 ConvolveASG_SG(in ASG asg, in SG sg) {
	float nu = sg.Sharpness * 0.5f;

	ASG convolveASG;
	convolveASG.BasisX = asg.BasisX;
	convolveASG.BasisY = asg.BasisY;
	convolveASG.BasisZ = asg.BasisZ;

	convolveASG.SharpnessX = (nu * asg.SharpnessX) / (nu + asg.SharpnessX);
	convolveASG.SharpnessY = (nu * asg.SharpnessY) / (nu + asg.SharpnessY);

	convolveASG.Amplitude = PI / sqrt((nu + asg.SharpnessX) * (nu + asg.SharpnessY));

	return EvaluateASG(convolveASG, sg.Axis) * sg.Amplitude * asg.Amplitude;
}

SG DistributionTermSG(in float3 normal, in float a)
{
	SG distribution;
	distribution.Axis = normal;

	//float a = Roughness * Roughness;
	float a2Rcp = 1.0f / (a * a);
	distribution.Sharpness = 2.0f * a2Rcp;
	distribution.Amplitude = a2Rcp * INV_PI;

	return distribution;
}

ASG WarpDistributionASG(in SG ndf, in float3 view, in float3 refl, in float NoV, in float roughness)
{
	ASG warp;

	warp.BasisZ = getSpecularDominantDir(ndf.Axis, refl, NoV, roughness);
	warp.BasisX = normalize(cross(ndf.Axis, warp.BasisZ));
	warp.BasisY = normalize(cross(warp.BasisZ, warp.BasisX));

	float dotdiro = max(dot(view, ndf.Axis), 0.1);

	warp.SharpnessX = ndf.Sharpness / (8.0 * dotdiro * dotdiro);
	warp.SharpnessY = ndf.Sharpness * (1.0 / 8.0);

	warp.Amplitude = ndf.Amplitude;

	return warp;
}

float3 SpecularTermASGWarp(in SG irradiance, in float3 normal, in float roughness, in float roughnessSq,
	in float3 viewVector, float NoV, in float3 reflectivity, in ASG warpedNDF)
{
	float3 specular = ConvolveASG_SG(warpedNDF, irradiance);

	float NoL = clamp(dot(normal, warpedNDF.BasisZ), 0.01, 1.0);
	float3 H = normalize(warpedNDF.BasisZ + viewVector);
	float VoH = saturate(dot(warpedNDF.BasisZ, H));
	 
	specular *= G_SmithGGX(NoL, NoV, roughnessSq) * F_Schlick(reflectivity, VoH);
	specular *= NoL;

	return max(specular, 0.0f);
}

void ComputeSGLighting(in float3 dirDiffuse, in SGAmpl sg, in GBufferData gbuffer, in DataForLightCompute mData, in float3 viewVector,
	in float4 sgBasis[SG_COUNT], in  float4 sgHelpers0[SG_COUNT], in float4 sgHelpers1[SG_COUNT], out float3 diffuseIrradiance, out float3 specularIrradiance)
{
	diffuseIrradiance = 0;
	specularIrradiance = 0;

	SG ndf = DistributionTermSG(gbuffer.normal, mData.avgRSq);
	ASG warpedNDF = WarpDistributionASG(ndf, viewVector, mData.reflect, mData.NoV, mData.avgR);
	
	const float surfaceFade = saturate(1.1 + dot(gbuffer.vertex_normal, warpedNDF.BasisZ));
	
	SG currentSG;
	[unroll]
	for (int i = 0; i < SG_COUNT; ++i)
	{
		currentSG.Amplitude = sg.A[i];
		currentSG.Axis = sgBasis[i].xyz;
		currentSG.Sharpness = sgBasis[i].w;

		diffuseIrradiance += SGFittedHill(currentSG, dirDiffuse, sgHelpers0[i], sgHelpers1[i]);
		specularIrradiance += SpecularTermASGWarp(currentSG, gbuffer.normal, mData.avgR, mData.avgRSq, viewVector, mData.NoV, gbuffer.reflectivity, warpedNDF);
	}

	specularIrradiance *= surfaceFade;
}