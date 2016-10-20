#include "../common/math.hlsl"
#include "../common/shared.hlsl"
#include "../common/structs.hlsl"

Texture2D albedoTex : register(t0);
Texture2D normalTex : register(t1);
Texture2D roughnessTex : register(t2);
Texture2D specTex : register(t3);
Texture2D aoTex : register(t4);
Texture2D alphaTex : register(t5); // todo OPAQUE_SHADER
Texture2D emissiveTex : register(t6);
Texture2D subsurfTex : register(t7);
Texture2D thicknessTex : register(t8);

SamplerState samplerAnisotropicWrap : register(s0);

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

cbuffer matrixBuffer : register(b3)
{
	matrix worldMatrix;
	matrix normalMatrix;
};

PO_Gbuffer OpaquePS(PI_Mesh input, bool front: SV_IsFrontFace)
{
	PO_Gbuffer res;

	float3 albedo;
	float ao = 1;
	float3 normal = float3(0.0f,0.0f,1.0f);;
	float2 roughtness;
	float3 specular;
	float3 emissive = 0;
	float3 subsurface = 0;
	float thickness = 0;
	float translucent = 0;
	
	float2 texcoord = input.tex;
	
	// opacity
	if( alpha_ref != 0 )
	{
		if( alphaTex.Sample(samplerAnisotropicWrap, texcoord).r < alpha_ref )
			discard;
	}
	
	// albedo
	if( albedo_tex == 1 )
	{
		albedo = GammaToLin(albedoTex.Sample(samplerAnisotropicWrap, texcoord).rgb) * albedo_color.rgb;
	}
	else
	{
		albedo = albedo_color.rgb;
	}
	
	// normal 
	if( normal_tex == 0 )
	{
		normal = input.normal;
	}
	else if( normal_tex == 1 )
	{
		normal = normalTex.Sample(samplerAnisotropicWrap, texcoord).rgb * 2.0f - 1.0f;
		//normal.y = -normal.y;
		if( normal.z != 0.0f )
			normal = normal.xyz / normal.z;
			
		normal = input.normal + normal.x * input.tangent + normal.y * input.binormal;
	}
	else if( normal_tex == 2 )
	{
		normal = normalTex.Sample(samplerAnisotropicWrap, texcoord).rbg * 2.0f - 1.0f;
		const float3x3 nM = (float3x3)normalMatrix;
		normal = mul(normal, nM);
	}
	
	// rough
	if( rough_tex == 1 )
	{
		float2 rough_tex = roughnessTex.Sample(samplerAnisotropicWrap, texcoord).rg;
		roughtness.x = rough_tex.r * rough_x;
		roughtness.y = roughtness.x;
	}
	else if( rough_tex == 2 )
	{
		float2 rough_tex = roughnessTex.Sample(samplerAnisotropicWrap, texcoord).rg;
		roughtness.x = rough_tex.r * rough_x;
		roughtness.y = rough_tex.g * rough_y;
	}
	else
	{
		roughtness.x = rough_x;
		roughtness.y = rough_y;
	}
	
	// specular
	float metallic = 0;
	if( metal_tex == -2 )
	{
		specular = GammaToLin(specTex.Sample(samplerAnisotropicWrap, texcoord).rgb);
	}
	else if( metal_tex == 1 )
	{
		metallic = specTex.Sample(samplerAnisotropicWrap, texcoord).r;
		specular = lerp(0.04, albedo, metallic);
	}
	else if( metal_tex == -1 )
	{
		specular = specular_color.rgb;
	}
	else
	{
		metallic = specular_color.r;
		specular = lerp(0.04, albedo, metallic);
	}
	albedo = albedo * (1 - metallic);
	
	// emiss
	if( emiss_tex != 0 )
	{
		emissive = emissiveTex.Sample(samplerAnisotropicWrap, texcoord).rgb * emissive_color.rgb;
	}
	else
	{
		emissive = emissive_color.rgb;
	}
	
	if( ao_tex != 0 )
	{
		ao = GammaToLin(aoTex.Sample(samplerAnisotropicWrap, texcoord).r);
	}
	
	if( subsurf_tex != 0 )
	{
		if( subsurf_tex == 1 )
		{
			subsurface = subsurface_color.rgb;
		}
		else
		{
			subsurface = GammaToLin(subsurfTex.Sample(samplerAnisotropicWrap, texcoord).rgb) * subsurface_color.rgb;
		}
		if( thickness_tex == 0 )
		{
			thickness = subsurface_color.a;
		}
		else
		{
			thickness = thicknessTex.Sample(samplerAnisotropicWrap, texcoord).r * subsurface_color.a;
		}
	}
	
	// normal final
	normal = normalize(normal);
	if(!front)normal = -normal;
	float3 nTangent = normalize(cross(normal, cross(input.tangent, normal)));
	
	float3 vnorm = normalize(cross(ddx(input.worldPos.xyz), ddy(input.worldPos.xyz)));
	
	res.albedo_roughY = float4(albedo, roughtness.y);
	res.tbn = EncodeTBNasFloat4(normal, nTangent);
	res.vnormXY = vnorm.xy;
	res.spec_roughX = float4(specular, roughtness.x);
	res.emiss_vnormZ = float4(emissive, vnorm.z);
	res.id = iddata.x;
	res.subs_thick = float4(subsurface * saturate(1 - thickness), 0); // a - unused
	res.ao = ao;
	
	return res;
}
