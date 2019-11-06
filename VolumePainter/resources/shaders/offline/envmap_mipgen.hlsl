TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "MipGen";
}

#include "../common/math.hlsl"
#include "../common/structs.hlsl"

Texture2D hammersleyTexture : register(t0); 
TextureCube cubemap : register(t1); 

SamplerState samplerPointClamp : register(s0);  
SamplerState samplerTrilinearWrap : register(s1); 

#include "offline_ibl.hlsl"

cbuffer materialBuffer : register(b0)
{
	float roughness;
	float mipCount;
	float resolution;
	float _padding;
};

PO_faces MipGen(PI_PosTex input)
{
	PO_faces res;
	
	TripleVect vects = UVCoordsToCube(input.tex);
	
	res.face0 = PrefilterMippedEnvMap( roughness, vects.xdir.xzy, cubemap, samplerTrilinearWrap, mipCount, resolution );
	res.face1 = PrefilterMippedEnvMap( roughness, reflect(-vects.xdir, float3(0,0,1)).xzy, cubemap, samplerTrilinearWrap, mipCount, resolution );
	res.face2 = PrefilterMippedEnvMap( roughness, vects.zdir.xzy, cubemap, samplerTrilinearWrap, mipCount, resolution );
	res.face3 = PrefilterMippedEnvMap( roughness, reflect(-vects.zdir, float3(1,0,0)).xzy, cubemap, samplerTrilinearWrap, mipCount, resolution );
	res.face4 = PrefilterMippedEnvMap( roughness, vects.ydir.xzy, cubemap, samplerTrilinearWrap, mipCount, resolution );
	res.face5 = PrefilterMippedEnvMap( roughness, reflect(-vects.ydir, float3(0,0,1)).xzy, cubemap, samplerTrilinearWrap, mipCount, resolution );
	
	return res;
}
