#define ROUGHNESS_REMAP 1

#define NORMAL_CLAMP 0.01
#define NORMAL_CLAMP_DOUBLE NORMAL_CLAMP * 2
#define NORMAL_CLAMP_MUL 0.5 / NORMAL_CLAMP

#define DFG_TEXTURE_SIZE 256
#define NOV_MIN 0.5f/DFG_TEXTURE_SIZE
#define NOV_MAX 1.0f - NOV_MIN

float calculateNoV(float3 N, float3 V)
{
	return saturate( dot(N, V) + 0.00001f );
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