TECHNIQUE_DEFAULT
{
	Queue = SC_TRANSPARENT;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;

	CullMode = NONE;

	VertexShader = "../resources/shaders/objects/opaque_vertex OpaqueVS";
	PixelShader = "../resources/shaders/objects/transparency MediumPS";
}

TECHNIQUE_PREPASS
{
	Queue = SC_TRANSPARENT;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;

	CullMode = FRONT;

	VertexShader = "../resources/shaders/objects/opaque_vertex OpaqueDepthNormalVS";
	PixelShader = "../resources/shaders/objects/transparency MediumPrepassPS";;
}
 
TECHNIQUE_SHADOW
{
	Queue = SC_ALPHATEST; 

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;
	   
	VertexShader = "../resources/shaders/objects/opaque_vertex AlphatestShadowVS";
	PixelShader = "../resources/shaders/objects/transparency MediumShadowPS";
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