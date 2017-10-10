TECHNIQUE_DEFAULT
{
	Queue = SC_OPAQUE;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;

	VertexShader = "../resources/shaders/objects/opaque_vertex OpaqueSkinnedVS"; // TODO
	PixelShader = "../resources/shaders/objects/pixel OpaquePS";
}
 
TECHNIQUE_SHADOW
{
	Queue = SC_OPAQUE; 

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;
	   
	VertexShader = "../resources/shaders/objects/opaque_vertex OpaqueSkinnedShadowVS";
	PixelShader = NULL;
}