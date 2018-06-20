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

float3 SGFittedHill(float3 sgA, float3 dir, float4 sgAxisSharpness, float4 sgHelpers0, float4 sgHelpers1)
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

// SphericalGaussian(dir) := Amplitude * exp(Sharpness * (dot(Axis, dir) - 1.0f))
struct SG
{
	float3 Amplitude;
	float3 Axis;
	float Sharpness;
};

// AnisotropicSphericalGaussian(dir) :=
//    Amplitude * exp(-SharpnessX * dot(BasisX, dir)^2 - SharpnessY * dot(BasisY, dir)^2)
struct ASG
{
	float3 Amplitude;
	float3 BasisZ;              // Direction the ASG points
	float3 BasisX;
	float3 BasisY;
	float SharpnessX;           // Scale of the X axis
	float SharpnessY;           // Scale of the Y axis
};

//-------------------------------------------------------------------------------------------------
// Evaluates an ASG given a direction on a unit sphere
//-------------------------------------------------------------------------------------------------
float3 EvaluateASG(in ASG asg, in float3 dir)
{
	float smoothTerm = saturate(dot(asg.BasisZ, dir)); 
	float lambdaTerm = asg.SharpnessX * dot(dir, asg.BasisX) * dot(dir, asg.BasisX);
	float muTerm = asg.SharpnessY * dot(dir, asg.BasisY) * dot(dir, asg.BasisY);
	return asg.Amplitude * smoothTerm * exp(-lambdaTerm - muTerm);
}

//-------------------------------------------------------------------------------------------------
// Convolve an SG with an ASG
//-------------------------------------------------------------------------------------------------
float3 ConvolveASG_SG(in ASG asg, in SG sg) {
	// The ASG paper specifes an isotropic SG as exp(2 * nu * (dot(v, axis) - 1)),
	// so we must divide our SG sharpness by 2 in order to get the nup parameter expected by
	// the ASG formulas
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

//-------------------------------------------------------------------------------------------------
// Returns an SG approximation of the GGX NDF used in the specular BRDF. For a single-lobe
// approximation, the resulting NDF actually more closely resembles a Beckmann NDF.
//-------------------------------------------------------------------------------------------------
SG DistributionTermSG(in float3 direction, in float roughness)
{
	SG distribution;
	distribution.Axis = direction;
	float m2 = roughness * roughness;
	distribution.Sharpness = 2 / m2;
	distribution.Amplitude = 1.0f / (PI * m2);

	return distribution;
}

//-------------------------------------------------------------------------------------------------
// Generate an ASG that best represents the NDF SG but with it's axis oriented in the direction
// of the current BRDF slice. This will allow easier integration, because the SG\ASG are both
// in the same domain.
//
// The warped NDF can be represented better as an ASG, so following Kun Xu from
// 'Anisotropic Spherical Gaussians' we change the SG to an ASG because the distribution of
// an NDF stretches at grazing angles.
//-------------------------------------------------------------------------------------------------
ASG WarpDistributionASG(in SG ndf, in float3 view, float NoV, float roughness)
{
	ASG warp;

	// Generate any orthonormal basis with Z pointing in the direction of the reflected view vector
	warp.BasisZ = reflect(-view, ndf.Axis);
	warp.BasisZ = getSpecularDominantDir(ndf.Axis, warp.BasisZ, NoV, roughness);

	warp.BasisX = normalize(cross(ndf.Axis, warp.BasisZ));
	warp.BasisY = normalize(cross(warp.BasisZ, warp.BasisX));

	float dotdiro = max(dot(view, ndf.Axis), 0.1f);

	// Second derivative of the sharpness with respect to how far we are from basis Axis direction
	warp.SharpnessX = ndf.Sharpness / (8.0f * dotdiro * dotdiro);
	warp.SharpnessY = ndf.Sharpness / 8.0f;

	warp.Amplitude = ndf.Amplitude;

	return warp;
}

//-------------------------------------------------------------------------------------------------
// Computes the specular reflectance from a single SG lobe containing incoming radiance
//-------------------------------------------------------------------------------------------------
float3 SpecularTermASGWarp(in SG irradiance, in float3 normal, in float roughness,
	in float3 view, float NoV, in float3 reflectivity)
{
	float roughnessSq = roughness * roughness;

	// Create an SG that approximates the NDF. Note that a single SG lobe is a poor fit for
	// the GGX NDF, since the GGX distribution has a longer tail. A sum of 3 SG's can more
	// closely match the shape of a GGX distribution, but it would also increase the cost
	// computing specular by a factor of 3.
	SG ndf = DistributionTermSG(normal, roughnessSq);

	// Apply a warpring operation that will bring the SG from the half-angle domain the the
	// the lighting domain. The resulting lobe is an ASG that's stretched along the viewing
	// direction in order to better match the actual shape of a GGX distribution.
	ASG warpedNDF = WarpDistributionASG(ndf, view, NoV, roughnessSq);

	// Convolve the NDF with the light. Note, this is a integration of the NDF which is an ASG
	// with the light which is a SG. See Kun Xu 'Anisotropic Spherical Gaussians' section 4.3
	// for more details
	float3 specular = ConvolveASG_SG(warpedNDF, irradiance);

	// Parameters needed for evaluating the visibility term
	float NoL = clamp(dot(normal, warpedNDF.BasisZ), 0.01, 1.0);
	float3 H = normalize(warpedNDF.BasisZ + view);
	float VoH = saturate(dot(warpedNDF.BasisZ, H));
	 
	specular *= G_SmithGGX(NoL, NoV, roughness) * F_Schlick(reflectivity, VoH);
	
	// Cosine term evaluated at the center of our warped BRDF lobe
	specular *= NoL;

	return max(specular, 0.0f);
}

void ComputeSGLighting(float3 dirDiffuse, SGAmpl sg, GBufferData gbuffer, DataForLightCompute mData, float3 viewVector,
	float4 sgBasis[SG_COUNT], float4 sgHelpers0[SG_COUNT], float4 sgHelpers1[SG_COUNT], out float3 diffuseIrradiance, out float3 specularIrradiance)
{
	diffuseIrradiance = 0;
	specularIrradiance = 0;

	SG currentSG;
	[unroll]
	for (int i = 0; i < SG_COUNT; ++i)
	{
		currentSG.Amplitude = sg.A[i];
		currentSG.Axis = sgBasis[i].xyz;
		currentSG.Sharpness = sgBasis[i].w;

		diffuseIrradiance += SGFittedHill(sg.A[i], dirDiffuse, sgBasis[i], sgHelpers0[i], sgHelpers1[i]);
		specularIrradiance += SpecularTermASGWarp(currentSG, gbuffer.normal, mData.avgR, viewVector, mData.NoV, gbuffer.reflectivity);
	}
}