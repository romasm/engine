TECHNIQUE_DEFAULT
{
	Queue = GUI_3D;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS_EQUAL;

	FillMode = WIREFRAME;
	CullMode = NONE;

	VertexShader = "../resources/shaders/objects/editor/debug_lines_common VS";
	PixelShader = "../resources/shaders/objects/editor/debug_lines_common PS";
}