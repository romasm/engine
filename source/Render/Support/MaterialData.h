#pragma once
#include "stdafx.h"
#include "Common.h"

#define SHADER_PS 0
#define SHADER_VS 1
#define SHADER_DS 2
#define SHADER_HS 3
#define SHADER_GS 4
#define SHADER_CS 5

namespace EngineCore
{
	enum RENDER_QUEUES
	{
		GUI_2D,
		GUI_2D_FONT,

		GUI_3D,
		GUI_3D_OVERLAY,

		SC_OPAQUE,
		SC_ALPHATEST,
		SC_ALPHA,
		SC_TRANSPARENT,
	};

	enum TECHNIQUES
	{
		TECHNIQUE_DEFAULT = 0,
		TECHNIQUE_SHADOW = 1,
		TECHNIQUE_VOXEL = 2,
		TECHNIQUES_COUNT = 3
	};
}