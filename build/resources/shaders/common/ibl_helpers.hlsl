
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

float computeDistanceBaseRoughness( float distInteresectionToShadedPoint, float distInteresectionToProbeCenter, float linearRoughness )
{
	const float newLinearRoughness = clamp( distInteresectionToShadedPoint / distInteresectionToProbeCenter * linearRoughness, 0, linearRoughness );
	return lerp( newLinearRoughness, linearRoughness, linearRoughness );
}

void sampleCubeArray(sampler cubeSampler, TextureCubeArray<float4> cubeArray, float3 R, float RoughnessSqrt, float mipsCount, float adress, float distAlpha, inout float4 specular)
{	
	const float mip = mipsCount * RoughnessSqrt;
	float4 envProb = cubeArray.SampleLevel( cubeSampler, float4(R, adress), mip );
	envProb.a *= distAlpha;
			
	const float curAlpha = min(1 - specular.a, envProb.a);
	specular.rgb += envProb.rgb * curAlpha;
	specular.a = min(1, specular.a + curAlpha);
}

void BoxEnvProbSpec(sampler cubeSampler, TextureCubeArray<float4> cubeArray, EnvProbRenderData data, float4 wPos, float3 dominantR, float Roughness, inout float4 specular)
{
	const float3 localPos = mul( wPos, data.invTransform ).xyz;
	
	const float distBoxToPoint = MinFromBoxToPoint(data.offsetFade.www - data.shape.xyz, data.shape.xyz - data.offsetFade.www, localPos);
	const float distAlpha = 1 - saturate( distBoxToPoint / data.offsetFade.w );

	[branch]
	if(distAlpha <= 0)
		return;
	
	const float3 localDir = mul( dominantR, (float3x3)data.invTransform ) * data.positionDistance.w;
	const float2 intersections = RayBoxIntersect( localPos, localDir, -data.shape.xyz, data.shape.xyz );
	
	[branch]
	if( intersections.y <= intersections.x )
		return;

	const float3 boxPointIntersection = intersections.y * localDir;
	float3 localR = localPos + boxPointIntersection - data.offsetFade.xyz;

	const float localRoughness = computeDistanceBaseRoughness( length( boxPointIntersection ), length( localR ), Roughness );
	localR = lerp( localR, dominantR, Roughness );
		
	sampleCubeArray(cubeSampler, cubeArray, localR, sqrt(localRoughness), data.mipsTypeAdressPriority.x, data.mipsTypeAdressPriority.z, distAlpha, specular);
}

void SphereEnvProbSpec(sampler cubeSampler, TextureCubeArray<float4> cubeArray, EnvProbRenderData data, float4 wPos, float3 dominantR, float Roughness, inout float4 specular)
{
	const float3 localPos = mul( wPos, data.invTransform ).xyz;
	const float distFromCenter = length(localPos);
	const float distFade = data.shape.x - distFromCenter;

	[branch]
	if(distFade <= 0)
		return;

	const float distAlpha = saturate( distFade / data.offsetFade.w );
		
	const float3 localDir = mul( dominantR, (float3x3)data.invTransform ) * data.positionDistance.w;
	const float2 intersections = RaySphereIntersect( localPos, localDir, data.shape.x );
		
	[branch]
	if(intersections.y <= intersections.x)
		return;

	const float3 spherePointIntersection = intersections.y * localDir;
	float3 localR = localPos + spherePointIntersection - data.offsetFade.xyz;

	const float localRoughness = computeDistanceBaseRoughness( length( spherePointIntersection ), length( localR ), Roughness );
	localR = lerp( localR, dominantR, Roughness );
				
	sampleCubeArray(cubeSampler, cubeArray, localR, sqrt(localRoughness), data.mipsTypeAdressPriority.x, data.mipsTypeAdressPriority.z, distAlpha, specular);
}

void SimpleEnvProbSpec(sampler cubeSampler, TextureCubeArray<float4> cubeArray, EnvProbRenderData data, float4 wPos, float3 dominantR, float RoughnessSqrt, inout float4 specular)
{
	const float3 localPos = mul( wPos, data.invTransform ).xyz;
	const float distFromCenter = length(localPos);
	const float distFade = data.shape.x - distFromCenter;

	[branch]
	if(distFade <= 0)
		return;

	const float distAlpha = saturate( distFade / data.offsetFade.w );
	const float3 localR = mul( dominantR, (float3x3)data.invTransform );
	
	sampleCubeArray(cubeSampler, cubeArray, localR, RoughnessSqrt, data.mipsTypeAdressPriority.x, data.mipsTypeAdressPriority.z, distAlpha, specular);
}

float4 EvaluateEnvProbSpecular(sampler cubeSampler, float NoV, float Roughness, float3 V, GBufferData gbuffer, float SO, out float3 specularBrdf)
{
	const float3 envBrdf = g_envbrdfLUT.SampleLevel(samplerBilinearClamp, float2(NoV, Roughness), 0).xyz;
	
	const float3 specularNormal = calculateAnisotropicNormal(gbuffer.roughness, gbuffer.normal, gbuffer.binormal, gbuffer.tangent, V);
	const float3 refl = reflect(-V, specularNormal);
	const float3 dominantR = normalize(getSpecularDominantDir(specularNormal, refl, Roughness, NoV ));

	specularBrdf = gbuffer.reflectivity * envBrdf.x + saturate(50.0 * gbuffer.reflectivity.g) * envBrdf.y;
		
	const float surfaceFade = saturate(1.1 + dot(gbuffer.vertex_normal, normalize(dominantR)));
	specularBrdf *= SO * surfaceFade;

	const float4 wPos = float4(gbuffer.wpos, 1.0);
	const float RoughnessSqrt = sqrt(Roughness);

	float4 result = 0;

	int i = 0;
	const int lqEnvProbsCount = min(g_lightCount.envProbsCountLQ, ENVPROBS_FRAME_COUNT_LQ);
	[loop]
	while( i < lqEnvProbsCount && result.a < 1.0 )
	{
		const EnvProbRenderData envData = g_lqEnvProbsData[i];

		[branch]
		if( int(envData.mipsTypeAdressPriority.y) == ENVPROBS_PARALLAX_NONE )
			SimpleEnvProbSpec(cubeSampler, g_lqEnvProbsArray, envData, wPos, dominantR, RoughnessSqrt, result);
		else 
		{
			[branch]
			if( int(envData.mipsTypeAdressPriority.y) == ENVPROBS_PARALLAX_SPHERE )
				SphereEnvProbSpec(cubeSampler, g_lqEnvProbsArray, envData, wPos, dominantR, Roughness, result);
			else
				BoxEnvProbSpec(cubeSampler, g_lqEnvProbsArray, envData, wPos, dominantR, Roughness, result);
		}
		i++;
	}

	i = 0;
	const int sqEnvProbsCount = min(g_lightCount.envProbsCountSQ, ENVPROBS_FRAME_COUNT_SQ);
	[loop]
	while( i < sqEnvProbsCount && result.a < 1.0 )
	{
		const EnvProbRenderData envData = g_sqEnvProbsData[i];

		[branch]
		if( int(envData.mipsTypeAdressPriority.y) == ENVPROBS_PARALLAX_NONE )
			SimpleEnvProbSpec(cubeSampler, g_sqEnvProbsArray, envData, wPos, dominantR, RoughnessSqrt, result);
		else
		{
			[branch]
			if( int(envData.mipsTypeAdressPriority.y) == ENVPROBS_PARALLAX_SPHERE )
				SphereEnvProbSpec(cubeSampler, g_sqEnvProbsArray, envData, wPos, dominantR, Roughness, result);
			else
				BoxEnvProbSpec(cubeSampler, g_sqEnvProbsArray, envData, wPos, dominantR, Roughness, result);
		}
		i++;
	}

	i = 0;
	const int hqEnvProbsCount = min(g_lightCount.envProbsCountHQ, ENVPROBS_FRAME_COUNT_HQ);
	[loop]
	while( i < hqEnvProbsCount && result.a < 1.0 )
	{
		const EnvProbRenderData envData = g_hqEnvProbsData[i];

		[branch]
		if( int(envData.mipsTypeAdressPriority.y) == ENVPROBS_PARALLAX_NONE )
			SimpleEnvProbSpec(cubeSampler, g_hqEnvProbsArray, envData, wPos, dominantR, RoughnessSqrt, result);
		else
		{
			[branch]
			if( int(envData.mipsTypeAdressPriority.y) == ENVPROBS_PARALLAX_SPHERE )
				SphereEnvProbSpec(cubeSampler, g_hqEnvProbsArray, envData, wPos, dominantR, Roughness, result);
			else
				BoxEnvProbSpec(cubeSampler, g_hqEnvProbsArray, envData, wPos, dominantR, Roughness, result);
		}
		i++;
	}

	result.rgb *= specularBrdf;
	return result;
}

/////////////////////////////////////////
// DEPRICATED
/*
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

// rework
LightComponents  CalcutaleDistantProbLight(sampler lutSampler, sampler cubeSampler, sampler cubeBlurredSampler, 
							   float NoV, float R, float3 V, GBufferData gbuffer, float SO,
							   out float3 specularBrdf, out float3 diffuseBrdf)
{
	const float3 envBrdf = g_envbrdfLUT.SampleLevel(samplerBilinearClamp, float2(NoV, R), 0).xyz;
	
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

void GetIndirectBrdf(float NoV, float R, float3 albedo, float3 reflectivity, out float3 specularBrdf, out float3 diffuseBrdf)
{
	const float3 envBrdf = g_envbrdfLUT.SampleLevel(samplerBilinearClamp, float2(NoV, R), 0).xyz;
	specularBrdf = reflectivity * envBrdf.x + saturate(50.0 * reflectivity.g) * envBrdf.y;
	diffuseBrdf = albedo * envBrdf.z;
}*/