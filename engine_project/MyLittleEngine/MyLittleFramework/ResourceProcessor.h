#pragma once
#include "stdafx.h"
#include "Common.h"

#define SHADERS_UPDATE_PERIOD 2000.0f
#define TEXTURES_UPDATE_PERIOD 3000.0f
#define STMESHES_UPDATE_PERIOD 5000.0f

#define SHADER_JOB_NAME "ShaderUpdate"
#define SHADERCODE_JOB_NAME "ShaderCodeUpdate"
#define TEXTURE_JOB_NAME "TexturesUpdate"
#define STMESH_JOB_NAME "StMeshesUpdate"

namespace EngineCore
{
	class ResourceProcessor
	{
	public:
		ResourceProcessor();
		~ResourceProcessor();

		void StartUpdate();

		inline static ResourceProcessor* Get(){return instance;}
	private:
		static ResourceProcessor *instance;
	};
}