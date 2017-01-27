
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

	const float4 albedo_roughY_Sample = gb_albedo_roughY.SampleLevel(gbufferSampler, coords, 0);
	const float4 TBN = gb_tbn.SampleLevel(gbufferSampler, coords, 0);
	const float2 vertex_normal_Sample = gb_vnXY.SampleLevel(gbufferSampler, coords, 0).xy;
	const float4 spec_roughX_Sample = gb_spec_roughX.SampleLevel(gbufferSampler, coords, 0);
	const float4 emiss_vnZ_Sample = gb_emiss_vnZ.SampleLevel(gbufferSampler, coords, 0);
	const float4 subsurf_thick_Sample = gb_subs_thick.SampleLevel(gbufferSampler, coords, 0);
	const float materiaAO_Sample = gb_ao.SampleLevel(gbufferSampler, coords, 0).r;
	const float sceneAO_Sample = dynamicAO.SampleLevel(gbufferSampler, coords, 0).r;
	res.depth = gb_depth.SampleLevel(gbufferSampler, UVforSamplePow2(coords), 0).r;
	res.ssr = ssr_buf.SampleLevel(gbufferSampler, coords, 0);
	
	// PREPARE DATA
	res.vertex_normal = float3(vertex_normal_Sample, emiss_vnZ_Sample.a);
	res.wpos = GetWPos(coords, res.depth);
	
	res.ao = min( materiaAO_Sample, sceneAO_Sample );	
	
	DecodeTBNfromFloat4(res.tangent, res.binormal, res.normal, TBN);
			
	res.albedo = albedo_roughY_Sample.rgb;
	res.reflectivity = spec_roughX_Sample.rgb;
	res.emissive = emiss_vnZ_Sample.rgb;
	res.subsurf = subsurf_thick_Sample.rgb;
	res.thickness = subsurf_thick_Sample.a;
	
	res.roughness = float2(spec_roughX_Sample.a, albedo_roughY_Sample.a);

	return res;
}

MaterialParamsStructBuffer ReadMaterialParams(int2 coords)
{
	const uint matID_objID = gb_mat_obj.Load(int3(coords, 0));
	const uint matID = GetMatID(matID_objID);
	const uint objID = GetObjID(matID_objID);
	MaterialParamsStructBuffer materialParams = MAT_PARAMS[matID];
	return materialParams;
}
#endif