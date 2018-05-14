TECHNIQUE_DEFAULT
{
	Queue = GUI_3D;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = ALWAYS;

	FillMode = WIREFRAME;
	CullMode = NONE;

	VertexShader = "../resources/shaders/objects/editor/debug_common LineColorVS";
	PixelShader = "../resources/shaders/objects/editor/debug_common LineColorPS";
}