#include "../common/math.include"
#include "../common/light_structs.hlsl"

#include "../common/structs.include"

#include "../common/shared.include"

struct EnvProbStructBuffer
{
	float3 Pos;
	float Radius;
	float3 Extend;
	float Fade;
	uint Type;
	uint NumMips;
	float3 Offset;
	uint textureID;
	float2 padding;
	matrix Transform;
};
//StructuredBuffer<EnvProbStructBuffer> EPI : register(t0);

struct MaterialParamsStructBuffer
{
	uint unlit;
	uint subscattering;
	float ss_distortion;
	float ss_direct_translucency;
	float ss_direct_pow;
	float ss_indirect_translucency;

	float padding[2];
};
StructuredBuffer<MaterialParamsStructBuffer> MAT_PARAMS : register(t0);

Texture2D envbrdfLUT : register(t1);
Texture2D inputPBS[12] : register(t2); 
TextureCube envprobsDist : register(t14); 
TextureCube envprobsDistDiff : register(t15); 
TextureCube envprobs[ENVPROBS_COUNT] : register(t16); 

SamplerState samplerClamp : register(s0);
SamplerState samplerWarp : register(s1);
SamplerState samplerClampFilter : register(s2);
SamplerState samplerWarpAutoFilter : register(s3);
SamplerState samplerLinearClamp : register(s4);
SamplerState samplerTrilinearClamp : register(s5);

#include "indirect_brdf.include"

#define DFG_TEXTURE_SIZE 256
#define NOV_MIN 0.5f/DFG_TEXTURE_SIZE
#define NOV_MAX 1.0f - NOV_MIN

cbuffer MatInfo : register(b1)
{
	float4 fdata[3];
};

cbuffer CamMove : register(b2)
{
	float4x4 viewProjInv_ViewProjPrev;
};

#include "ssr.include"

struct PixelOutputType
{
    float4 diffuse : SV_TARGET0;
	float2 specular : SV_TARGET1;
};

PixelOutputType PS(PIFromQuad input)
{
	PixelOutputType res;
	res.diffuse = inputPBS[9].Sample(samplerClamp, input.tex);
	res.specular = inputPBS[10].Sample(samplerClamp, input.tex).rg;
	
	float4 res_diff = 0;
	float4 res_spec = 0;
		
	float4 albedo_roughY = inputPBS[0].Sample(samplerClamp, input.tex);
	float4 TBN = inputPBS[1].Sample(samplerClamp, input.tex);
	float4 spec_roughX = inputPBS[2].Sample(samplerClamp, input.tex);
	float4 emiss_ao = inputPBS[3].Sample(samplerClamp, input.tex);
	float4 matID_wpos = inputPBS[4].Sample(samplerClamp, input.tex);
	float4 subsurf = inputPBS[5].Sample(samplerClamp, input.tex);
	float depth = inputPBS[6].Sample(samplerClamp, input.tex).r;
	
	//float r = VISBUFFER.SampleLevel(samplerClamp, input.tex, uint(10 * frac(g_time / 20000.0))).r;
	//res.diffuse = PowAbs(float4(r, r, r, 0), 1);
	//res.specular = 0;
	//return res;
	
	float matID = matID_wpos.a;
	float3 wpos = matID_wpos.rgb;
	
	MaterialParamsStructBuffer params = MAT_PARAMS[uint(matID)];
	
	float3 normal;
	float3 tangent;
	float3 binormal;
	DecodeTBNfromFloat4(tangent, binormal, normal, TBN);
	
	float3 albedo = albedo_roughY.rgb;
	float ao = emiss_ao.a;
	float3 spec = spec_roughX.rgb;
	float2 roughnessXY;
	roughnessXY.x = spec_roughX.a;
	roughnessXY.y = albedo_roughY.a;
	//float sqrtR = roughnessXY.x;
	roughnessXY = PowAbs(roughnessXY, ROUGHNESS_REMAP);
	
	if(params.unlit == 0)
	{	
		float R = clamp( min(roughnessXY.x, roughnessXY.y) ,0.0001f,0.9999f);
		float sqrtR = sqrt(R);
	
		float3 VtoWP = g_CamPos - wpos;
	
		const uint aoBlurSamples = uint(fdata[1].r);
		const float samplesDiv = (aoBlurSamples+1)*0.5 - 1;
		const float aoBlurClampDist = fdata[1].g;
		
		float rt_ao = 0;
		float ao_weight = 0;
		float2 blurPix = g_PixSize * 1.3333 * fdata[1].b;
		for(uint i=0; i<aoBlurSamples; i++)
		{
			for(uint j=0; j<aoBlurSamples; j++)
			{
				float2 coords = input.tex + blurPix * float2(i-samplesDiv, j-samplesDiv);
				float3 ao_brur_wpos = inputPBS[4].SampleLevel(samplerClamp, coords, 0).rgb;
				float3 dist = ao_brur_wpos - wpos;
				if(dot(dist,dist) < aoBlurClampDist)
				{
					rt_ao += inputPBS[7].SampleLevel(samplerClamp, coords, 0).r;
					ao_weight++;
				}
			}
		}
		rt_ao = rt_ao / ao_weight;
		
		//const float lowerContrastDist = fdata[2].r;
		//float distFalloff = saturate(dot(VtoWP, VtoWP) / lowerContrastDist);
		float ao_contrast_fix = R * (spec.r + spec.g + spec.b) * 0.33333333f;
		ao = min(ao, PowAbs(rt_ao, lerp(fdata[1].a, 1, ao_contrast_fix) ));
		
		//res.diffuse = float4(ao, ao, ao, 0);
		//return res;
	
		const uint EnvProbsCount = uint(fdata[0].r);
		const float skyMip = fdata[0].a;
		
		VtoWP = normalize(VtoWP);
		
		float NoV = saturate( dot(normal, VtoWP) + 0.00001f );
		NoV = clamp(NoV, NOV_MIN, NOV_MAX);
		
		float3 specNormal = normal;
		if(roughnessXY.x != roughnessXY.y)
		{
			float anisotropy = roughnessXY.x-roughnessXY.y;
			
			float3 anisotropicBinormal;
			if(anisotropy>0) anisotropicBinormal = binormal;
			else anisotropicBinormal = tangent;
				
			anisotropy = clamp(PowAbs(anisotropy, ANISOTROPY_REFL_REMAP), 0, 0.9);
			float3 anisotropicTangent = cross(-VtoWP, anisotropicBinormal);
			float3 anisotropicNormal = normalize(cross(anisotropicTangent, anisotropicBinormal));
			specNormal = normalize(lerp(specNormal, anisotropicNormal, anisotropy));
		}
	
		float3 envBrdf = envbrdfLUT.SampleLevel( samplerClampFilter, float2(NoV, R), 0).xyz;
		float3 specBrdf = spec * envBrdf.x + saturate(50.0 * spec.g) * envBrdf.y;
		float3 diffBrdf = albedo * envBrdf.z;
	
		if(fdata[0].g>0)
		{
			/*
			[unroll(ENVPROBS_COUNT)]
			for(uint j=0; j < EnvProbsCount; j++)
			{
				if(res.specular.a >= 1)
					break;
				
				float specMip = sqrtR * EPI[j].NumMips;
				
				switch(EPI[j].Type)
				{
				case PARALLAX_SPHERE:
					sphereEnvProbSpec(wpos, EPI[j], spec, normal, VtoWP, NoV , R, envprobs[j], res.specular );
					break;
					
				case PARALLAX_BOX:
					boxEnvProbSpec(wpos, EPI[j], spec, normal, VtoWP, NoV , R, envprobs[j], res.specular );
					break;
					
				case PARALLAX_NONE:
					simpleEnvProbSpec(wpos, EPI[j], spec, normal, VtoWP, NoV , R, specMip, envprobs[j], res.specular );
					break;
				}			
			}
			*/
			//if(res.specular.a < 1)
				skyEnvProbSpec(spec, specNormal, VtoWP, NoV, R, sqrtR, skyMip, envprobsDist, envprobsDistDiff, res_spec ); // use specBrdf
				
			/*
			float3 posSS = float3(input.tex, depth);				
			float4 ssr = calcSSR(posSS, VtoWP, specNormal, wpos, float2(float(g_screenW), float(g_screenH)), 11, R);
			ssr.rgb *= specBrdf;
				
			res_spec.rgb = lerp( res_spec.rgb, ssr.rgb, ssr.a);
			*/
			
			float specAO = computeSpecularOcclusion(NoV, ao, R);
			specAO = PowAbs(specAO, lerp(fdata[2].r, 1, ao_contrast_fix));
			res_spec *= specAO;
			
			//}
		}
		
		if(fdata[0].b>0)
		{
			skyEnvProbDiff(albedo, normal, VtoWP, NoV, R, 0, envprobsDistDiff, res_diff); // use diffBrdf
			res_diff *= ao;
			if(params.subscattering != 0)
			{
				res_diff.rgb += indirectSubScattering(subsurf.rgb, params, normal, VtoWP, ao, 0, envprobsDistDiff, 2, envprobsDist);
			}
		}
	}
		
	res.diffuse.rgb += res_diff.rgb;
	res.diffuse.a += res_spec.r;
	res.specular.rg += res_spec.gb;
	
	return res;
}
