TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "BlurV";
}

//~ code
#define G_SAMPLES 7
#include "gblur.hlsl"