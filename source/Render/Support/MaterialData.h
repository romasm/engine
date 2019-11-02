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
		SC_FORWARD,
		SC_TRANSPARENT,
	};

	enum TECHNIQUES
	{
		TECHNIQUE_DEFAULT = 0,
		TECHNIQUE_SKIN_DEFAULT = 1,
		TECHNIQUE_SHADOW = 2,
		TECHNIQUE_SKIN_SHADOW = 3,
		TECHNIQUE_VOXEL = 4,
		TECHNIQUE_SKIN_VOXEL = 5,
		TECHNIQUE_PREPASS = 6,
		TECHNIQUE_SKIN_PREPASS = 7,
		TECHNIQUES_COUNT = 8
	};
}