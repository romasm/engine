#define PARALLAX_SPHERE 0
#define PARALLAX_BOX 1
#define PARALLAX_NONE 2

float computeSpecularOcclusion( float NoV, float AO, float R ) // ????
{
	return saturate( PowAbs( NoV + AO , R ) - 1 + AO );
}
/* for skin
const float SpecularPow = 8.0;
float NdotV = dot(normal, view);
float s = saturate(-0.3 + NdotV * NdotV);
return lerp(pow(ambientOcclusion, SpecularPow),
            1.0, s);
*/

float computeDistanceBaseRoughness( float distInteresectionToShadedPoint, float distInteresectionToProbeCenter, float linearRoughness )
{
	float newLinearRoughness = clamp( distInteresectionToShadedPoint / distInteresectionToProbeCenter * linearRoughness, 0, linearRoughness );
	return lerp( newLinearRoughness, linearRoughness, linearRoughness );
}

float3 getSpecularDominantDir( float3 N, float3 Refl, float R, float NoV ) // NOT per cubmap!!!!
{
	float smoothness = saturate(1 - R );
	float lerpFactor = smoothness * ( sqrt( smoothness ) + R );
	
	return lerp(N, Refl, lerpFactor );
}

float3 getDiffuseDominantDir( float3 N, float3 V, float NoV , float R ) // NOT per cubmap!!!!
{
	float a = 1.02341f * R - 1.51174f;
	float b = -0.511705f * R + 0.755868f;
	float lerpFactor = saturate(( NoV * a + b) * R );
	// The result is not normalized as we fetch in a cubemap
	return lerp(N, V, lerpFactor );
}
/*
float4 indirectDiffuseBRDF( float3 A, float3 N, float3 V, float NoV, float R, float mip, TextureCube diffuseCube )
{
	float3 dominantN = getDiffuseDominantDir(N, V, NoV, R );
	float4 cubemap = diffuseCube.SampleLevel( samplerLinearClamp, -dominantN.xzy, mip );
	float3 diffuseLighting = cubemap.rgb;

	float diffF = envbrdfLUT.SampleLevel( samplerClampFilter, float2(NoV, R ), 0).z;

	return float4(A * diffuseLighting * diffF, cubemap.a);
}

float4 indirectSpecularBRDF( float3 S, float3 refl, float NoV , float R, float mip, TextureCube specularCube, TextureCube specularCubeLastMip, float lastMipLerp = 0 )
{
	float4 cubemap = specularCube.SampleLevel( samplerLinearClamp, -refl.xzy, mip );
	//float4 cubemap = specularCube.Sample( samplerTrilinearClamp, -refl.xzy );
	if(lastMipLerp != 0)
	{
		float4 diffCube = specularCubeLastMip.SampleLevel( samplerLinearClamp, -refl.xzy, 0 );
		cubemap = lerp(cubemap, diffCube, lastMipLerp);
	}
	
	float3 envColor = cubemap.rgb;
	
	float2 envBrdf = envbrdfLUT.SampleLevel( samplerClampFilter, float2(NoV, R ), 0).xy;

	return float4(envColor * (S * envBrdf.x + saturate(50.0 * S.g) * envBrdf.y), cubemap.a);
}

float3 indirectSubScattering(float3 color, MaterialParamsStructBuffer params, float3 N, float3 V, float ao, float mip, TextureCube diffuseCube, float mipSpec, TextureCube specCube )
{
	//float3 L = -V;
	//float3 vLight = L - N * params.ss_distortion;
	
	//todo?
	
	//float3 cubemapV_d = diffuseCube.SampleLevel( samplerLinearClamp, -vLight.xzy, mip ).rgb;
	//float3 cubemapV_s = specCube.SampleLevel( samplerLinearClamp, -vLight.xzy, mipSpec ).rgb;
	float3 cubemapN = diffuseCube.SampleLevel( samplerLinearClamp, N.xzy, mip ).rgb;
	cubemapN += diffuseCube.SampleLevel( samplerLinearClamp, -N.xzy, mip ).rgb * ao;
	
	float3 SSS = params.ss_indirect_translucency * cubemapN;
		
	return SSS * color;
}

void boxEnvProbSpec(float3 wp, EnvProbStructBuffer data, float3 S, float3 N, float3 V, float NoV, float R, TextureCube specularCube, inout float4 specular )
{
	float3 localPos = mul( float4( wp, 1), data.Transform ).xyz;
	
	float distBoxToPoint = MinFromBoxToPoint(data.Fade.xxx - data.Extend, data.Extend - data.Fade.xxx, localPos);
	float distalpha = 1 - saturate( distBoxToPoint / data.Fade );
	if(distalpha <= 0)return;
	
	float3 Refl = reflect(-V, N);
	float3 dominantR = normalize(getSpecularDominantDir(N, Refl, R, NoV ));
	float3 localDir = mul( dominantR, (float3x3)data.Transform ) * data.Radius;

	float2 intersections = RayBoxIntersect( localPos, localDir, -data.Extend, data.Extend );
	
	if ( intersections.y > intersections.x )
	{		
		float3 BoxPointIntersection = intersections.y * localDir;
		float3 localR = localPos + BoxPointIntersection;
		localR = localR - data.Offset;

		float localRoughness = computeDistanceBaseRoughness( length( BoxPointIntersection ), length( localR ), R );
		localR = lerp( localR, dominantR, R );
		
		float4 envprob_color = indirectSpecularBRDF(S, localR, NoV, R, data.NumMips * sqrt(localRoughness), specularCube, specularCube);
		
		envprob_color.a *= distalpha;
			
		float cur_alpha = min(1 - specular.a, envprob_color.a);
		specular.rgb += envprob_color.rgb * cur_alpha;
		specular.a = min(1, specular.a + cur_alpha);
	}
}

void sphereEnvProbSpec(float3 wp, EnvProbStructBuffer data, float3 S, float3 N, float3 V, float NoV , float R, TextureCube specularCube, inout float4 specular )
{
	float3 localPos = wp - data.Pos;
	float distFromCenter = length(localPos);
	float distfade = data.Radius - distFromCenter;
	if(distfade > 0)
	{	
		float distalpha = saturate( distfade / data.Fade );
		
		float3 Refl = reflect(-V, N);
		float3 dominantR = normalize(getSpecularDominantDir(N, Refl, R, NoV ));

		float3 SpherePointIntersection = dominantR * data.Radius;	
		float2 intersections = RaySphereIntersect( localPos, SpherePointIntersection, data.Radius );
		
		if(intersections.y > intersections.x)
		{
			SpherePointIntersection *= intersections.y;
			float3 localR = localPos + SpherePointIntersection;
			localR = localR - data.Offset;

			float localRoughness = computeDistanceBaseRoughness( length( SpherePointIntersection ), length( localR ), R );
			localR = lerp( localR, dominantR, R );
			
			float4 envprob_color = indirectSpecularBRDF(S, dominantR, NoV, R, data.NumMips * sqrt(localRoughness), specularCube, specularCube);
			envprob_color.a *= distalpha;
				
			float cur_alpha = min(1 - specular.a, envprob_color.a);
			specular.rgb += envprob_color.rgb * cur_alpha;
			specular.a = min(1, specular.a + cur_alpha);
		}
	}
}

void simpleEnvProbSpec(float3 wp, EnvProbStructBuffer data, float3 S, float3 N, float3 V, float NoV , float R, float mip, TextureCube specularCube, inout float4 specular )
{
	float distFromCenter = length(wp - data.Pos);
	float distfade = data.Radius - distFromCenter;
	if(distfade > 0)
	{	
		float distalpha = saturate( distfade / data.Fade );
		
		float3 Refl = reflect(-V, N);
		float3 dominantR = getSpecularDominantDir(N, Refl, R, NoV );
		
		float4 envprob_color = indirectSpecularBRDF(S, dominantR, NoV, R, mip, specularCube, specularCube);
		envprob_color.a *= distalpha;
			
		float cur_alpha = min(1 - specular.a, envprob_color.a);
		specular.rgb += envprob_color.rgb * cur_alpha;
		specular.a = min(1, specular.a + cur_alpha);
	}
}
*/
float3 skyEnvProbSpec(float3 N, float3 V, float NoV , float R, float mipR, float maxMip, TextureCube specularCube, TextureCube specularCubeLastMip)
{
	float3 Refl = reflect(-V, N);
	float3 dominantR = getSpecularDominantDir(N, Refl, R, NoV );

	float mipNum = mipR * maxMip;
	float lastMipLerp = clamp(1 - (maxMip - mipNum), 0, 1);		

	float3 cubemap = specularCube.SampleLevel( samplerTrilinearWrap, dominantR, mipNum ).rgb;
	if(lastMipLerp != 0)
	{
		float3 diffCube = specularCubeLastMip.SampleLevel( samplerBilinearWrap, dominantR, 0 ).rgb;
		cubemap = lerp(cubemap, diffCube, lastMipLerp);
	}
	
	return cubemap;
}

float3 skyEnvProbDiff(float3 N, float3 V, float NoV , float R, TextureCube diffuseCube)
{
	float3 dominantN = getDiffuseDominantDir(N, V, NoV, R );
	return diffuseCube.SampleLevel( samplerBilinearWrap, dominantN, 0 ).rgb;
}