TECHNIQUE_DEFAULT
{
	Queue = GUI_2D;

	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "../resources/shaders/gui/rect_color_icon_alpha PS";

	BlendEnable = true;
	BlendOp = ADD;
	SrcBlend = ONE;
	DestBlend = INV_SRC_ALPHA;
}