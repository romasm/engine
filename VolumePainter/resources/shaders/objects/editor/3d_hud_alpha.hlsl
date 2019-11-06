TECHNIQUE_DEFAULT
{
	Queue = GUI_3D_OVERLAY;

	BlendEnable = true;
	BlendOp = ADD;
	BlendOpAlpha = ADD;
	SrcBlend = SRC_ALPHA;
	DestBlend = INV_SRC_ALPHA;
	SrcBlendAlpha = ONE;
	DestBlendAlpha = ONE;

	CullMode = BACK;

	VertexShader = "../resources/shaders/objects/editor/3d_hud HudVS";
	PixelShader = "../resources/shaders/objects/editor/3d_hud HudPS";
}