#pragma once
#include "stdafx.h"
#include "Shader.h"
#include "BaseMgr.h"

#define SHADERS_MAX_COUNT 2048

namespace EngineCore
{
	// TODO: move shader first loading in background
	class ShaderMgr : public BaseMgr<BaseShader, SHADERS_MAX_COUNT>
	{
	public:
		ShaderMgr();
		uint32_t GetResource(string& name, bool simple = false, onLoadCallback callback = nullptr)
		{
			return BaseMgr<BaseShader, SHADERS_MAX_COUNT>::GetResource(name, simple, callback);
		}
		void CheckForReload();

		inline static ShaderMgr* Get(){return (ShaderMgr*)BaseMgr<BaseShader, SHADERS_MAX_COUNT>::Get();}

	protected:
		uint32_t AddResourceToList(string& name, bool reload, onLoadCallback callback);			
	};
}