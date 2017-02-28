TECHNIQUE_DEFAULT
{
	Queue = SC_TRANSPARENT;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;

	VertexShader = "../resources/shaders/objects/opaque_vertex OpaqueVS";
	PixelShader = "../resources/shaders/objects/transparency MediumPS";
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
 
TECHNIQUE_VOXEL
{
	Queue = SC_TRANSPARENT; 

	DepthEnable = false;
	DepthWrite = false;
	CullMode = NONE;
	
	AntialiasedLineEnable = false;
	MultisampleEnable = true; 
	   
	VertexShader = "../resources/shaders/objects/opaque_vertex VoxelizationOpaqueVS";
	GeometryShader = "../resources/shaders/objects/voxelization VoxelizationGS";
	PixelShader = "../resources/shaders/objects/voxelization VoxelizationOpaquePS";
}