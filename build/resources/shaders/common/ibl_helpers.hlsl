
#define ANISOTROPY_REFL_REMAP 0.5
#define ANISOTROPY_MAX 0.5

#define MAX_MIP_LEVEL 8

float computeSpecularOcclusion( float NoV, float AO, float R ) // ????
{
	return saturate( PowAbs( NoV + AO , R ) - 1 + AO );
}

float3 getSpecularDominantDir( float3 N, float3 Refl, float R, float NoV )
{
	float smoothness = saturate(1 - R );
	float lerpFactor = smoothness * ( sqrt( smoothness ) + R );
	
	return lerp(N, Refl, lerpFactor );
}

float3 getDiffuseDominantDir( float3 N, float3 V, float NoV , float R )
{
	float a = 1.02341f * R - 1.51174f;
	float b = -0.511705f * R + 0.755868f;
	float lerpFactor = saturate(( NoV * a + b) * R );
	// The result is not normalized as we fetch in a cubemap
	return lerp(N, V, lerpFactor );
}

float3 calculateAnisotropicNormal(float2 R, float3 N, float3 B, float3 T, float3 V)
{
	if(R.x == R.y)
		return N;

	float anisotropy = R.x - R.y;
		
	float3 anisotropicBinormal;
	if(anisotropy > 0) anisotropicBinormal = B;
	else anisotropicBinormal = T;
			
	anisotropy = clamp(PowAbs(anisotropy, ANISOTROPY_REFL_REMAP), 0, ANISOTROPY_MAX);
	float3 anisotropicTangent = cross(-V, anisotropicBinormal);
	float3 anisotropicNormal = normalize(cross(anisotropicTangent, anisotropicBinormal));

	return normalize(lerp(N, anisotropicNormal, anisotropy));
}

float3 distantProbSpecular(sampler cubemapSampler, TextureCube cubemap, sampler cubemapBlurredSampler, TextureCube cubemapBlurred, 
						   float3 N, float3 V, float NoV, float R, float mipR, float3 VN)
{
	float3 dominantR = getSpecularDominantDir(N, reflect(-V, N), R, NoV );

	float surfaceFade = saturate(1.1 + dot(VN, normalize(dominantR)));

	float mipNum = mipR * MAX_MIP_LEVEL;
	float lastMipLerp = clamp(1 - (MAX_MIP_LEVEL - mipNum), 0, 1);		

	float3 cubemapSample = cubemap.SampleLevel( cubemapSampler, dominantR, mipNum ).rgb;
	[branch]
	if(lastMipLerp != 0)
	{
		float3 diffuseCubemapSample = cubemapBlurred.SampleLevel( cubemapBlurredSampler, dominantR, 0 ).rgb;
		cubemapSample = lerp(cubemapSample, diffuseCubemapSample, lastMipLerp);
	}
	
	return cubemapSample * surfaceFade;
}

float3 distantProbDiffuse(sampler cubemapSampler, TextureCube cubemap, float3 N, float3 V, float NoV , float R)
{
	float3 dominantN = getDiffuseDominantDir(N, V, NoV, R );
	return cubemap.SampleLevel( cubemapSampler, dominantN, 0 ).rgb;
}

LightComponents CalcutaleDistantProbLight(sampler lutSampler, sampler cubeSampler, sampler cubeBlurredSampler, 
							   float NoV, float R, float3 V, GBufferData gbuffer, float SO,
							   out float3 specularBrdf, out float3 diffuseBrdf)
{
	float3 envBrdf = g_envbrdfLUT.SampleLevel(lutSampler, float2(NoV, R), 0).xyz;
	
	float3 specularNormal = calculateAnisotropicNormal(gbuffer.roughness, gbuffer.normal, gbuffer.binormal, gbuffer.tangent, V);
	specularBrdf = gbuffer.reflectivity * envBrdf.x + saturate(50.0 * gbuffer.reflectivity.g) * envBrdf.y;
	
	LightComponents result = (LightComponents)0;

	// SPECULAR
	result.specular = distantProbSpecular(cubeSampler, g_envprobsDist, cubeBlurredSampler, g_envprobsDistBlurred,
		specularNormal, V, NoV, R, sqrt(R), gbuffer.vertex_normal);

	result.specular *= specularBrdf * SO;
	
	// DIFFUSE
	diffuseBrdf = gbuffer.albedo * envBrdf.z;
	result.diffuse = distantProbDiffuse(cubeSampler, g_envprobsDistBlurred, gbuffer.normal, V, NoV, R);
	result.diffuse *= diffuseBrdf * gbuffer.ao; 

	result.scattering = 0;// TODO

	return result;
}