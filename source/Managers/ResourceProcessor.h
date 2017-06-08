#pragma once
#include "stdafx.h"
#include "Common.h"

#define UPDATE_PERIOD_DEFAULT 5000.0f

namespace EngineCore
{
	class ResourceProcessor
	{
	public:
		ResourceProcessor();
		~ResourceProcessor();

		void Tick(float dt);

		void Update();

		void ForceUpdate();
		void Preload();

		inline static ResourceProcessor* Get(){return instance;}
	private:
		static ResourceProcessor *instance;
		
		class ShaderCodeMgr* shaderCodeMgr;
		class ShaderMgr* shaderMgr;
		class MaterialMgr* materialMgr;
		class TexMgr* texMgr;
		class StMeshMgr* stmeshMgr;
		class FontMgr* fontMgr;
		class WorldMgr* worldMgr;

		thread* loader;
		mutex m_update;
		condition_variable v_updateRequest;
		bool loaderRunning;

		float updateDelay;
		float updateTime;
	};
}