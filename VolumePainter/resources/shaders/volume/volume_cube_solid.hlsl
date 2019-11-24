TECHNIQUE_DEFAULT
{
	Queue = SC_TRANSPARENT;

	DepthEnable = false;
	DepthWrite = false;
	DepthFunc = LESS;

	BlendEnable = true;

	BlendOp = ADD;
	SrcBlend = ONE;
	DestBlend = INV_SRC_ALPHA;

	BlendOpAlpha = ADD;
	SrcBlendAlpha = SRC_ALPHA;
	DestBlendAlpha = ONE;

	CullMode = BACK;

	VertexShader = "../resources/shaders/volume/volume_raymatch VolumeCubeVS";
	PixelShader = "../resources/shaders/volume/volume_raymatch VolumeCubePS";
	PixelShaderDefines = "VOLUME_SOLID[]";
}