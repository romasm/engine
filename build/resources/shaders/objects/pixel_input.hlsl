#define NONMETAL_REFLECTIVITY 0.04 // to global scope

Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D roughnessTexture : register(t2);
Texture2D reflectivityTexture : register(t3);
Texture2D aoTexture : register(t4);
Texture2D alphaTexture : register(t5); // todo OPAQUE_SHADER
Texture2D emissiveTexture : register(t6);
Texture2D subsurfTexture : register(t7);
Texture2D thicknessTexture : register(t8);

cbuffer materialBuffer : register(b1)
{
	float4 albedoColor;
	float4 reflectivityColor;
	float4 emissiveColor;
	float4 subsurfaceColor;

	float hasAlbedoTexture;
	float hasNormalTexture;
	float objectSpaceNormalMap;
	float hasRoughnessTexture;

	float roughnessAnisotropic;
	float roughnessX;
	float roughnessY;
	float hasReflectivityTexture;

	float isMetalPipeline;
	float hasAlphatestTexture;
	float alphatestThreshold;
	float hasEmissiveTexture;

	float hasAOTexture;
	float hasSubsurfTexture;
	float hasThicknessTexture;
	float thicknessValue;
};

cbuffer materialId : register(b2)
{
	uint4 iddata;
};

bool AlphatestCalculate(SamplerState samplerTex, float2 uv)
{
	if( hasAlphatestTexture == 0 )
		return true;

	if( alphaTexture.Sample(samplerTex, uv).r < alphatestThreshold )
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
		float3 normal_sample = normalTexture.Sample(samplerTex, uv).rbg * 2.0f - 1.0f;

		if( objectSpaceNormalMap > 0 )
		{
			const float3x3 nM = (float3x3)nMatrix;
			normal = mul(normal_sample, nM);
		}
		else
		{
			if( normal_sample.z != 0.0f )
				normal_sample = normal_sample / normal_sample.z;
			normal = vertex_normal + normal_sample.x * vertex_tangent + normal_sample.y * vertex_binormal;
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
		roughtness.x *= rough_tex.r;
		roughtness.y *= rough_tex.g;
	}

	if( roughnessAnisotropic == 0 )
		roughtness.y = roughtness.x;
	
	return roughtness;
}

float3 ReflectivityCalculate(SamplerState samplerTex, float2 uv, inout float3 albedo)
{
	float3 reflectivity;

	if( isMetalPipeline > 0 )
	{
		float metallic = reflectivityColor.r;
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
	float3 emissive = emissive_color.rgb;
	if( hasEmissiveTexture > 0 )
		emissive *= emissiveTexture.Sample(samplerTex, uv).rgb;
	return emissive
}

float AOCalculate(SamplerState samplerTex, float2 uv)
{
	if( hasAOTexture == 0 )
		return 1;
	return GammaToLin(aoTexture.Sample(samplerTex, uv).r);
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