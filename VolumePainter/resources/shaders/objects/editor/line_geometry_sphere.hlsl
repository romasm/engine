TECHNIQUE_DEFAULT
{
	Queue = GUI_3D;

	DepthEnable = true;
	DepthWrite = true;
	DepthFunc = LESS;

	FillMode = WIREFRAME;
	CullMode = NONE;

	VertexShader = "../resources/shaders/objects/editor/line_geometry VSsphere";
	PixelShader = "../resources/shaders/objects/editor/line_geometry PS";
}