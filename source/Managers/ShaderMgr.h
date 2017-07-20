#pragma once
#include "stdafx.h"
#include "Shader.h"
#include "BaseMgr.h"

namespace EngineCore
{
	// TODO: move shader first loading in background
	class ShaderMgr : public BaseMgr<BaseShader>
	{
	public:
		ShaderMgr();
		uint32_t GetResource(string& name, bool simple = false, onLoadCallback callback = nullptr)
		{
			return BaseMgr<BaseShader>::GetResource(name, simple, callback);
		}
		void CheckForReload();

	protected:
		uint32_t AddResourceToList(string& name, bool reload, onLoadCallback callback);			
	};
}