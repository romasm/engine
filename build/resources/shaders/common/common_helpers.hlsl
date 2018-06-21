#define ROUGHNESS_REMAP 1

#define NORMAL_CLAMP 0.01
#define NORMAL_CLAMP_DOUBLE NORMAL_CLAMP * 2
#define NORMAL_CLAMP_MUL 0.5 / NORMAL_CLAMP

#define DFG_TEXTURE_SIZE 256
#define NOV_MIN 0.5f/DFG_TEXTURE_SIZE
#define NOV_MAX 1.0f - NOV_MIN

#define NOV_EPCILON 0.00001f

float calculateNoV(float3 N, float3 V)
{
	return saturate( abs(dot(N, V) - NOV_EPCILON) + NOV_EPCILON );
}

#ifdef GBUFFER_READ
GBufferData ReadGBuffer(sampler gbufferSampler, float2 coords)
{
	GBufferData res;

	const float4 albedo_roughY_Sample = gb_AlbedoRoughnesY.SampleLevel(gbufferSampler, coords, 0);
	const float4 TBN = gb_TBN.SampleLevel(gbufferSampler, coords, 0);
	const float2 vertex_normal_Sample = gb_VertexNormalXY.SampleLevel(gbufferSampler, coords, 0).xy;
	const float4 spec_roughX_Sample = gb_ReflectivityRoughnessX.SampleLevel(gbufferSampler, coords, 0);
	const float4 emiss_vnZ_Sample = gb_EmissiveVertexNormalZ.SampleLevel(gbufferSampler, coords, 0);
	const float4 subsurf_thick_Sample = gb_SubsurfaceThickness.SampleLevel(gbufferSampler, coords, 0);
	res.ao = gb_AmbientOcclusion.SampleLevel(gbufferSampler, coords, 0).r;
	res.depth = gb_Depth.SampleLevel(gbufferSampler, UVforSamplePow2(coords), 0).r;
	
	// PREPARE DATA
	res.vertex_normal = float3(vertex_normal_Sample, emiss_vnZ_Sample.a);
	res.wpos = GetWPos(coords, res.depth);
		
	DecodeTBNfromFloat4(res.tangent, res.binormal, res.normal, TBN);
			
	res.albedo = albedo_roughY_Sample.rgb;
	res.reflectivity = spec_roughX_Sample.rgb;
	res.emissive = emiss_vnZ_Sample.rgb;
	res.subsurf = subsurf_thick_Sample.rgb; 
	res.subsurfTint = 1.0;
	res.thickness = subsurf_thick_Sample.a;
	res.roughness = float2(spec_roughX_Sample.a, albedo_roughY_Sample.a);

	return res;
}

MaterialParams ReadMaterialParams(uint2 pixelID)
{ 
	const uint matID_objID = gb_MaterialObjectID.Load(int3(pixelID, 0));
	const uint matID = GetMatID(matID_objID);
	const uint objID = GetObjID(matID_objID);
	MaterialParams materialParams = gb_MaterialParamsBuffer[matID];
	return materialParams;
}
#endif

float computeSpecularOcclusion(float NoV, float AO, float R) // ????
{
	return saturate(PowAbs(NoV + AO, R) - 1 + AO);
}

float3 getSpecularDominantDir(float3 N, float3 Refl, float R, float NoV)
{
	float smoothness = saturate(1 - R);
	float lerpFactor = smoothness * (sqrt(smoothness) + R);

	return normalize(lerp(N, Refl, lerpFactor));
}

float3 getDiffuseDominantDir(float3 N, float3 V, float NoV, float R)
{
	float a = 1.02341f * R - 1.51174f;
	float b = -0.511705f * R + 0.755868f;
	float lerpFactor = saturate((NoV * a + b) * R); 
	
	return normalize(lerp(N, V, lerpFactor));
}

DataForLightCompute PrepareDataForLight(GBufferData gbuffer, float3 V)
{
	DataForLightCompute mData;
	mData.R = gbuffer.roughness;
	mData.R.x = clamp(mData.R.x, 0.001f, 0.9999f);
	mData.R.y = clamp(mData.R.y, 0.001f, 0.9999f);
	mData.avgR = (mData.R.x + mData.R.y) * 0.5;
	mData.avgRSq = mData.avgR * mData.avgR;
	mData.minR = min(mData.R.x, mData.R.y);
	mData.aGGX = max(mData.R.x * mData.R.y, 0.1);
	mData.sqr_aGGX = Square(mData.aGGX);
	mData.NoV = calculateNoV(gbuffer.normal, V);
	mData.reflect = 2 * gbuffer.normal * mData.NoV - V;
	mData.dominantNormalDiffuse = getDiffuseDominantDir(gbuffer.normal, V, mData.NoV, mData.minR);

	return mData;
}