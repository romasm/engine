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

#define IMP_BYTE_TEXTURE		uint8_t(1<<0)
#define IMP_BYTE_MESH			uint8_t(1<<1)
#define IMP_BYTE_COLLISION		uint8_t(1<<2)
#define IMP_BYTE_SKELETON		uint8_t(1<<3)
#define IMP_BYTE_ANIMATION		uint8_t(1<<4)

#define IMPORT_FILE_VERSION 101

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

		uint8_t importBytes;

		bool isSkinnedMesh;
		DXGI_FORMAT textureFormat;
		bool genMips;
		uint32_t genMipsFilter;

		ImportInfo()
		{
			importBytes = 0;
			isSkinnedMesh = false;
			textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			genMips = true;
			genMipsFilter = TEX_FILTER_DEFAULT;
		};

		void parseFromLua(LuaRef params);
		
		static uint32_t sizeNoString()
		{
			return sizeof(uint8_t) + 
				sizeof(bool) + 
				sizeof(DXGI_FORMAT) +
				sizeof(bool) +
				sizeof(uint32_t);
		};
	};

	typedef function<void (const ImportInfo&, bool)> onImportCallback;

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
		bool QueueLoad(uint32_t id, ResourceType type, onLoadCallback callback = nullptr, bool clone = false);

		bool QueueImport(ImportInfo info, onImportCallback callback = nullptr, bool clone = false);

		void AddUpdateJobs();
		void DeleteUpdateJobs();

		void Preload(string& filename, ResourceType type);
		void Drop(string& filename, ResourceType type);

		void WaitLoadingComplete();

		inline static ResourceProcessor* Get(){return instance;}

		static void RegLuaFunctions();

		static bool ImportResource(ImportInfo& info, bool force = false);
		static bool SaveImportInfo(string& resFile, ImportInfo& info);
		static void LoadImportInfo(string& resName, ImportInfo& info, uint32_t& date);

		static bool CheckImportNeeded(ImportInfo& info, uint32_t date, string& resFile);

	private:
		bool loadResource(const ResourceSlot& loadingSlot);

		static ResourceProcessor *instance;
		
		class ShaderCodeMgr* shaderCodeMgr;
		class ShaderMgr* shaderMgr;
		class MaterialMgr* materialMgr;
		class TexMgr* texMgr;
		class MeshMgr* meshMgr;
		class SkeletonMgr* skeletonMgr;
		class AnimationMgr* animationMgr;
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