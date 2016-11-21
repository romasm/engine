Texture2D albedoTex : register(t0);
Texture2D normalTex : register(t1);
Texture2D roughnessTex : register(t2);
Texture2D specTex : register(t3);
Texture2D aoTex : register(t4);
Texture2D alphaTex : register(t5); // todo OPAQUE_SHADER
Texture2D emissiveTex : register(t6);
Texture2D subsurfTex : register(t7);
Texture2D thicknessTex : register(t8);

cbuffer materialBuffer : register(b1)
{
	float4 albedo_color;
	float4 specular_color;
	float4 emissive_color;
	float4 subsurface_color;

	float albedo_tex;
	float normal_tex;
	float rough_tex;
	float rough_x;

	float rough_y;
	float metal_tex;
	float alpha_ref;
	float emiss_tex;

	float ao_tex;
	float subsurf_tex;
	float thickness_tex;
	float _padding;
};

cbuffer materialId : register(b2)
{
	uint4 iddata;
};

bool AlphatestSample(SamplerState samplerTex, float2 uv)
{
	if( alpha_ref != 0 )
	{
		if( alphaTex.Sample(samplerTex, uv).r < alpha_ref )
			return false;
	}
	return true;
}

float3 AlbedoSample(SamplerState samplerTex, float2 uv)
{
	if( albedo_tex == 1 )
		return GammaToLin(albedoTex.Sample(samplerTex, uv).rgb) * albedo_color.rgb;
	else
		return albedo_color.rgb;
}

float3 NormalSample(SamplerState samplerTex, float2 uv, float3 vertex_normal, float3 vertex_tangent, float3 vertex_binormal, matrix nMatrix )
{
	float3 normal;
	if( normal_tex == 0 )
	{
		normal = vertex_normal;
	}
	else if( normal_tex == 1 )
	{
		normal = normalTex.Sample(samplerTex, uv).rgb * 2.0f - 1.0f;
		if( normal.z != 0.0f )
			normal = normal.xyz / normal.z;
			
		normal = vertex_normal + normal.x * vertex_tangent + normal.y * vertex_binormal;
	}
	else if( normal_tex == 2 )
	{
		normal = normalTex.Sample(samplerTex, uv).rbg * 2.0f - 1.0f;
		const float3x3 nM = (float3x3)nMatrix;
		normal = mul(normal, nM);
	}
	return normalize(normal);
}

float2 RoughnessSample(SamplerState samplerTex, float2 uv)
{
	float2 roughtness;
	if( rough_tex == 1 )
	{
		float2 rough_tex = roughnessTex.Sample(samplerTex, uv).rg;
		roughtness.x = rough_tex.r * rough_x;
		roughtness.y = roughtness.x;
	}
	else if( rough_tex == 2 )
	{
		float2 rough_tex = roughnessTex.Sample(samplerTex, uv).rg;
		roughtness.x = rough_tex.r * rough_x;
		roughtness.y = rough_tex.g * rough_y;
	}
	else
	{
		roughtness.x = rough_x;
		roughtness.y = rough_y;
	}
	return roughtness;
}

float3 SpecularSample(SamplerState samplerTex, float2 uv, inout float3 albedo)
{
	float3 specular;
	float metallic = 0;
	if( metal_tex == -2 )
	{
		specular = GammaToLin(specTex.Sample(samplerTex, uv).rgb);
	}
	else if( metal_tex == 1 )
	{
		metallic = specTex.Sample(samplerTex, uv).r;
		specular = lerp(0.04, albedo, metallic);
	}
	else if( metal_tex == -1 )
	{
		specular = specular_color.rgb;
	}
	else
	{
		metallic = specular_color.r;
		specular = lerp(0.04, albedo, metallic); // 0.04 global define
	}
	albedo = albedo * (1 - metallic);
	return specular;
}

float3 EmissiveSample(SamplerState samplerTex, float2 uv)
{
	if( emiss_tex != 0 )
		return emissiveTex.Sample(samplerTex, uv).rgb * emissive_color.rgb;
	else
		return emissive_color.rgb;
}

float AOSample(SamplerState samplerTex, float2 uv)
{
	if( ao_tex != 0 )
		return GammaToLin(aoTex.Sample(samplerTex, uv).r);
	else
		return 1;
}

float4 SSSSample(SamplerState samplerTex, float2 uv)
{
	float4 subsurf;
	if( subsurf_tex == 0 )
		return 0;
	
	if( subsurf_tex == 1 )
		subsurf.rgb = subsurface_color.rgb;
	else
		subsurf.rgb = GammaToLin(subsurfTex.Sample(samplerTex, uv).rgb) * subsurface_color.rgb;
	
	if( thickness_tex == 0 )
		subsurf.a = subsurface_color.a;
	else
		subsurf.a = thicknessTex.Sample(samplerTex, uv).r * subsurface_color.a;
	
	return subsurf;
}