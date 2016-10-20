TECHNIQUE_DEFAULT
{
	Queue = SC_OPAQUE;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;

	VertexShader = "../resources/shaders/objects/opaque_vertex OpaqueVS";
	PixelShader = "OpaquePS";
}
 
TECHNIQUE_SHADOW
{
	Queue = SC_OPAQUE; 

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;
	   
	VertexShader = "../resources/shaders/objects/opaque_vertex OpaqueShadowVS";
	PixelShader = NULL;
}
//~ code     

#define OPAQUE_SHADER

#include "pixel.hlsl"