TECHNIQUE_DEFAULT
{
	VertexShader = "../resources/shaders/system/screen_plane Main";
	PixelShader = "BlurH";
}

//~ code
#define G_SAMPLES 10
#include "gblur.hlsl"