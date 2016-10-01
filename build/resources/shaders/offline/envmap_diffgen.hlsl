TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "DiffGen";
}

//~ code
#include "../common/math.hlsl"
#include "../common/structs.hlsl"

Texture2D hammersleyTexture : register(t0); 
TextureCube cubemap : register(t1); 

SamplerState samplerPointClamp : register(s0);  
SamplerState samplerTrilinearWrap : register(s1); 

#include "offline_ibl.hlsl"

triple_vect coordsRTtoCube(float2 tex)
{
	triple_vect res;
	
	float2 norm_tex = (tex - float2(0.5f,0.5f)) * 2;
	
	res.xdir.z = -norm_tex.y * SQRT2DIV2;
	res.xdir.y = -norm_tex.x * SQRT2DIV2;
	res.xdir.x = SQRT2DIV2;
	res.xdir = normalize(res.xdir);
	
	res.ydir.z = -norm_tex.y * SQRT2DIV2;
	res.ydir.x = norm_tex.x * SQRT2DIV2;
	res.ydir.y = SQRT2DIV2;
	res.ydir = normalize(res.ydir);
	
	res.zdir.x = norm_tex.x * SQRT2DIV2;
	res.zdir.y = norm_tex.y * SQRT2DIV2;
	res.zdir.z = SQRT2DIV2;
	res.zdir = normalize(res.zdir);
	
	return res;
}

PO_faces DiffGen(PI_PosTex input)
{
	PO_faces res;
	
	triple_vect vects = coordsRTtoCube(input.tex);
	
	res.face0 = PrefilterDiffEnvMap( vects.xdir.xzy, cubemap, samplerTrilinearWrap );
	res.face1 = PrefilterDiffEnvMap( reflect(-vects.xdir, float3(0,0,1)).xzy, cubemap, samplerTrilinearWrap );
	res.face2 = PrefilterDiffEnvMap( vects.zdir.xzy, cubemap, samplerTrilinearWrap );
	res.face3 = PrefilterDiffEnvMap( reflect(-vects.zdir, float3(1,0,0)).xzy, cubemap, samplerTrilinearWrap );
	res.face4 = PrefilterDiffEnvMap( vects.ydir.xzy, cubemap, samplerTrilinearWrap );
	res.face5 = PrefilterDiffEnvMap( reflect(-vects.ydir, float3(0,0,1)).xzy, cubemap, samplerTrilinearWrap );

	return res;
}
