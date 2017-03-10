#define NONMETAL_REFLECTIVITY 0.04 // to global scope

#ifdef FORWARD_LIGHTING
Texture2D albedoTexture : register(t21);
Texture2D normalTexture : register(t22);
Texture2D roughnessTexture : register(t23);
Texture2D reflectivityTexture : register(t24);
Texture2D aoTexture : register(t25);
Texture2D alphaTexture : register(t26);
Texture2D emissiveTexture : register(t27);
Texture2D subsurfTexture : register(t28);
Texture2D thicknessTexture : register(t29);
Texture2D absorptionTexture : register(t30);
Texture2D insideRoughnessTexture : register(t31);

#else
Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D roughnessTexture : register(t2);
Texture2D reflectivityTexture : register(t3);
Texture2D aoTexture : register(t4);
Texture2D alphaTexture : register(t5); // todo OPAQUE_SHADER
Texture2D emissiveTexture : register(t6);
Texture2D subsurfTexture : register(t7);
Texture2D thicknessTexture : register(t8);
#endif

cbuffer materialBuffer : register(b1)
{
	float4 albedoColor;
	float4 reflectivityColor;
	float4 emissiveColor;
	float4 subsurfaceColor;

	float hasAlbedoTexture;
	float hasNormalTexture;
	float normalMapInvertY;
	float objectSpaceNormalMap;

	float hasRoughnessTexture;
	float roughnessAnisotropic;
	float roughnessX;
	float roughnessY;

	float isGlossiness;
	float hasReflectivityTexture;
	float isMetalPipeline;
	float metalnessValue;

	float hasAlphaTexture;
	float alphaValue;
	float hasEmissiveTexture;
	float emissiveIntensity;

	float hasAOTexture;
	float aoPower;
	float hasSubsurfTexture;
	float hasThicknessTexture;

	float thicknessValue;

#ifdef FORWARD_LIGHTING
	float hasAbsorptionTexture;
	float absorptionValue;
	float hasInsideRoughnessTexture;

	float insideRoughnessValue;
	float invIorRed;
	float invIorGreen;
	float invIorBlue;
#else
	float _padding0;
	float _padding1;
	float _padding2;
#endif
};

#ifndef FORWARD_LIGHTING

cbuffer materialId : register(b2)
{
	uint4 iddata;
};

#endif

#ifdef FORWARD_LIGHTING

float OpacityCalculate(SamplerState samplerTex, float2 uv)
{
	float opacity = alphaValue;

	if( hasAlphaTexture == 0 )
		return opacity;

	opacity *= alphaTexture.Sample(samplerTex, uv).r;
	return opacity;
}

float InsideRoughnessCalculate(SamplerState samplerTex, float2 uv)
{
	float insideRoughness = insideRoughnessValue;

	if( hasInsideRoughnessTexture > 0 )
		insideRoughness *= insideRoughnessTexture.Sample(samplerTex, uv).r;
	
	return insideRoughness;
}

float AbsorptionCalculate(SamplerState samplerTex, float2 uv)
{
	float absorption = absorptionValue;

	if( hasAbsorptionTexture > 0 )
		absorption *= absorptionTexture.Sample(samplerTex, uv).r;
	
	return absorption;
}

float3 IORCalculate()
{
	return float3(invIorRed, invIorGreen, invIorBlue);
}

#endif

bool AlphatestCalculate(SamplerState samplerTex, float2 uv)
{
	if( hasAlphaTexture == 0 )
		return true;

	if( alphaTexture.Sample(samplerTex, uv).r < alphaValue )
		return false;

	return true;
}

float3 AlbedoCalculate(SamplerState samplerTex, float2 uv)
{
	float3 albedo = albedoColor.rgb;

	if( hasAlbedoTexture > 0 )
		albedo *= albedoTexture.Sample(samplerTex, uv).rgb;
	
	return GammaToLin(albedo);
}

float3 NormalCalculate(SamplerState samplerTex, float2 uv, float3 vertex_normal, float3 vertex_tangent, float3 vertex_binormal, matrix nMatrix )
{
	float3 normal = vertex_normal;

	if( hasNormalTexture > 0 )
	{
		float3 normal_sample = normalTexture.Sample(samplerTex, uv).rgb * 2.0f - 1.0f;
		if( normalMapInvertY > 0 )
			normal_sample.y = -normal_sample.y;
		
		if( objectSpaceNormalMap > 0 )
		{
			const float3x3 nM = (float3x3)nMatrix;
			normal = mul(normal_sample, nM);
		}
		else
		{
			normal = normal_sample.z * vertex_normal + normal_sample.x * vertex_tangent + normal_sample.y * vertex_binormal;
		}
	}

	return normalize(normal);
}

float2 RoughnessCalculate(SamplerState samplerTex, float2 uv)
{
	float2 roughtness = float2(roughnessX, roughnessY);

	if( hasRoughnessTexture > 0 )
	{
		float2 rough_sample = roughnessTexture.Sample(samplerTex, uv).rg;
		roughtness.x *= rough_sample.r;
		roughtness.y *= rough_sample.g;
	}

	if( roughnessAnisotropic == 0 )
		roughtness.y = roughtness.x;
	
	if( isGlossiness > 0 )
		roughtness = float2(1.0, 1.0) - roughtness;
	return roughtness;
}

float3 ReflectivityCalculate(SamplerState samplerTex, float2 uv, inout float3 albedo)
{
	float3 reflectivity;

	if( isMetalPipeline > 0 )
	{
		float metallic = metalnessValue;
		if( hasReflectivityTexture > 0 )
			metallic = reflectivityTexture.Sample(samplerTex, uv).r;

		reflectivity = lerp(NONMETAL_REFLECTIVITY, albedo, metallic);
		albedo *= (1 - metallic);
	}
	else
	{
		reflectivity = reflectivityColor.rgb;
		if( hasReflectivityTexture > 0 )
			reflectivity = reflectivityTexture.Sample(samplerTex, uv).rgb;

		reflectivity = GammaToLin(reflectivity);	
	}

	return reflectivity;
}

float3 EmissiveCalculate(SamplerState samplerTex, float2 uv)
{
	float3 emissive = emissiveColor.rgb;
	if( hasEmissiveTexture > 0 )
		emissive *= emissiveTexture.Sample(samplerTex, uv).rgb;
	emissive = GammaToLin(emissive);
	return emissive * emissiveIntensity;
}

float AOCalculate(SamplerState samplerTex, float2 uv)
{
	if( hasAOTexture == 0 )
		return 1;
	return PowAbs( GammaToLin(aoTexture.Sample(samplerTex, uv).r), aoPower );
}

float4 SSSCalculate(SamplerState samplerTex, float2 uv)
{
	float4 subsurf = float4(subsurfaceColor.rgb, thicknessValue);

	if( hasSubsurfTexture > 0 )
		subsurf.rgb *= subsurfTexture.Sample(samplerTex, uv).rgb;
	subsurf.rgb = GammaToLin(subsurf.rgb);

	if( hasSubsurfTexture > 0 )
		subsurf.a *= thicknessTexture.Sample(samplerTex, uv).r;
	
	return subsurf;
}