TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "BlurV";
}

//~ code
#define G_SAMPLES 1
#define DEPTH_DEPEND
#include "gblur.hlsl"
 