#pragma once
#include "stdafx.h"
#include "Common.h"

#define LOADING_QUEUE_SIZE 16384
#define IMPORT_QUEUE_SIZE 1024

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

	struct ImportInfo
	{
		string filePath;
		string resourceName;

		bool importMesh;
		// TODO: extended mesh format
		bool isSkinnedMesh;

		bool importSkeleton;
		bool importAnimation;
		bool importCollision;

		bool importTexture;
		DXGI_FORMAT textureFormat;
	};

	typedef function<void (ImportInfo, bool)> onImportCallback;

	struct ImportSlot
	{
		ImportInfo info;
		LoadingStatus status;
		onImportCallback callback;

		ImportSlot() : callback(nullptr), status(LoadingStatus::FAILED) {}
		ImportSlot(ImportInfo i, onImportCallback func) : info(i), callback(func), status(LoadingStatus::FAILED) {}
	};

	class ResourceProcessor
	{
	public:
		ResourceProcessor();
		~ResourceProcessor();

		void Tick();

		void ThreadMain();
		bool QueueLoad(uint32_t id, ResourceType type, onLoadCallback callback, bool clone = false);

		bool QueueImport(ImportInfo info, onImportCallback callback, bool clone = false);

		void AddUpdateJobs();
		void DeleteUpdateJobs();

		void Preload(string& filename, ResourceType type);

		void WaitLoadingComplete();

		inline static ResourceProcessor* Get(){return instance;}

		static void RegLuaFunctions();

		static bool ImportResource(const ImportInfo& info);

	private:
		bool loadResource(const ResourceSlot& loadingSlot);

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

#ifdef _EDITOR
		RQueueLockfree<ImportSlot>* importQueue;
		RQueueLockfree<ImportSlot>* postImportQueue;
#endif
	};
}