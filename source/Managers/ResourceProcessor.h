#pragma once
#include "stdafx.h"
#include "Common.h"

#define LOADING_QUEUE_SIZE 4096

#define SHADERS_UPDATE_PERIOD 2000.0f
#define TEXTURES_UPDATE_PERIOD 3000.0f
#define STMESHES_UPDATE_PERIOD 5000.0f

#define SHADER_JOB_NAME "ShaderUpdate"
#define SHADERCODE_JOB_NAME "ShaderCodeUpdate"
#define TEXTURE_JOB_NAME "TexturesUpdate"
#define STMESH_JOB_NAME "StMeshesUpdate"

namespace EngineCore
{
	enum ResourceType
	{
		TEXTURE = 0,
		MESH,
	};

	struct ResourceSlot
	{
		uint32_t id;
		ResourceType type;

		ResourceSlot() : id(0), type(ResourceType::TEXTURE) {}
		ResourceSlot(uint32_t i, ResourceType t) : id(i), type(t) {}
	};

	class ResourceProcessor
	{
	public:
		ResourceProcessor();
		~ResourceProcessor();

		void Tick(float dt);

		void Loading();
		bool QueueLoad(uint32_t id, ResourceType type);

		void Preload();
		void AddUpdateJobs();

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

		RQueueLockfree<ResourceSlot>* loadingQueue;

		thread* loader;
		mutex m_loading;
		condition_variable v_loadingRequest;
		bool loaderRunning;
	};
}