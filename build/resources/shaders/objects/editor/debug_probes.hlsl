TECHNIQUE_DEFAULT
{
	Queue = GUI_3D;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS_EQUAL;

	FillMode = SOLID;
	CullMode = NONE;

	VertexShader = "../resources/shaders/objects/editor/debug_common ProbVS";
	GeometryShader = "../resources/shaders/objects/editor/debug_common ProbGS";
	PixelShader = "../resources/shaders/objects/editor/debug_common ProbPS";
}