#pragma once
#include "stdafx.h"
#include "Common.h"

#define LOADING_QUEUE_SIZE 16384

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
		COLLISION,
		SKELETON,
		ANIMATION,
		MATERIAL,
		FONT,
		SHADER,
		GUI_SHADER,
		COMPUTE,
	};

	enum LoadingStatus
	{
		NEW = 0,
		LOADED,
		FAILED,
	};

	//typedef void (*onLoadCallback)(uint32_t, bool);
	typedef function<void (uint32_t, bool)> onLoadCallback;

	struct ResourceSlot
	{
		uint32_t id;
		ResourceType type;
		LoadingStatus status;
		onLoadCallback callback;

		ResourceSlot() : id(0), type(ResourceType::TEXTURE), callback(nullptr), status(LoadingStatus::FAILED) {}
		ResourceSlot(uint32_t i, ResourceType t, onLoadCallback func) : id(i), type(t), callback(func), status(LoadingStatus::FAILED) {}
	};

	class ResourceProcessor
	{
	public:
		ResourceProcessor();
		~ResourceProcessor();

		void Tick();

		void ThreadMain();
		bool QueueLoad(uint32_t id, ResourceType type, onLoadCallback callback, bool clone = false);

		void AddUpdateJobs();
		void DeleteUpdateJobs();

		void Preload(string& filename, ResourceType type);

		void WaitLoadingComplete();

		inline static ResourceProcessor* Get(){return instance;}

		static void RegLuaFunctions();
	private:
		static ResourceProcessor *instance;
		
		class ShaderCodeMgr* shaderCodeMgr;
		class ShaderMgr* shaderMgr;
		class MaterialMgr* materialMgr;
		class TexMgr* texMgr;
		class MeshMgr* meshMgr;
		class CollisionMgr* collisionMgr;
		class FontMgr* fontMgr;
		class WorldMgr* worldMgr;

		RQueueLockfree<ResourceSlot>* loadingQueue;
		RQueueLockfree<ResourceSlot>* postLoadingQueue;

		thread* loader;
		mutex m_loading;
		mutex m_complete;
		condition_variable v_loadingRequest;
		condition_variable v_loadingComplete;
		bool loadingComplete;
		bool loaderRunning;
	};
}