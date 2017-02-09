TECHNIQUE_DEFAULT
{
	Queue = SC_ALPHATEST;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;

	VertexShader = "../resources/shaders/objects/opaque_vertex OpaqueVS";
	PixelShader = "../resources/shaders/objects/pixel OpaquePS";
}
 
TECHNIQUE_SHADOW
{
	Queue = SC_ALPHATEST; 

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;
	   
	VertexShader = "../resources/shaders/objects/opaque_vertex AlphatestShadowVS";
	PixelShader = "../resources/shaders/objects/pixel AlphatestShadowPS";
}
 
TECHNIQUE_VOXEL
{
	Queue = SC_ALPHATEST; 

	DepthEnable = false;
	DepthWrite = false;
	CullMode = NONE;
	 
	AntialiasedLineEnable = false;
	MultisampleEnable = true; 
	   
	VertexShader = "../resources/shaders/objects/opaque_vertex VoxelizationOpaqueVS";
	GeometryShader = "../resources/shaders/objects/voxelization VoxelizationGS";
	PixelShader = "../resources/shaders/objects/voxelization VoxelizationOpaquePS";
}