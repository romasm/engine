TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "MipCopy";
}

#include "../common/math.hlsl"
#include "../common/structs.hlsl"

TextureCube cubemap : register(t0); 

SamplerState samplerBilinearWrap : register(s0); 

PO_faces MipCopy(PI_PosTex input)
{
	PO_faces res;
	
	TripleVect vects = UVCoordsToCube(input.tex);
	
	res.face0 = cubemap.SampleLevel( samplerBilinearWrap, vects.xdir.xzy, 0 );
	res.face1 = cubemap.SampleLevel( samplerBilinearWrap, reflect(-vects.xdir, float3(0,0,1)).xzy, 0 );
	res.face2 = cubemap.SampleLevel( samplerBilinearWrap, vects.zdir.xzy, 0 );
	res.face3 = cubemap.SampleLevel( samplerBilinearWrap, reflect(-vects.zdir, float3(1,0,0)).xzy, 0 );
	res.face4 = cubemap.SampleLevel( samplerBilinearWrap, vects.ydir.xzy, 0 );
	res.face5 = cubemap.SampleLevel( samplerBilinearWrap, reflect(-vects.ydir, float3(0,0,1)).xzy, 0 );

	return res;
}
